#include "raylib.h"      // Raylib graphics library
#include <math.h>        // For sqrtf, fabs, atan2 etc.
#include <stdio.h>       // For sprintf
#include <stdlib.h>
#include <string.h>      // For strcpy
#include <stdbool.h>     // For bool type

// ---------------------- CONSTANT DEFINITIONS ----------------------

#define MAX_BALLS 16              // Total balls including cue ball
#define TABLE_WIDTH 800           // Table width
#define TABLE_HEIGHT 400          // Table height
#define BALL_RADIUS 15            // Radius of each ball
#define POCKET_RADIUS 28          // Radius of pocket
#define RAIL_WIDTH 40             // Thickness of table rail

// Physics tuning parameters
#define FRICTION 0.985f           // Friction multiplier per frame
#define MIN_VELOCITY 0.06f        // Minimum velocity threshold to stop ball
#define MAX_POWER_PIXELS 160.0f   // Maximum drag distance for power
#define MAX_SHOT_SPEED 22.0f      // Maximum initial shot speed
#define MAX_BALL_SPEED 26.0f      // Maximum speed any ball can have

// ---------------------- ENUM TYPES ----------------------

// Ball type classification
typedef enum {
    BALL_CUE,      // White cue ball
    BALL_SOLID,    // Solid balls (1-7)
    BALL_STRIPE,   // Stripe balls (9-15)
    BALL_EIGHT     // Black 8-ball
} BallType;

// Game state tracking
typedef enum {
    GAME_START,
    GAME_PLAYING,
    GAME_SCRATCH,
    GAME_WON,
    GAME_LOST
} GameState;

// Player assigned type
typedef enum {
    PLAYER_NONE,
    PLAYER_SOLIDS,
    PLAYER_STRIPES
} PlayerType;

// ---------------------- STRUCT DEFINITIONS ----------------------

// Ball structure
typedef struct {
    Vector2 position;     // Current position
    Vector2 velocity;     // Current velocity
    Color color;          // Ball color
    BallType type;        // Ball type
    int number;           // Ball number
    bool pocketed;        // Is ball inside pocket
    bool isStriped;       // Stripe flag
} Ball;

// Player structure
typedef struct {
    PlayerType type;      // Assigned type
    int ballsRemaining;   // Balls left to clear
    char name[20];        // Player name
} Player;

// Main Game structure
typedef struct {
    Ball balls[MAX_BALLS];        // All balls
    Player players[2];            // Two players
    int currentPlayer;            // Whose turn
    GameState state;              // Current state

    Vector2 cueBallPos;           // Cue ball respawn position
    float power;                  // Current shot power
    bool aiming;                  // Is player dragging
    bool ballsMoving;             // Are balls in motion
    bool firstShot;               // Break shot flag
    bool assignedTypes;           // Are solids/stripes assigned

    char statusMessage[100];      // UI message

    // Cue stick mechanics
    Vector2 dragStart;            
    float stickPullPixels;        
    float stickLength;            
    bool stickRecoil;
    float recoilTimer;
} Game;

// ---------------------- FUNCTION PROTOTYPES ----------------------

void InitGame(Game *game);
void ResetBalls(Game *game);
void UpdateGame(Game *game);
void DrawGame(Game *game);
void HandleInput(Game *game);
void UpdatePhysics(Game *game);
void CheckCollisions(Game *game);
void CheckPockets(Game *game);
void CheckWinCondition(Game *game);
void NextTurn(Game *game);
void ApplyScratch(Game *game);
void DrawPowerBar(Game *game);
void DrawTable();
bool AreBallsMoving(Game *game);
float Distance(Vector2 a, Vector2 b);
int playerIndexForType(Game *game, BallType btype);
void ResolveElasticCollision(Ball *a, Ball *b);
void ClampBallSpeed(Ball *b, float maxSpeed);

// ---------------------- GAME INITIALIZATION ----------------------

