//this has the includes of all files in one consolidated spot
#include "powerup.h"
#include "raylib.h"
#include <string.h>

//great macro function
#define ARRAY_LENGTH(arr) ( sizeof(char[1 - 2*__builtin_types_compatible_p(__typeof__(arr), __typeof__(&(arr)[0]))]) * 0 + sizeof(arr) / sizeof((arr)[0]) )

static PowerUp getPowerUp(char *name){
    for (int i = 0; i < ARRAY_LENGTH(powerupsInternal); i++)
        if (strcmp(name, powerupsInternal[i].powerUpName) == 0) return powerupsInternal[i].powerUp;
    return (PowerUp){0};
}

static void spawnPowerUp(BrickData *poweruptoload, CollectiblePowerup *storage, Vector2 spawnPosition){
    for (int i = 0; i < COLLECTIBLE_POWERUP_LIMIT; i++){
        //continue doesn't work, okay note to self, continue means skip, as it skips to the next integer in the array, would've been nice to know that earlier
        if (memcmp(&storage[i], &emptyCollectiblePowerUp, sizeof(CollectiblePowerup)) == 0){
            storage[i].powerUpIcons = PowerupAtlas;
            storage[i].drawOffset = (Rectangle){72, 24, 24, 24};
            storage[i].position = spawnPosition;
            storage[i].speed = (Vector2){0, 100};
            storage[i].bounds = (Rectangle){0, 0, 24, 24};
            //check if containedPowerup is empty, if it is, get a random power up
            if (memcmp(&storage[i].containedPowerup, &emptyPowerUp, sizeof(PowerUp)) == 0){
                int randomIndex = GetRandomValue(1, ARRAY_LENGTH(powerupsInternal) - 1); //power up 0 is a debug power up
                storage[i].containedPowerup = powerupsInternal[randomIndex].powerUp;
                storage[i].drawOffset = powerupsInternal[randomIndex].powerUp.PowerUpTextureOffset;
            }
            else{
                //oh, the contained data is in the brick
                if (poweruptoload->containedPowerUp != NULL){
                    storage[i].containedPowerup = getPowerUp(poweruptoload->containedPowerUp);
                    storage[i].drawOffset = storage[i].containedPowerup.PowerUpTextureOffset;
                }
                else{
                    //panic handler
                    storage[i].containedPowerup = getPowerUp("empty");
                }
            }
            break;
        }; //check if empty
    }
}

static void addPowerUp(Player *paddle, PowerUp *power){
    for (int i = 0; i < POWER_UP_COUNT; i++){
        //continue doesn't work, okay note to self, continue means skip, as it skips to the next instance of the loop, would've been nice to know that earlier
        if (memcmp(&paddle->powerups[i], &emptyPowerUp, sizeof(PowerUp)) == 0){
            paddle->powerups[i] = *power;
            break;
        }
    }
}

//load Level Data
Image LoadLevel(const char *path){
    return LoadImage(path);
}

//Draw the Actual Level
void PrepareLevel(Player *playerInst){
    // 7/8?, you know what its shockingly effective
    playerInst->speed = (Vector2){8.0f, 0.0f}; //floats are defined with .0 and f (1.0f), kinda weird, i mean 1.5 is not an integer so why can't it just find it as a float?
    playerInst->size = (Vector2){100,24};
    playerInst->lives = PLAYER_LIVES;
    playerInst->paddleSegments = 56; //that's the default
    playerInst->position = (Vector2){GetScreenWidth()/2.0f - ((playerInst->paddleSegments + 44.0f)/2), GetScreenHeight()*7/8.0f};
    for (int i=0; i < POWER_UP_COUNT; i++) memset(&playerInst->powerups[i], 0, sizeof(PowerUp)); //zero out the array
    for (int i=0; i < BALL_LIMIT; i++) memset(&playerInst->balls[i], 0, sizeof(Ball)); //zero out the array
    //the initial ball
    playerInst->balls[0] = (Ball){
        .radius = 10.0f,
        .active = false,
        .paddleHit = false,
        .speed = (Vector2){4.0f, 4.0f}
    };
    //this needs to be seperated because it needs to struct to be initialized first
    playerInst->balls[0].position = (Vector2){ playerInst->position.x + (playerInst->paddleSegments + 44.0f)/2, playerInst->position.y - playerInst->balls[0].radius*2 };
    for (int j=0; j < BRICK_LINES; j++){
        for (int i=0; i < BRICKS_PER_LINE; i++){
            Color color = GetImageColor(levelData, i, j);
            if (memcmp(&color, &GRAY, sizeof(Color)) == 0){//regular brick
                bricks[j][i].data = (BrickData){
                    .brickTexture = texBrick,
                    .resistance = 0,
                    .color = color,
                    .hasPowerup = false,
                    .containedPowerUp = NULL
                };
                bricks[j][i].size = (Vector2){(float)GetScreenWidth()/BRICKS_PER_LINE, 20};
                bricks[j][i].position = (Vector2){i*bricks[j][i].size.x, j*bricks[j][i].size.y + BRICKS_Y_POSITION};
                bricks[j][i].bounds = (Rectangle){bricks[j][i].position.x, bricks[j][i].position.y, bricks[j][i].size.x, bricks[j][i].size.y };
                bricks[j][i].active = true;
            }
            else if (memcmp(&color, &GREEN, sizeof(Color)) == 0){//"brick" brick
                bricks[j][i].data = (BrickData){
                    .brickTexture = texHeavyBrick,
                    .resistance = 1,
                    .color = color,
                    .hasPowerup = false,
                    .containedPowerUp = NULL
                };
                bricks[j][i].size = (Vector2){(float)GetScreenWidth()/BRICKS_PER_LINE, 20};
                bricks[j][i].position = (Vector2){i*bricks[j][i].size.x, j*bricks[j][i].size.y + BRICKS_Y_POSITION};
                bricks[j][i].bounds = (Rectangle){bricks[j][i].position.x, bricks[j][i].position.y, bricks[j][i].size.x, bricks[j][i].size.y };
                bricks[j][i].active = true;
            }
            else if (memcmp(&color, &ORANGE, sizeof(Color)) == 0){//powerup brick
                bricks[j][i].data = (BrickData){
                    .brickTexture = texBrick,
                    .resistance = 0,
                    .color = color,
                    .hasPowerup = true,
                    .containedPowerUp = NULL
                };
                bricks[j][i].size = (Vector2){(float)GetScreenWidth()/BRICKS_PER_LINE, 20};
                bricks[j][i].position = (Vector2){i*bricks[j][i].size.x, j*bricks[j][i].size.y + BRICKS_Y_POSITION};
                bricks[j][i].bounds = (Rectangle){bricks[j][i].position.x, bricks[j][i].position.y, bricks[j][i].size.x, bricks[j][i].size.y };
                bricks[j][i].active = true;
            }
        }
    }
}
