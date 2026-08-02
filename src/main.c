/*******************************************************************************************
*   Raylib Breakout
*   from: https://github.com/raysan5/raylib-intro-course
********************************************************************************************/
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <string.h>
#include "macros.h"
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//so to use functions, you need to define them beforehand?, wack.
//with newfound knowledge, its done because C reads sequentually, so if you define a function after it is used, it will not be found.
static void main_loop(void);// contains the bottom 2
static void Update(void);// Update one frame
static void Draw(void);// draw one frame afterwards
static _Bool BallCollision(Ball *ball, Brick *brick); //a macro function for ball collision
static _Bool CheckLevelCompletion(void); //checks if the level is complete by checking if all bricks are inactive

GameScreen screen = LOGO;
unsigned int framesCounter = 0; //i love unsigned integers, why don't more programming languages have them as an option?, i get that its like autistic levels of control but still
signed int gameResult = -1;
bool gamePaused = false;
bool gameStarted = false;
const int screenWidth = 800; //finally i have constants
const int screenHeight = 440;

Player player = {0}; //instantiate a player typedef
//---GLOBAL GAME STORAGE---//
CollectiblePowerup collectibleStorage[COLLECTIBLE_POWERUP_LIMIT]; //in theory there shouldn't be more in it

int main()
{
    InitWindow(screenWidth, screenHeight, "BRKT, coded in a week edition");
    SetWindowMonitor(0); //for some reason it goes to the highest monitor index first, a GLFW bug it seems, doesn't happen with SDL
    //had sigsev errors i couldn't understand, these need to be AFTER the window gets created and not any time beforehand
    theFunny =          LoadTexture("resources/Soyjaklib.png");
    font =              LoadFont("resources/setback.png");
    texLogo =           LoadTexture("resources/raylib_logo.png"); //this causes an error
    texPaddlePieces =   LoadTexture("resources/paddle_pieces.png");
    texBall =           LoadTexture("resources/ball.png");
    texBrick =          LoadTexture("resources/brick.png");
    texHeavyBrick =     LoadTexture("resources/brick_solid.png");
    levelData =         LoadImage("resources/levelTest.png");
    texPowerupOverlay = LoadTexture("resources/Power-Up_Overlay.png");
    texCrackOverlay =   LoadTexture("resources/Cracked_Overlay.png");
    emptyPowerUpIcon =  LoadTexture("resources/emptyPowerUp.png");
    PowerupAtlas =      LoadTexture("resources/PowerUps.png");

    InitAudioDevice(); //audio is optional, yo that's funky
    // https://modarchive.org/index.php?request=view_by_moduleid&query=166013 found it manually
    music =             LoadMusicStream("resources/blockshock.mod"); //it can play mod tracker files for some reason, what in the Amiga 500
    fxStart =           LoadSound("resources/start.wav");
    fxBounce =          LoadSound("resources/bonk.wav");
    fxExplode =         LoadSound("resources/explosion.wav");
    fxCrack =           LoadSound("resources/crack.wav");
    fxScratch =         LoadSound("resources/scratch.wav");
    fxWomp =            LoadSound("resources/BallLeftForMilk.wav");
    PlayMusicStream(music);

//both while loops prevent the endif from ever being reached, only ever getting to it when the application is closed
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(main_loop, 60, 1); //raylib would tell you to set the frame rate to 0 so that it will get frame rate natively,
    //don't unless you want a ton of jitter, needs to be investigated
#else
    SetTargetFPS(60);               // Set our game to cap at 60 frames-per-second
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        //in the tutorial, it lists variables as local to main, meanwhile the way raylib sets up multi platform projects is like this, so variables need to be module local
        main_loop();
    }
#endif
    CloseWindow();                  // kill the window and OpenGL context on close
    return 0;
}

//this is for emscripten, because something something it needs it to simulate an infinite while loop
//but its not usable like a regular while loop, so then this
static void main_loop(void){
    Update();
    //fixedUpdate
    Draw();
}