void InitGame(Game *game) {

    // Initialize player names and starting state
    strcpy(game->players[0].name, "Player 1");
    game->players[0].type = PLAYER_NONE;
    game->players[0].ballsRemaining = 7;

    strcpy(game->players[1].name, "Player 2");
    game->players[1].type = PLAYER_NONE;
    game->players[1].ballsRemaining = 7;

    game->currentPlayer = 0;
    game->state = GAME_START;

    game->power = 0.0f;
    game->aiming = false;
    game->ballsMoving = false;
    game->firstShot = true;
    game->assignedTypes = false;

    strcpy(game->statusMessage, 
        "Break shot: click on cue, drag back, release to shoot");

    // Cue stick initial settings
    game->stickPullPixels = 0.0f;
    game->stickLength = 120.0f;
    game->stickRecoil = false;
    game->recoilTimer = 0.0f;

    // Arrange balls
    ResetBalls(game);
}

// ---------------------- BALL SETUP ----------------------

void ResetBalls(Game *game) {

    // Triangle rack starting position
    Vector2 triangleStart = { TABLE_WIDTH * 0.72f, TABLE_HEIGHT * 0.5f };

    // Setup cue ball
    game->balls[0].position = 
        (Vector2){ TABLE_WIDTH * 0.25f, TABLE_HEIGHT * 0.5f };
    game->balls[0].velocity = (Vector2){0, 0};
    game->balls[0].color = WHITE;
    game->balls[0].type = BALL_CUE;
    game->balls[0].number = 0;
    game->balls[0].pocketed = false;
    game->balls[0].isStriped = false;

    // Define colors
    Color solidColors[] = { YELLOW, BLUE, RED, PURPLE, 
                            ORANGE, GREEN, MAROON };
    Color stripeColors[] = { YELLOW, BLUE, RED, PURPLE, 
                             ORANGE, GREEN, MAROON };

    int idx = 1;

    // Create triangle formation
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col <= row; col++) {

            if (idx >= MAX_BALLS) break;

            float offsetX = row * (BALL_RADIUS * 2 * 0.88f);
            float offsetY = (col * (BALL_RADIUS * 2)) 
                            - (row * BALL_RADIUS);

            game->balls[idx].position = 
                (Vector2){ triangleStart.x + offsetX,
                           triangleStart.y + offsetY };

            game->balls[idx].velocity = (Vector2){0,0};
            game->balls[idx].pocketed = false;

            // Assign ball types
            if (idx == 8) {
                game->balls[idx].color = BLACK;
                game->balls[idx].type = BALL_EIGHT;
                game->balls[idx].isStriped = false;
            }
            else if (idx <= 7) {
                game->balls[idx].color = solidColors[idx-1];
                game->balls[idx].type = BALL_SOLID;
                game->balls[idx].isStriped = false;
            }
            else {
                int sidx = idx - 9;
                if (sidx < 0) sidx = 0;
                game->balls[idx].color = stripeColors[sidx];
                game->balls[idx].type = BALL_STRIPE;
                game->balls[idx].isStriped = true;
            }
            game->balls[idx].number = idx;
            idx++;
        }
    }

    game->cueBallPos = game->balls[0].position;
}

// ---------------------- PHYSICS UPDATE ----------------------

void UpdatePhysics(Game *game) {

    for (int i = 0; i < MAX_BALLS; i++) {

        if (game->balls[i].pocketed) continue;

        // Update position
        game->balls[i].position.x += game->balls[i].velocity.x;
        game->balls[i].position.y += game->balls[i].velocity.y;

        // Apply friction
        game->balls[i].velocity.x *= FRICTION;
        game->balls[i].velocity.y *= FRICTION;

        // Stop tiny velocities
        if (fabs(game->balls[i].velocity.x) < MIN_VELOCITY)
            game->balls[i].velocity.x = 0;
        if (fabs(game->balls[i].velocity.y) < MIN_VELOCITY)
            game->balls[i].velocity.y = 0;

        // Rail collision (bounce effect)
        if (game->balls[i].position.x - BALL_RADIUS < RAIL_WIDTH) {
            game->balls[i].position.x = RAIL_WIDTH + BALL_RADIUS;
            game->balls[i].velocity.x *= -0.86f;
        }
        if (game->balls[i].position.x + BALL_RADIUS >
            TABLE_WIDTH - RAIL_WIDTH) {
            game->balls[i].position.x =
                TABLE_WIDTH - RAIL_WIDTH - BALL_RADIUS;
            game->balls[i].velocity.x *= -0.86f;
        }
        if (game->balls[i].position.y - BALL_RADIUS < RAIL_WIDTH) {
            game->balls[i].position.y = RAIL_WIDTH + BALL_RADIUS;
            game->balls[i].velocity.y *= -0.86f;
        }
        if (game->balls[i].position.y + BALL_RADIUS >
            TABLE_HEIGHT - RAIL_WIDTH) {
            game->balls[i].position.y =
                TABLE_HEIGHT - RAIL_WIDTH - BALL_RADIUS;
            game->balls[i].velocity.y *= -0.86f;
        }

        // Limit maximum speed
        ClampBallSpeed(&game->balls[i], MAX_BALL_SPEED);
    }

    // Ball-to-ball collision
    CheckCollisions(game);

    // Check pocketing
    CheckPockets(game);
}

