#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 750
#define SCREEN_HEIGHT 800
#define SPACESHIP_WIDTH 60
#define SPACESHIP_HEIGHT 60
#define BULLET_WIDTH 20
#define BULLET_HEIGHT 20
#define BULLET_SPEED 5
#define SPACESHIP_SPEED 5
#define MAX_BULLETS 10
#define MAX_DEBRIS 20
#define MAX_STARS 50
#define TICK_INTERVAL 16
#define RELOAD_TIME 2000
#define WELCOME_SCREEN_DURATION 2000

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Rect bullets[MAX_BULLETS];
SDL_Rect debris[MAX_DEBRIS];
SDL_Rect stars[MAX_STARS];
int starSpeeds[MAX_STARS];
int bulletActive[MAX_BULLETS] = {0};
TTF_Font* font = NULL;
SDL_Texture* spaceshipTexture = NULL;
SDL_Texture* debrisTexture = NULL;
SDL_Texture* bulletTexture = NULL;
SDL_Texture* backgroundTexture = NULL;
SDL_Texture* welcomeTexture = NULL;
SDL_Texture* welcomeBackgroundTexture = NULL;
SDL_Rect spaceshipRect;
int spaceshipVelocityX = 0;
int spaceshipVelocityY = 0;
int score = 0;
int gameOver = 0;
Uint32 next_game_tick = 0;
int bulletsFired = 0;
Uint32 lastBulletTime = 0;
Mix_Chunk* shootSound = NULL;
Mix_Chunk* hitSound = NULL;
Mix_Chunk* gameOverSound = NULL;

int initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return -1;
    } if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image initialization failed: %s\n", IMG_GetError());
        return -1;
    } if (TTF_Init() == -1) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        return -1;
    }
    
    window = SDL_CreateWindow("Space Debris Shooter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer initialization failed: %s\n", Mix_GetError());
        return -1;
    }
    return 0;
}

void loadAssets() {
    SDL_Surface* backgroundSurface = IMG_Load("one.png");
    if (backgroundSurface == NULL) {
        printf("Failed to load background image: %s\n", IMG_GetError());
        return;
    }
    backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
    SDL_FreeSurface(backgroundSurface);

    SDL_Surface* spaceshipSurface = IMG_Load("rocket.png");
    if (spaceshipSurface == NULL) {
        printf("Failed to load spaceship image: %s\n", IMG_GetError());
        return;
    }
    spaceshipTexture = SDL_CreateTextureFromSurface(renderer, spaceshipSurface);
    SDL_FreeSurface(spaceshipSurface);

    SDL_Surface* debrisSurface = IMG_Load("asteroid.png");
    if (debrisSurface == NULL) {
        printf("Failed to load debris image: %s\n", IMG_GetError());
        return;
    }
    debrisTexture = SDL_CreateTextureFromSurface(renderer, debrisSurface);
    SDL_FreeSurface(debrisSurface);

    SDL_Surface* bulletSurface = IMG_Load("bullet.png");
    if (bulletSurface == NULL) {
        printf("Failed to load bullet image: %s\n", IMG_GetError());
        return;
    }
    bulletTexture = SDL_CreateTextureFromSurface(renderer, bulletSurface);
    SDL_FreeSurface(bulletSurface);

    spaceshipRect.x = (SCREEN_WIDTH - SPACESHIP_WIDTH) / 2;
    spaceshipRect.y = SCREEN_HEIGHT - SPACESHIP_HEIGHT - 20;
    spaceshipRect.w = SPACESHIP_WIDTH;
    spaceshipRect.h = SPACESHIP_HEIGHT;
    
    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].w = BULLET_WIDTH;
        bullets[i].h = BULLET_HEIGHT;
        bullets[i].x = -BULLET_WIDTH;
        bullets[i].y = -BULLET_HEIGHT;
    }
    srand(time(NULL));
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        int debrisSize = rand() % 30 + 20;
        debris[i].w = debrisSize;
        debris[i].h = debrisSize;
        debris[i].x = rand() % (SCREEN_WIDTH - debris[i].w);
        debris[i].y = -rand() % SCREEN_HEIGHT;
    }
    for (int i = 0; i < MAX_STARS; ++i) {
        stars[i].w = 2;
        stars[i].h = 2;
        stars[i].x = rand() % SCREEN_WIDTH;
        stars[i].y = rand() % SCREEN_HEIGHT;
        starSpeeds[i] = rand() % 3 + 1;
    }
    font = TTF_OpenFont("game_over.ttf", 48);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }
    shootSound = Mix_LoadWAV("shoot.wav");
    if (shootSound == NULL) printf("Failed to load shoot sound effect: %s\n", Mix_GetError());
    hitSound = Mix_LoadWAV("destroyed.wav");
    if (hitSound == NULL) printf("Failed to load hit sound effect: %s\n", Mix_GetError());
    gameOverSound = Mix_LoadWAV("game_over.wav");
    if (gameOverSound == NULL) printf("Failed to load game over sound effect: %s\n", Mix_GetError());
}