// Update and draw game frame
// should be split into update and draw seperately
static void Update(void)
{
    //---UPDATE CODE---//
    switch (screen) {
        case LOGO:{
            framesCounter++;
            if (framesCounter > 180){
                screen = TITLE;
                framesCounter = 0;
            }
        }break; //so this is because C allows multiple cases to use the same assignment, that's weird, this is technically not needed but it seems its good practice
        case TITLE:{
            // framesCounter++; these will be hidden for now til i understand what use they serve
            if (gamePaused) gamePaused = false;
            if (IsKeyPressed(KEY_SPACE)) {
                screen = GAMEPLAY;
                PlaySound(fxStart);
            }

        }break;
        case GAMEPLAY:{
            //---GAME START LOGIC---//
            if (!gameStarted){
                gameStarted = true;
                score = 0;
                for (int i = 0; i < ARRAY_LENGTH(collectibleStorage); i++){
                    memset(&collectibleStorage[i], 0, sizeof(CollectiblePowerup));
                }
                currentLevel = 0;
                //---BRICK SPAWNING---//
                // j and i are far too similar
                levelData = LoadLevel(levelsInternal[currentLevel].levelResource);
                PrepareLevel(&player);
            }

            if (IsKeyPressed('P')) gamePaused = !gamePaused; //i should've thought about this sooner, this is clever, its a single line toggle
            if (!gamePaused){
                //PADDLE //switched to A and D because my keyboard is falling apart and the left keyswitch's dead
                if (IsKeyDown(KEY_LEFT)) player.position.x -= player.speed.x; //case in point, it uses player.speed.x, instead of just player.speed
                if (IsKeyDown(KEY_RIGHT)) player.position.x += player.speed.x;

                if ((player.position.x) <= 0) player.position.x = 0;
                if ((player.position.x + (player.paddleSegments + 44) ) >= screenWidth ) player.position.x = screenWidth - (player.paddleSegments + 44);
                player.bounds = (Rectangle){ player.position.x, player.position.y, player.paddleSegments + 44, player.size.y }; //get a rectangle from the player position and size
                if (player.paddleSegments < 0) player.paddleSegments = 0; //in case the segments go minus

                //NOTE POWER UP LOGIC
                for (int i = 0; i < POWER_UP_COUNT; i++){
                    if (memcmp(&player.powerups[i], &emptyPowerUp, sizeof(PowerUp)) != 0) {
                        if (player.powerups[i].powerUpdate != NULL) {
                            player.powerups[i].powerUpdate(&player, &player.powerups[i].powerUpDuration, i);
                        }
                    }
                }
                //BALL
                for (int b = 0; b < BALL_LIMIT; b++){
                    if (memcmp(&player.balls[b], &emptyBall, sizeof(Ball)) != 0) {//if it ain't empty
                        if (player.balls[b].active) {
                            //TODO these are frame rate dependent at the moment, will fix later, or not, i don't know
                            //---BALL "PHYSICS"---//
                            player.balls[b].position.x += player.balls[b].speed.x; player.balls[b].position.y += player.balls[b].speed.y;

                            //diabolical if statement, it doesn't help that zed doesn't color them well
                            //oh it does check for screen bounds, its just broken, nice
                            if ( ((player.balls[b].position.x + player.balls[b].radius) >= screenWidth) || ((player.balls[b].position.x - player.balls[b].radius) <= 0 ) ){
                                player.balls[b].speed.x *= 1;
                                player.balls[b].paddleHit = false;
                                PlaySound(fxBounce);
                            }

                            if ((player.balls[b].position.y - player.balls[b].radius) <= 0){
                                player.balls[b].speed.y *= -1;
                                player.balls[b].paddleHit = false;
                                PlaySound(fxBounce);
                            }

                            //POWER-UP ICON PHYSICS//
                            for (int i = 0; i < COLLECTIBLE_POWERUP_LIMIT; i++){
                                if (memcmp(&collectibleStorage[i], &emptyCollectiblePowerUp, sizeof(CollectiblePowerup)) != 0) {
                                    collectibleStorage[i].position.y += collectibleStorage[i].speed.y * GetFrameTime();
                                    collectibleStorage[i].bounds = (Rectangle){collectibleStorage[i].position.x, collectibleStorage[i].position.y, collectibleStorage[i].powerUpIcons.width, collectibleStorage[i].powerUpIcons.height};
                                }
                            }

                            //---COLLISION---//
                            //ball<->paddle
                            if (CheckCollisionCircleRec(player.balls[b].position, player.balls[b].radius, player.bounds) && !player.balls[b].paddleHit)
                            {
                                player.balls[b].speed.y *= -1; //flip the ball's speed
                                //this feels a bit random, likely intentionally but like i can't read this
                                //Xspeed = ball position - player position's middle offset / player size * 5?
                                player.balls[b].paddleHit = true;
                                player.balls[b].speed.x = (player.balls[b].position.x - (player.position.x + (player.paddleSegments + 44.0f)/2))/(player.paddleSegments + 44)*5.0f;
                                PlaySound(fxBounce);
                            }
                            //ball<->bricks
                            for (int j = 0; j < BRICK_LINES; j++){
                                for (int i = 0; i < BRICKS_PER_LINE; i++){
                                    if (bricks[j][i].active){
                                        //the code here is kind of messy because of the way its split
                                        //the easiest way to clean would be to rout it through a function but i don't want to start bloating the file for the sake of optimization
                                        //---NEW COLLISION---//
                                        // this is not perfect, it may sometimes that you hit the side but in fact you were closer to the top collision and that acts first
                                        // a potential solution is to shrink the lines slightly, result in the ball needing to move slightly further in to get collision
                                        // another solution would be to make the ball use point collision instead of square collision, which comes with a downside of making the gameplay more annoying
                                        //if diamond collision was a thing i would use that, as it solves all those problems while providing better visual feedback
                                        //square has too large a surface area, circle is non deterministic thanks to not using pixels
                                        //meanwhile diamond has single point surface areas on all major corners, while gliding through with diagonal corners
                                        //supposably providing a better collision feel at the end
                                        //this also handles diagonal corner collision by flinging the ball sideways when it hits a diagonal corner
                                        //something which is only otherwise a thing with a proper realistic physics system
                                        //in this current instance the double hitting bug is caused by it being ifs instead of else ifs
                                        //and now its back to double hitting despite that
                                        //interestingly it doesn't seem to happen the same function call, but it does happen the same frame
                                        //it happens because the for loop just iterates the next objects after a break
                                        //no, its not double collision that's shadow collision
                                        //its probably because the circle is a shape with no points, thus its protruding outwards
                                        //and causing a funny scenario where its hitting a side with 0 x speed, and multiplying 0 by -1, which is still 0

                                        if (BallCollision(&player.balls[b], &bricks[j][i])){
                                            score += 200;
                                            if (bricks[j][i].data.resistance > 0){
                                                bricks[j][i].data.resistance--;
                                                bricks[j][i].data.isCracked = true;
                                                PlaySound(fxCrack);
                                            } else {
                                                if (bricks[j][i].data.hasPowerup){
                                                    spawnPowerUp(&bricks[j][i].data, collectibleStorage, bricks[j][i].position);
                                                }
                                                bricks[j][i].active = false;
                                            }
                                            player.balls[b].paddleHit = false;
                                            PlaySound(fxBounce);
                                            break;
                                            //goto exit_loop; //okay, it was a bug then
                                        }
                                    }
                                }
                            }
                            exit_loop:
                            //ball<->screen bounds //this is not in the tutorial, quality
                            if (CheckCollisionCircleRec(player.balls[b].position, player.balls[b].radius, (Rectangle){0,0,1, screenHeight} ) || CheckCollisionCircleRec(player.balls[b].position, player.balls[b].radius, (Rectangle){screenWidth,0,1, screenHeight}) ){
                                player.balls[b].speed.x *= -1;
                                player.balls[b].paddleHit = false;
                                PlaySound(fxBounce);
                            }


                            for (int i = 0; i < COLLECTIBLE_POWERUP_LIMIT; i++){
                                if (memcmp(&collectibleStorage[i], &emptyCollectiblePowerUp, sizeof(CollectiblePowerup)) != 0) {
                                    //paddle<->power-up
                                    if (CheckCollisionRecs(player.bounds, collectibleStorage[i].bounds)){
                                        addPowerUp(&player, &collectibleStorage[i].containedPowerup);
                                        collectibleStorage[i].containedPowerup.powerOnLoad(&player, i);
                                        memset(&collectibleStorage[i], 0, sizeof(CollectiblePowerup));
                                        PlaySound(fxScratch);
                                    }
                                    //paddle<->bounds
                                    if (collectibleStorage[i].position.y >= screenHeight){
                                        memset(&collectibleStorage[i], 0, sizeof(CollectiblePowerup));
                                    }
                                }
                            }


                            //game end conditions
                            // check if ball hit bottom of screen, if it did, remove life and reset the ball
                            if ((player.balls[b].position.y - player.balls[b].radius) >= screenHeight){
                                unsigned int ballCount = 0;
                                for (int i = 0; i < BALL_LIMIT; i++){
                                    if (player.balls[i].active) ballCount++;
                                }
                                if (ballCount > 1) {
                                    //delete the ball
                                    memset(&player.balls[b], 0, sizeof(Ball));
                                    PlaySound(fxWomp);
                                }else{
                                    //reset the ball position
                                    player.balls[b].position.x = player.position.x + (player.paddleSegments + 44.0f)/2;
                                    player.balls[b].position.y = player.position.y - player.balls[b].radius - 1.0f;
                                    player.balls[b].speed = (Vector2){0,0};
                                    player.balls[b].active = false;
                                    player.balls[b].paddleHit = false;
                                    player.lives--;
                                    PlaySound(fxExplode);
                                }
                                //if (ballCount == 0) player.lives--;
                                //reset its position
                            }
                            //zed is not making this easy, is this in the if ball.active section?
                        }
                        else{//if the ball is not active, just center on the player until the player shoots it again
                            player.balls[b].position.x  = player.position.x + (player.paddleSegments + 44.0f)/2;
                            if (IsKeyPressed(KEY_SPACE)){
                                player.balls[b].active = true;
                                player.balls[b].speed = (Vector2){0, -5.0f};
                                player.balls[b].paddleHit = false;
                                PlaySound(fxBounce);
                            }
                        }
                    }
                }

                //game completion check
                if (CheckLevelCompletion()) {
                    UnloadImage(levelData); //unload useless level
                    if(currentLevel+1 >= ARRAY_LENGTH(levelsInternal)){
                        screen = ENDING;
                        player.lives = 5;
                        PlaySound(fxStart);
                    }else {
                        currentLevel++;
                        levelData = LoadLevel(levelsInternal[currentLevel].levelResource);
                        PrepareLevel(&player);
                        PlaySound(fxStart);
                    }
                }
                else if (player.lives < 0){
                    screen = ENDING;
                    currentLevel = 0;
                    player.lives = 5;
                    PlaySound(fxExplode);
                }
            }
        }break;
        case ENDING:{
            // framesCounter++;
            if (gameStarted == true) gameStarted = false;
            if (IsKeyPressed(KEY_SPACE)) screen = TITLE;
        }break;
        default: break;
    }
    // NOTE: Music buffers must be refilled if consumed
    UpdateMusicStream(music); //so music won't play if it isn't updated, this makes sense considering raylib's structure
}
static void Draw(void){

    //---DRAW-CODE---//

    BeginDrawing();
        ClearBackground(RAYWHITE);
        //DrawFPS(10, 10);
        switch (screen) {
            case LOGO:{
                //i'm not sorry
                DrawTexture(theFunny, (screenWidth/2) - theFunny.width/2, -20, WHITE);
                DrawText("Made With:", 20, 20, 40, DARKGRAY);
                DrawTexture(texLogo, 300, 50, WHITE);
            }break;
            case TITLE:{
                DrawTextEx(font, "BRKT", (Vector2){ 180, 80 }, 160, 10, MAROON);
                DrawText("PRESS SPACE to PLAY", (GetScreenWidth()/2) - 250, 280, 40, DARKGREEN);
            }break;
            case GAMEPLAY:{
                DrawTexturePro(texPaddlePieces,
                    (Rectangle){1, 4, 22, 24},
                    (Rectangle){player.position.x, player.position.y, 22, 24},
                    (Vector2){0, 0}, 0.0f, WHITE);
                DrawTexturePro(texPaddlePieces,
                    (Rectangle){26, 4, 1, 24},
                    (Rectangle){player.position.x, player.position.y, player.paddleSegments, 24},
                    (Vector2){-22, 0}, 0.0f, WHITE); //NOTE 22 corrosponds to the length of the left piece of the paddle
                DrawTexturePro(texPaddlePieces,
                    (Rectangle){1, 4, -22, 24}, //oh, to mirror you do it on the source
                    (Rectangle){player.position.x, player.position.y, 22, 24},
                    (Vector2){-22 -player.paddleSegments, 0}, 0.0f, WHITE);
                //ball "graphics" to prove a point
                for (int i = 0; i < BALL_LIMIT; i++){
                    if (memcmp(&player.balls[i], &emptyBall, sizeof(Ball)) != 0) {//if it ain't empty
                        DrawTextureV(texBall, (Vector2){player.balls[i].position.x - player.balls[i].radius, player.balls[i].position.y - player.balls[i].radius}, WHITE);
                        // DrawCircleV(player.balls[i].position, player.balls[i].radius, (Color){150,150,150,255}); //outline
                        // DrawCircleV((Vector2){player.balls[i].position.x,player.balls[i].position.y}, player.balls[i].radius - 2, (Color){200,200,200,255}); //shadow
                        // DrawEllipseV((Vector2){player.balls[i].position.x,player.balls[i].position.y-2}, player.balls[i].radius - 3, 5, (Color){225,225,225,255}); //shading
                        // DrawCircleV((Vector2){player.balls[i].position.x+3,player.balls[i].position.y-3}, player.balls[i].radius - 7.5f, (Color){255,255,255,255}); //highlight
                    }
                }

                //and now the bricks
                for (int j = 0; j < BRICK_LINES; j++){
                    for (int i = 0; i < BRICKS_PER_LINE; i++){
                        if (bricks[j][i].active){
                            if ((i + j)%2 == 0) DrawTextureEx(bricks[j][i].data.brickTexture, bricks[j][i].position, 0.0f, 1.0f, GRAY);
                            else DrawTextureEx(bricks[j][i].data.brickTexture, bricks[j][i].position, 0.0f, 1.0f, DARKGRAY);
                            if (bricks[j][i].data.isCracked) DrawTextureEx(texCrackOverlay, bricks[j][i].position, 0.0f, 1.0f, WHITE);
                            if (bricks[j][i].data.hasPowerup) DrawTextureEx(texPowerupOverlay, bricks[j][i].position, 0.0f, 1.0f, WHITE);
                        }
                    }
                }

                //Power Up Icons
                for (int i = 0; i < COLLECTIBLE_POWERUP_LIMIT; i++){
                    if (memcmp(&collectibleStorage[i], &emptyCollectiblePowerUp, sizeof(CollectiblePowerup)) != 0)
                        DrawTextureRec(PowerupAtlas, collectibleStorage[i].drawOffset, collectibleStorage[i].position, WHITE);
                }

                //HUD
                DrawText(TextFormat("Score: %u", score), 10, 10, 30, LIGHTGRAY);
                for (int i = 0; i < player.lives; i++) DrawRectangle(20 + 40*i, screenHeight - 30, 35, 10, LIGHTGRAY);
                //draw pause message, shouldn't this be in the next step?
                if (gamePaused) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 + 60, 40, GRAY);
            }break;
            case ENDING:{
                DrawText("ENDING SCREEN", 20, 20, 40, DARKBLUE);
                DrawText("PRESS SPACE to RETURN to TITLE SCREEN", 120, 220, 20, DARKBLUE);
            }break;
            default: break;
        }
    EndDrawing();
}