// ---------------------- MAIN FUNCTION ----------------------

int main(void) {

    // Create game window

    InitWindow(TABLE_WIDTH, TABLE_HEIGHT + 100,"8 Ball Pool - Drag to Charge (Fixed)");
    SetTargetFPS(60);
    Game game;
    InitGame(&game);

    // Main game loop

    while (!WindowShouldClose()) {
        UpdateGame(&game);   // Update logic
        DrawGame(&game);     // Draw everything
    }
    CloseWindow();
    return 0;
}

void UpdateGame(Game *game) {

    // Handle keyboard & mouse input
    HandleInput(game);

    // Handle cue stick recoil animation after shot
    if (game->stickRecoil) {

        // Reduce recoil timer per frame (60 FPS assumption)
        game->recoilTimer -= 1.0f/60.0f;

        // When recoil ends, reset stick pull
        if (game->recoilTimer <= 0.0f) {
            game->stickRecoil = false;
            game->stickPullPixels = 0.0f;
        } 
        else {
            // Gradually reduce pull distance visually
            game->stickPullPixels *= 0.92f;

            // Update power based on pull distance
            game->power = game->stickPullPixels / MAX_POWER_PIXELS;
            if (game->power < 0) game->power = 0;
        }
    }

    // Only update physics during play or scratch state
    if (game->state == GAME_PLAYING || 
        game->state == GAME_SCRATCH) {
        UpdatePhysics(game);

        // Detect start of ball movement
        if (!game->ballsMoving && 
            AreBallsMoving(game)) 
            game->ballsMoving = true;

        // Detect stop of all balls
        if (game->ballsMoving && 
            !AreBallsMoving(game)) {
            game->ballsMoving = false;

            // When balls stop
            if (game->state == GAME_PLAYING) {
                CheckWinCondition(game);
                if (game->state != GAME_WON &&
                    game->state != GAME_LOST) {
                    NextTurn(game);
                }
            }
        }
    }
}