void render() {
    SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < MAX_STARS; ++i) {
        SDL_RenderDrawRect(renderer, &stars[i]);
    }
    SDL_RenderCopy(renderer, spaceshipTexture, NULL, &spaceshipRect);
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bulletActive[i]) SDL_RenderCopy(renderer, bulletTexture, NULL, &bullets[i]);
    }
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        SDL_Rect destRect = {debris[i].x, debris[i].y, debris[i].w, debris[i].h};
        SDL_RenderCopy(renderer, debrisTexture, NULL, &destRect);
    }
    SDL_Color textColor = {255, 255, 255, 255};
    char scoreText[50];
    sprintf(scoreText, "SCORE: %d", score);
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText, textColor);
    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_FreeSurface(scoreSurface);
    SDL_Rect scoreRect = {SCREEN_WIDTH - 120, 20, 100, 30};
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
    SDL_DestroyTexture(scoreTexture);

    if (gameOver) {
        SDL_Color bgColor = {255, 255, 255, 255};
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_Rect bgRect = {0, SCREEN_HEIGHT / 2 - 50, SCREEN_WIDTH, 60};
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Color gameOverColor = {255, 26, 140, 255};
        char gameOverText[50];
        sprintf(gameOverText, "GAME OVER");
        SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, gameOverText, gameOverColor);
        SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
        SDL_FreeSurface(gameOverSurface);
        SDL_Rect gameOverRect = {(SCREEN_WIDTH - 200) / 2, SCREEN_HEIGHT / 2 - 55, 200, 60};
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        SDL_DestroyTexture(gameOverTexture);
        if (gameOverSound != NULL) Mix_PlayChannel(-1, gameOverSound, 0);
    }
    SDL_RenderPresent(renderer);
}

void handleInput(SDL_Event* event) {
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
    spaceshipVelocityX = 0;
    spaceshipVelocityY = 0;
    if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A]) {
        spaceshipVelocityX = -SPACESHIP_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D]) {
        spaceshipVelocityX = SPACESHIP_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W]) {
        spaceshipVelocityY = -SPACESHIP_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S]) {
        spaceshipVelocityY = SPACESHIP_SPEED;
    }
    if (!gameOver && event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        switch (event->key.keysym.sym) {
            case SDLK_SPACE:
                Uint32 currentTicks = SDL_GetTicks();
                if (currentTicks - lastBulletTime > RELOAD_TIME) {
                    for (int i = 0; i < MAX_BULLETS; ++i) {
                        if (!bulletActive[i]) {
                            bullets[i].x = spaceshipRect.x + (SPACESHIP_WIDTH - BULLET_WIDTH) / 2;
                            bullets[i].y = spaceshipRect.y - BULLET_HEIGHT;
                            bulletActive[i] = 1;
                            bulletsFired++;
                            if (bulletsFired >= 10) {
                                lastBulletTime = currentTicks;
                                bulletsFired = 0;
                            }
                            if (shootSound != NULL) {
                                Mix_PlayChannel(-1, shootSound, 0);
                            } break;
                        }
                    }
                } break;
        }
    }
}