_Bool BallCollision(Ball *ballinstance, Brick *brick)
{
    //alright this needs a rework, because it should reflect where the ball hit, instead of oh it touched a line
    //if its going down it should go up, if its going up it should go down, if its going left it should go right
    //so according to my searches, since line checking is just completely wrong and can and will generate incorrect results, especially with a circle
    //i'm going for plan B
    //distance from brick's center
    // i want this to be noted as this would be impossible on the systems that were actually used to develop the original breakout arcade machine
    // or even popular clones from the time like arkanoid and DX-Ball
    // not only is circle collision a figment of modern times
    // but this is built on the factor of devs of the time being good mathematicians, which even today isn't the case
    // needless to say, i don't like this method of collision, even if its technically correct
    // half the problems come from the circle
    // problems that i would fix if i wasn't into a project hastly put together in the span of a weak while wrestling with a langauge i've never used for a serious project before
    if(CheckCollisionCircleRec(ballinstance->position, ballinstance->radius, brick->bounds)){
        //clamp the ball's position between the bounds of the hit brick, otherwise numbers would be too high
        float closestX = Clamp(ballinstance->position.x, brick->bounds.x, brick->bounds.x + brick->bounds.width);
        float closestY = Clamp(ballinstance->position.y, brick->bounds.y, brick->bounds.y + brick->bounds.height);
        //calculate distance
        float dx = ballinstance->position.x - closestX;
        float dy = ballinstance->position.y - closestY;

        if (fabsf(dx) > fabsf(dy)) ballinstance->speed.x *= -1;
        else ballinstance->speed.y *= -1;
        return true;
    }
    return false;
}

_Bool CheckLevelCompletion(void){
    for (int j = 0; j < BRICK_LINES; j++)
        for (int i = 0; i < BRICKS_PER_LINE; i++)
            if (bricks[j][i].active) return false;
    return true;
}