void HandleInput(Game *game) {

    // Restart game anytime by pressing R
    if (IsKeyPressed(KEY_R)) {
        InitGame(game);
        return;
    }

    Vector2 mousePos = GetMousePosition();

    // -------- SCRATCH MODE --------
    if (game->state == GAME_SCRATCH) {

        // Player can place cue ball inside valid area

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (mousePos.x > RAIL_WIDTH + BALL_RADIUS &&
                mousePos.x < TABLE_WIDTH - RAIL_WIDTH - BALL_RADIUS &&
                mousePos.y > RAIL_WIDTH + BALL_RADIUS &&
                mousePos.y < TABLE_HEIGHT - RAIL_WIDTH - BALL_RADIUS) {
                game->cueBallPos = mousePos;
                game->balls[0].position = game->cueBallPos;
                game->balls[0].pocketed = false;
                game->balls[0].velocity = (Vector2){0,0};
                game->state = GAME_PLAYING;
                sprintf(game->statusMessage, "Cue placed. %s's turn",game->players[game->currentPlayer].name);
            }
            else {
                strcpy(game->statusMessage, "Invalid position! Place inside rails");
            }
        }
        return;
    }

    // Ignore input if balls are moving
    if (game->ballsMoving) return;
    Vector2 cueBallPos = 
        game->balls[0].pocketed ?
        game->cueBallPos :
        game->balls[0].position;

    // Start drag if clicking near cue ball
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (Distance(mousePos, cueBallPos) <= BALL_RADIUS*1.6f) {
            game->aiming = true;
            game->dragStart = mousePos;
            game->stickPullPixels = 0.0f;
            game->power = 0.0f;
        }
    }

    // While dragging mouse

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && 
        game->aiming) {
        float d = Distance(mousePos, cueBallPos);
        if (d > MAX_POWER_PIXELS)
            d = MAX_POWER_PIXELS;
        game->stickPullPixels = d;
        game->power = d / MAX_POWER_PIXELS;
    }

    // Release mouse → shoot

    if (game->aiming &&
        IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        game->aiming = false;
        Vector2 dir = {
            mousePos.x - cueBallPos.x,
            mousePos.y - cueBallPos.y
        };
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
        if (len < 0.001f) return;
        dir.x /= len;
        dir.y /= len;
        float shotSpeed =
            (game->stickPullPixels / MAX_POWER_PIXELS)
            * MAX_SHOT_SPEED;
        if (shotSpeed > MAX_SHOT_SPEED)
            shotSpeed = MAX_SHOT_SPEED;

        // Apply velocity to cue ball

        game->balls[0].velocity.x =
            dir.x * shotSpeed;
        game->balls[0].velocity.y =
            dir.y * shotSpeed;
        game->state = GAME_PLAYING;
        game->firstShot = false;

        // Start recoil animation

        game->stickRecoil = true;
        game->recoilTimer = 0.12f;

        game->power = 0.0f;
    }
}

void CheckCollisions(Game *game) {

    // Compare each ball with every other ball
    for (int i = 0; i < MAX_BALLS; i++) {
        if (game->balls[i].pocketed) continue;
        for (int j = i+1; j < MAX_BALLS; j++) {
            if (game->balls[j].pocketed) continue;

            // Calculate distance between ball centers
            float dist = Distance(
                game->balls[i].position,
                game->balls[j].position);
            float minDist = BALL_RADIUS * 2.0f;

            // If balls overlap → collision occurred
            if (dist < minDist && dist > 0.0001f) {

                // Calculate overlap amount
                float overlap = 0.5f * (minDist - dist + 0.001f);

                // Normal direction between balls
                Vector2 normal = {
                    (game->balls[j].position.x -
                     game->balls[i].position.x) / dist,
                    (game->balls[j].position.y -
                     game->balls[i].position.y) / dist
                };

                // Push balls apart equally
                game->balls[i].position.x -= normal.x * overlap;
                game->balls[i].position.y -= normal.y * overlap;
                game->balls[j].position.x += normal.x * overlap;
                game->balls[j].position.y += normal.y * overlap;

                // Apply elastic collision physics
                ResolveElasticCollision(
                    &game->balls[i],
                    &game->balls[j]);

                // Clamp speeds to avoid unrealistic speed
                ClampBallSpeed(&game->balls[i],
                               MAX_BALL_SPEED);
                ClampBallSpeed(&game->balls[j],
                               MAX_BALL_SPEED);
            }
        }
    }
}

void ResolveElasticCollision(Ball *a, Ball *b) {

    // Compute normal vector
    float dx = b->position.x - a->position.x;
    float dy = b->position.y - a->position.y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist <= 0.0001f) return;
    float nx = dx / dist;
    float ny = dy / dist;

    // Tangent vector
    float tx = -ny;
    float ty = nx;

    // Project velocities onto normal and tangent

    float va_n = a->velocity.x * nx +
                 a->velocity.y * ny;
    float va_t = a->velocity.x * tx +
                 a->velocity.y * ty;
    float vb_n = b->velocity.x * nx +
                 b->velocity.y * ny;
    float vb_t = b->velocity.x * tx +
                 b->velocity.y * ty;

    // Equal mass elastic collision:
    // Swap normal components

    float va_n_after = vb_n;
    float vb_n_after = va_n;

    // Convert back to vector form

    a->velocity.x = va_n_after * nx +
                    va_t * tx;
    a->velocity.y = va_n_after * ny +
                    va_t * ty;
    b->velocity.x = vb_n_after * nx +
                    vb_t * tx;
    b->velocity.y = vb_n_after * ny +
                    vb_t * ty;
}