void updateGame() {
    if (gameOver) return;
    spaceshipRect.x += spaceshipVelocityX;
    spaceshipRect.y += spaceshipVelocityY;
    if (spaceshipRect.x < 0) spaceshipRect.x = 0;
    if (spaceshipRect.x > SCREEN_WIDTH - SPACESHIP_WIDTH) spaceshipRect.x = SCREEN_WIDTH - SPACESHIP_WIDTH;
    if (spaceshipRect.y < 0) spaceshipRect.y = 0;
    if (spaceshipRect.y > SCREEN_HEIGHT - SPACESHIP_HEIGHT) spaceshipRect.y = SCREEN_HEIGHT - SPACESHIP_HEIGHT;
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bulletActive[i]) {
            bullets[i].y -= BULLET_SPEED;
            for (int j = 0; j < MAX_DEBRIS; ++j) {
                if (debris[j].y <= SCREEN_HEIGHT && SDL_HasIntersection(&bullets[i], &debris[j])) {
                    bulletActive[i] = 0;
                    debris[j].w = rand() % 30 + 20;
                    debris[j].h = debris[j].w;
                    debris[j].x = rand() % (SCREEN_WIDTH - debris[j].w);
                    debris[j].y = -rand() % SCREEN_HEIGHT;
                    score++;
                    if (hitSound != NULL) Mix_PlayChannel(-1, hitSound, 0);
                }
            }
            if (bullets[i].y < 0) bulletActive[i] = 0;
        }
    }
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        if (debris[i].y <= SCREEN_HEIGHT) {
            debris[i].y += 2;
            if (SDL_HasIntersection(&spaceshipRect, &debris[i])) gameOver = 1;
        } else {
            debris[i].w = rand() % 30 + 20;
            debris[i].h = debris[i].w;
            debris[i].x = rand() % (SCREEN_WIDTH - debris[i].w);
            debris[i].y = -rand() % SCREEN_HEIGHT;
        }
    }
    for (int i = 0; i < MAX_STARS; ++i) {
        stars[i].y += starSpeeds[i];
        if (stars[i].y > SCREEN_HEIGHT) {
            stars[i].y = 0;
            stars[i].x = rand() % SCREEN_WIDTH;
            starSpeeds[i] = rand() % 3 + 1;
        }
    }
}

void showWelcomeScreen()
{
    SDL_Color welcomeColor = {255, 255, 255, 255};
    char welcomeText[50];
    sprintf(welcomeText, "Don't fly too close!");

    SDL_Surface* welcomeBackgroundSurface = IMG_Load("bg.png");
    welcomeBackgroundTexture = SDL_CreateTextureFromSurface(renderer, welcomeBackgroundSurface);
    SDL_FreeSurface(welcomeBackgroundSurface);
    SDL_Rect welcomeBackgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, welcomeBackgroundTexture, NULL, &welcomeBackgroundRect);

    SDL_Surface* welcomeSurface = TTF_RenderText_Solid(font, welcomeText, welcomeColor);
    SDL_Texture* welcomeTexture = SDL_CreateTextureFromSurface(renderer, welcomeSurface);
    SDL_FreeSurface(welcomeSurface);
    SDL_Rect welcomeRect = {(SCREEN_WIDTH - 300) / 2, SCREEN_HEIGHT / 2 - 25, 300, 50};
    SDL_RenderCopy(renderer, welcomeTexture, NULL, &welcomeRect);
    SDL_RenderPresent(renderer);
    SDL_Delay(WELCOME_SCREEN_DURATION);

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(welcomeBackgroundTexture);
    SDL_DestroyTexture(welcomeTexture);
}

void closeSDL()
{
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeChunk(gameOverSound);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(spaceshipTexture);
    SDL_DestroyTexture(debrisTexture);
    SDL_DestroyTexture(bulletTexture);
    TTF_Quit();
    IMG_Quit();
    Mix_Quit();
    SDL_Quit();
}

Uint32 time_left(void) {
    Uint32 now = SDL_GetTicks();
    return (next_game_tick > now) ? next_game_tick - now : 0;
}

int main(int argc, char* args[]) {
    if (initSDL() == -1) return -1;
    loadAssets();
    showWelcomeScreen();
    int quit = 0;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = 1;
            else handleInput(&e);
        }
        Uint32 ticks = SDL_GetTicks();
        updateGame();
        render();
        next_game_tick += TICK_INTERVAL;
        Uint32 sleep_time = time_left();
        if (sleep_time > 0) SDL_Delay(sleep_time);
    }
    closeSDL();
    return 0;
}
