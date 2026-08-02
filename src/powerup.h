//soon-ish
#include "raylib.h"
#include <string.h>
#include "data.h"
#include "raylib.h"

//a basic timeout thingamajig
static void pwr_UpgradeTimeout(Player *instance, float *duration, unsigned int arrIndex){
    if (*duration > 0) {
        *duration -= GetFrameTime();
    }
    else {
        //uh, some way to erase itself from the array
        //so i would need to pass the ID of the power up when its instantiated
        //so my current idea is the for loop that iterates through the power ups
        //it is an index, so i can pass the index here
        if (instance->powerups[arrIndex].powerOnEnd != NULL) instance->powerups[arrIndex].powerOnEnd(instance, arrIndex);
        memset(&instance->powerups[arrIndex], 0, sizeof(PowerUp));

    }
}

static void pwr_load_IncreasePaddleSize(Player *instance, unsigned int arrIndex){
    instance->paddleSegments += 15;
    instance->position.x -= 15.0f/2;
}

static void pwr_end_IncreasePaddleSize(Player *instance, unsigned int arrIndex){
    instance->paddleSegments -= 15;
    instance->position.x += 15.0f/2;
}

static void pwr_load_DecreasePaddleSize(Player *instance, unsigned int arrIndex){
    instance->paddleSegments -= 15;
    instance->position.x += 15.0f/2;
}

static void pwr_end_DecreasePaddleSize(Player *instance, unsigned int arrIndex){
    instance->paddleSegments += 15;
    instance->position.x -= 15.0f/2;
}

static void pwr_load_DoubleBall(Player *instance, unsigned int arrIndex){//note to self, do not zero out struct with = {0}, use memset instead
    for (int b = 0; b < BALL_LIMIT; b++){
        if (memcmp(&instance->balls[b], &emptyBall, sizeof(Ball)) == 0) {//check if even possible
            instance->balls[b] = (Ball){
                .position = (Vector2){0, 0},
                .radius = 10.0f,
                .speed = (Vector2){4, 4},
                .active = true,
                .paddleHit = false,
            };
            for (int e = 0; e < BALL_LIMIT; e++){
                if (memcmp(&instance->balls[e], &emptyBall, sizeof(Ball)) != 0) {//check for an existing ball to latch on to
                    //okay so memcmp may or may not be unreliable here
                    instance->balls[b].position = instance->balls[e].position;
                    if (instance->balls[e].speed.x > 0) {
                        instance->balls[b].speed.x = instance->balls[e].speed.x;
                    } else if (instance->balls[e].speed.x < 0) {
                        instance->balls[b].speed.x = -instance->balls[e].speed.x;
                    }
                    else {
                        switch (GetRandomValue(0, 1)) {
                        case 0:
                            instance->balls[b].speed.x = 5;
                            break;
                        case 1:
                            instance->balls[b].speed.x = -5;
                            break;
                        }
                    }
                    instance->balls[b].speed.y = -5;
                }
                break;
            }
            memset(&instance->powerups[arrIndex], 0, sizeof(PowerUp));
            break;
        }
    }
}

static const powerUpData powerupsInternal[] = {
    {.powerUpName = "empty", .powerUp = {
        .powerOnLoad = pwr_load_IncreasePaddleSize,
        .powerUpdate = pwr_UpgradeTimeout,
        .powerOnEnd = pwr_end_IncreasePaddleSize,
        .powerUpDuration = 10.0f,
        .PowerUpTextureOffset = (Rectangle){72, 24, 24, 24}
    }},
    {.powerUpName = "paddleIncrease", .powerUp = {
        .powerOnLoad = pwr_load_IncreasePaddleSize,
        .powerUpdate = pwr_UpgradeTimeout,
        .powerOnEnd = pwr_end_IncreasePaddleSize,
        .powerUpDuration = 10.0f,
        .PowerUpTextureOffset = (Rectangle){0, 0, 24, 24}
    }},
    {.powerUpName = "paddleDecrease", .powerUp = {
        .powerOnLoad = pwr_load_DecreasePaddleSize,
        .powerUpdate = pwr_UpgradeTimeout,
        .powerOnEnd = pwr_end_DecreasePaddleSize,
        .powerUpDuration = 10.0f,
        .PowerUpTextureOffset = (Rectangle){24, 0, 24, 24}
    }},
    {.powerUpName = "doubleBall", .powerUp = {
        //since the new ball doesn't really need a timer, this can just empty itself on load
        .powerOnLoad = pwr_load_DoubleBall,
        .powerUpdate = NULL,
        .powerOnEnd = NULL,
        .powerUpDuration = 0.0f,
        .PowerUpTextureOffset = (Rectangle){48, 0, 24, 24}
    }},
};