void CheckPockets(Game *game) {

    // Define 6 pocket positions

    Vector2 pockets[] = {
        {RAIL_WIDTH, RAIL_WIDTH},
        {TABLE_WIDTH*0.5f, RAIL_WIDTH},
        {TABLE_WIDTH - RAIL_WIDTH, RAIL_WIDTH},
        {RAIL_WIDTH, TABLE_HEIGHT - RAIL_WIDTH},
        {TABLE_WIDTH*0.5f, TABLE_HEIGHT - RAIL_WIDTH},
        {TABLE_WIDTH - RAIL_WIDTH, TABLE_HEIGHT - RAIL_WIDTH}
    };
    bool cueBallPocketed = false;
    bool anyPocketed = false;
    for (int i = 0; i < MAX_BALLS; i++) {
        if (game->balls[i].pocketed) continue;
        for (int p = 0; p < 6; p++) {

            // If ball center inside pocket radius

            if (Distance(game->balls[i].position,
                         pockets[p]) < POCKET_RADIUS) {
                game->balls[i].pocketed = true;
                game->balls[i].velocity = (Vector2){0,0};
                anyPocketed = true;
                // Cue ball scratch

                if (i == 0) {
                    cueBallPocketed = true;
                    game->cueBallPos = (Vector2){ TABLE_WIDTH * 0.25f, TABLE_HEIGHT * 0.5f };
                }
                else {
                    // 8-ball logic
                    if (game->balls[i].type == BALL_EIGHT) {
                        int myIdx = game->currentPlayer;
                        if ((game->players[myIdx].type
                             == PLAYER_SOLIDS &&
                             game->players[myIdx].ballsRemaining == 0)
                            ||
                            (game->players[myIdx].type
                             == PLAYER_STRIPES &&
                             game->players[myIdx].ballsRemaining == 0)) {

                            game->state = GAME_WON;
                        }
                        else {
                            game->state = GAME_LOST;
                        }
                        return;
                    }
                }
                break;
            }
        }
    }

    if (cueBallPocketed)
        ApplyScratch(game);

    if (anyPocketed) {
        sprintf(game->statusMessage,
            "%s pocketed a ball!",
            game->players[game->currentPlayer].name);
    }
}

void ApplyScratch(Game *game) {
    game->state = GAME_SCRATCH;
    strcpy(game->statusMessage,
           "Scratch! Place cue ball");

    // Switch turn to opponent
    game->currentPlayer =
        1 - game->currentPlayer;
}

void CheckWinCondition(Game *game) {
    int idx = game->currentPlayer;
    if ((game->players[idx].type ==
         PLAYER_SOLIDS &&
         game->players[idx].ballsRemaining == 0)
        ||
        (game->players[idx].type ==
         PLAYER_STRIPES &&
         game->players[idx].ballsRemaining == 0)) {
        strcpy(game->statusMessage,"Shoot the 8-ball!");
    }
}

void NextTurn(Game *game) {

    // Switch player
    game->currentPlayer =
        1 - game->currentPlayer;
    sprintf(game->statusMessage,
        "%s's turn",game->players[game->currentPlayer].name);
}

bool AreBallsMoving(Game *game) {
    for (int i = 0; i < MAX_BALLS; i++) {
        if (game->balls[i].pocketed)
            continue;
        if (fabs(game->balls[i].velocity.x)
            > MIN_VELOCITY ||
            fabs(game->balls[i].velocity.y)
            > MIN_VELOCITY)
            return true;
    }
    return false;
}

