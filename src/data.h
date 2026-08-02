#include "raylib.h"
#include "raylib/src/raylib.h"
#include <stddef.h> //all of this just for
                    // #define NULL ((void *)0)
//so variables can also be literally defined like this?
//these are not variables they're macros, they're typeless and without memory allocation,
//so a typeless but optimized static const? when is that useful?.
//turns out a lot, these are fun
#define PLAYER_LIVES        5
#define BRICK_LINES         22
#define BRICKS_PER_LINE     20 //from research this currently uses DX-Ball logic as its 20 per line,
//the Arkanoid arcade game use 13, the nes version of it uses 11, the original Atari Breakout uses 14, the fuck?
//the point of note for this is that each number gives a moderately different difficulty, the game designer in me is cringing at the thought of that
#define BRICKS_Y_POSITION   0
#define POWER_UP_COUNT      50
#define COLLECTIBLE_POWERUP_LIMIT 50
#define BALL_LIMIT          10

//typedefs need both an identifier and a name? aren't those the same thing?
//so it seems one is for the typedef itself and one is for the identifer that you then call like a type, weird.
typedef enum GameScreen{LOGO, TITLE, GAMEPLAY,ENDING}GameScreen;

struct Player; //forward declaration, because C is built like a deck of cards

typedef struct Ball{
    Vector2 position; float radius;
    Vector2 speed;
    bool active;
    bool paddleHit; //i don't like this but i don't see a better option.
}Ball;

typedef struct PowerUp{
    void (*powerOnLoad)(struct Player *instance, unsigned int arrIndex);
    void (*powerUpdate)(struct Player *instance, float *duration, unsigned int arrIndex);
    void (*powerOnEnd)(struct Player *instance, unsigned int arrIndex);
    //void *customData; //customData is a generic type for holding arbitrary data used by some of the power ups
    float powerUpDuration; //meant for the update above to use
    Rectangle PowerUpTextureOffset;
}PowerUp;

typedef struct CollectiblePowerup{
    Texture2D powerUpIcons;
    Rectangle drawOffset;
    Vector2 position;
    Vector2 speed;
    Rectangle bounds;
    PowerUp containedPowerup;
}CollectiblePowerup;

typedef struct Player{
    PowerUp powerups[POWER_UP_COUNT]; //power up storage, i say about 50 because if you're gonna have that many powerups then there's a problem
    signed int paddleSegments;
    Vector2 position; Vector2 size;
    Vector2 speed;
    Rectangle bounds;
    int lives;
    Ball balls[BALL_LIMIT]; //a storage of balls, meant for power ups to manipulate
}Player;

typedef struct BrickData{
    Texture2D brickTexture;
    unsigned int resistance; //0 means one-shot, 1 means two shot
    _Bool isCracked; //controls the overlay that shows if it can take another hit
    Color color;
    _Bool hasPowerup; //if true: drop power up on inactivity
    char *containedPowerUp; //if null get random power up
}BrickData;

typedef struct Brick{
    BrickData data;
    Vector2 position; Vector2 size;
    Rectangle bounds;
    bool active;
}Brick;

typedef struct powerUpData{
    char *powerUpName;
    PowerUp powerUp;
}powerUpData;

typedef struct Level{//the idea being is that the current level index corresponds to the level's resource
    char *levelResource;
}Level;

static const Level levelsInternal[] = {
    { "resources/level1.png" },
    { "resources/level2.png" },
    { "resources/level3.png" },
    { "resources/level4.png" }
};

PowerUp emptyPowerUp = {0};
CollectiblePowerup emptyCollectiblePowerUp = {0};
Ball emptyBall = {0};
Brick bricks[BRICK_LINES][BRICKS_PER_LINE] = {0};
Image levelData = {0};

//---RESOURCES---//
//---TITLE---//
Texture2D theFunny = {0};
//---MAIN---//
Font font = {0};
Texture2D texLogo = {0};
Texture2D texBall = {0};
//---PADDLE TEXTURES---//
Texture2D texPaddlePieces = {0};
//---BRICK TEXTURES---//
Texture2D texBrick = {0}; Texture2D texHeavyBrick = {0};
Texture2D texPowerupOverlay = {0}; Texture2D texCrackOverlay = {0};
//---MISC---//
Texture2D emptyPowerUpIcon = {0}; Texture2D PowerupAtlas = {0};
//---MUSIC AND SFX---//
Music music = {0};
Sound fxStart = {0}; Sound fxBounce = {0}; Sound fxExplode = {0};
Sound fxCrack = {0}; Sound fxScratch = {0}; Sound fxWomp = {0};
//---GLOBAL GAME STORAGE---//
CollectiblePowerup collectibleStorage[COLLECTIBLE_POWERUP_LIMIT]; //in theory there shouldn't be more in it
unsigned int score = 0;
unsigned int currentLevel = 0;