float Distance(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

void ClampBallSpeed(Ball *b,float maxSpeed) {
    float sx = b->velocity.x;
    float sy = b->velocity.y;
    float mag = sqrtf(sx*sx + sy*sy);
    if (mag > maxSpeed) {
        b->velocity.x =
            (b->velocity.x / mag)* maxSpeed;
        b->velocity.y =(b->velocity.y / mag) * maxSpeed;
    }
}

void DrawTable() {
    BeginDrawing();
    ClearBackground(DARKGREEN);

    // Draw wooden outer border
    DrawRectangle(0, 0, TABLE_WIDTH, TABLE_HEIGHT, BROWN);

    // Draw inner table (playing surface)
    DrawRectangle(RAIL_WIDTH, RAIL_WIDTH,
                  TABLE_WIDTH - 2*RAIL_WIDTH,
                  TABLE_HEIGHT - 2*RAIL_WIDTH,
                  DARKGREEN);

    // Draw pockets (6 total)
    Vector2 pockets[] = {
        {RAIL_WIDTH, RAIL_WIDTH},
        {TABLE_WIDTH*0.5f, RAIL_WIDTH},
        {TABLE_WIDTH - RAIL_WIDTH, RAIL_WIDTH},
        {RAIL_WIDTH, TABLE_HEIGHT - RAIL_WIDTH},
        {TABLE_WIDTH*0.5f, TABLE_HEIGHT - RAIL_WIDTH},
        {TABLE_WIDTH - RAIL_WIDTH, TABLE_HEIGHT - RAIL_WIDTH}
    };

    for (int i = 0; i < 6; i++) {
        DrawCircleV(pockets[i], POCKET_RADIUS, BLACK);
    }
}
//------------------------ Draws the power bar UI showing the current shot power-------------------

void DrawPowerBar(Game *game) {

    float barWidth = 300;
    float barHeight = 20;
    float x = (TABLE_WIDTH - barWidth) / 2;
    float y = TABLE_HEIGHT + 40;

    // Outline
    DrawRectangleLines(x, y, barWidth, barHeight, WHITE);

    // Fill based on power

    DrawRectangle(x, y,
                  barWidth * game->power,
                  barHeight,
                  RED);
    DrawText("Power", x - 60, y,20,  WHITE);
}

//-----------------Draws the entire game scene: table, balls, aiming line, UI, status, and power bar-------------

void DrawGame(Game *game) {
    DrawTable();

    // Draw balls
    for (int i = 0; i < MAX_BALLS; i++) {
        if (game->balls[i].pocketed)
            continue;

        // Draw ball body
        DrawCircleV(game->balls[i].position,
                    BALL_RADIUS,
                    game->balls[i].color);

        // Draw stripe if striped ball
        if (game->balls[i].isStriped) {
            DrawCircleLines(
                game->balls[i].position.x,
                game->balls[i].position.y,
                BALL_RADIUS,
                WHITE);
        }

        // Draw ball number (except cue ball)

        if (game->balls[i].number != 0) {
            char num[3];
            sprintf(num, "%d",
                    game->balls[i].number);
            DrawText(num,
                game->balls[i].position.x - 6,
                game->balls[i].position.y - 6,
                12,
                WHITE);
        }
    }

    // Draw aiming line
    if (game->aiming && !game->ballsMoving) {
        Vector2 cuePos = game->balls[0].position;
        Vector2 mouse = GetMousePosition();
        DrawLineV(cuePos, mouse, WHITE);
    }

    // Draw UI area background
    DrawRectangle(0, TABLE_HEIGHT,
                  TABLE_WIDTH, 100,
                  DARKGRAY);

    // Draw status message
    DrawText(game->statusMessage,
             20,
             TABLE_HEIGHT + 10,
             20,
             WHITE);

    // Draw power bar
    DrawPowerBar(game);

    EndDrawing();
}

//----------------- Returns the index of the player assigned to the given ball type------------
int playerIndexForType(Game *game, BallType btype) {
    if (btype == BALL_SOLID) {
        if (game->players[0].type == PLAYER_SOLIDS)
            return 0;
        if (game->players[1].type == PLAYER_SOLIDS)
            return 1;
    }
    if (btype == BALL_STRIPE) {
        if (game->players[0].type == PLAYER_STRIPES)
            return 0;
        if (game->players[1].type == PLAYER_STRIPES)
            return 1;
    }
    return -1;  // Not assigned
}