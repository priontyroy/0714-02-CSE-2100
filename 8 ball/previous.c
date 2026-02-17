#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_BALLS 16
#define TBL_W 800
#define TBL_H 400
#define BALL_R 15
#define POCK_R 28
#define RAIL_W 40

#define FRICTION 0.985f
#define MIN_SPEED 0.06f
#define MAX_PULL 160.0f
#define MAX_SHOT 22.0f
#define MAX_BALL_SPEED 26.0f

typedef enum { CUE, SOLID, STRIPE, EIGHT } BallType;
typedef enum { START, PLAY, SCRATCH, WON, LOST } GameState;
typedef enum { NONE, SOL, STR } PlayerType;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    BallType type;
    int num;
    bool pocketed;
    bool striped;
} Ball;

typedef struct {
    PlayerType type;
    int remain;
    char name[20];
} Player;

typedef struct {
    Ball tblBalls[NUM_BALLS];
    Player players[2];
    int curPlayer;
    GameState state;
    Vector2 cuePos;
    float power;
    bool aiming;
    bool ballsMoving;
    bool firstShot;
    bool assignedType;
    char status[100];

    Vector2 dragStart;
    float pullDist;
    float stickLen;
    bool stickRecoiling;
    float recoilTimer;
} Game;

// ---- Prototypes ----
void initGame(Game *g);
void resetBalls(Game *g);
void updGame(Game *g);
void drawGame(Game *g);
void handleInput(Game *g);
void updPhysics(Game *g);
void chkCollisions(Game *g);
void chkPockets(Game *g);
void chkWin(Game *g);
void nextTurn(Game *g);
void applyScratch(Game *g);
void drawPower(Game *g);
void drawTable();
bool ballsAreMoving(Game *g);
float dist(Vector2 a, Vector2 b);
int playerIdxForType(Game *g, BallType t);
void resolveElastic(Ball *a, Ball *b);
void clampSpeed(Ball *b, float maxSpeed);

// ---------------- Implementations ----------------
void initGame(Game *g) {
    strcpy(g->players[0].name, "Player1");
    g->players[0].type = NONE;
    g->players[0].remain = 7;
    strcpy(g->players[1].name, "Player2");
    g->players[1].type = NONE;
    g->players[1].remain = 7;

    g->curPlayer = 0;
    g->state = START;
    g->power = 0;
    g->aiming = false;
    g->ballsMoving = false;
    g->firstShot = true;
    g->assignedType = false;
    strcpy(g->status, "Break shot: click cue, drag back, release to shoot");

    g->pullDist = 0.0f;
    g->stickLen = 120.0f;
    g->stickRecoiling = false;
    g->recoilTimer = 0.0f;

    resetBalls(g);
}

void resetBalls(Game *g) {
    Vector2 triStart = { TBL_W*0.72f, TBL_H*0.5f };

    // cue ball index 0
    g->tblBalls[0].pos = (Vector2){ TBL_W*0.25f, TBL_H*0.5f };
    g->tblBalls[0].vel = (Vector2){0,0};
    g->tblBalls[0].color = WHITE;
    g->tblBalls[0].type = CUE;
    g->tblBalls[0].num = 0;
    g->tblBalls[0].pocketed = false;
    g->tblBalls[0].striped = false;

    Color solidCols[] = { YELLOW, BLUE, RED, PURPLE, ORANGE, GREEN, MAROON };
    Color stripeCols[] = { YELLOW, BLUE, RED, PURPLE, ORANGE, GREEN, MAROON };

    int idx = 1;
    for (int row=0; row<5; row++){
        for(int col=0; col<=row; col++){
            if(idx>=NUM_BALLS) break;
            float offX = row*(BALL_R*2*0.88f);
            float offY = (col*(BALL_R*2)) - (row*BALL_R);
            g->tblBalls[idx].pos = (Vector2){ triStart.x+offX, triStart.y+offY };
            g->tblBalls[idx].vel = (Vector2){0,0};
            g->tblBalls[idx].pocketed = false;

            if(idx==8){
                g->tblBalls[idx].color = BLACK;
                g->tblBalls[idx].type = EIGHT;
                g->tblBalls[idx].striped = false;
            } else if(idx<=7){
                g->tblBalls[idx].color = solidCols[idx-1];
                g->tblBalls[idx].type = SOLID;
                g->tblBalls[idx].striped = false;
            } else{
                int sidx = idx-9; if(sidx<0)sidx=0;
                g->tblBalls[idx].color = stripeCols[sidx];
                g->tblBalls[idx].type = STRIPE;
                g->tblBalls[idx].striped = true;
            }
            g->tblBalls[idx].num = idx;
            idx++;
        }
    }

    g->cuePos = g->tblBalls[0].pos;
}

float dist(Vector2 a, Vector2 b){
    float dx = a.x-b.x;
    float dy = a.y-b.y;
    return sqrtf(dx*dx+dy*dy);
}

bool ballsAreMoving(Game *g){
    for(int i=0;i<NUM_BALLS;i++){
        if(g->tblBalls[i].pocketed) continue;
        if(fabs(g->tblBalls[i].vel.x)>MIN_SPEED || fabs(g->tblBalls[i].vel.y)>MIN_SPEED) return true;
    }
    return false;
}

int playerIdxForType(Game *g, BallType t){
    if(t==SOLID){
        if(g->players[0].type==SOL) return 0;
        if(g->players[1].type==SOL) return 1;
    } else if(t==STRIPE){
        if(g->players[0].type==STR) return 0;
        if(g->players[1].type==STR) return 1;
    }
    return -1;
}

void clampSpeed(Ball *b, float maxSpeed){
    float mag = sqrtf(b->vel.x*b->vel.x + b->vel.y*b->vel.y);
    if(mag>maxSpeed){
        b->vel.x = (b->vel.x/mag)*maxSpeed;
        b->vel.y = (b->vel.y/mag)*maxSpeed;
    }
}

void resolveElastic(Ball *a, Ball *b){
    float dx = b->pos.x-a->pos.x;
    float dy = b->pos.y-a->pos.y;
    float d = sqrtf(dx*dx+dy*dy);
    if(d<=0.0001f) return;
    float nx=dx/d, ny=dy/d;
    float tx=-ny, ty=nx;

    float va_n = a->vel.x*nx + a->vel.y*ny;
    float va_t = a->vel.x*tx + a->vel.y*ty;
    float vb_n = b->vel.x*nx + b->vel.y*ny;
    float vb_t = b->vel.x*tx + b->vel.y*ty;

    float va_n2 = vb_n;
    float vb_n2 = va_n;

    a->vel.x = va_n2*nx + va_t*tx;
    a->vel.y = va_n2*ny + va_t*ty;
    b->vel.x = vb_n2*nx + vb_t*tx;
    b->vel.y = vb_n2*ny + vb_t*ty;
}

void chkCollisions(Game *g){
    for(int i=0;i<NUM_BALLS;i++){
        if(g->tblBalls[i].pocketed) continue;
        for(int j=i+1;j<NUM_BALLS;j++){
            if(g->tblBalls[j].pocketed) continue;
            float d = dist(g->tblBalls[i].pos, g->tblBalls[j].pos);
            float minD = BALL_R*2.0f;
            if(d<minD && d>0.0001f){
                float overlap = 0.5f*(minD-d+0.001f);
                Vector2 norm = {(g->tblBalls[j].pos.x-g->tblBalls[i].pos.x)/d,
                                 (g->tblBalls[j].pos.y-g->tblBalls[i].pos.y)/d};
                g->tblBalls[i].pos.x -= norm.x*overlap;
                g->tblBalls[i].pos.y -= norm.y*overlap;
                g->tblBalls[j].pos.x += norm.x*overlap;
                g->tblBalls[j].pos.y += norm.y*overlap;
                resolveElastic(&g->tblBalls[i], &g->tblBalls[j]);
                clampSpeed(&g->tblBalls[i], MAX_BALL_SPEED);
                clampSpeed(&g->tblBalls[j], MAX_BALL_SPEED);
            }
        }
    }
}
void chkPockets(Game *g){
    Vector2 pockets[] = {
        {RAIL_W, RAIL_W},
        {TBL_W*0.5f, RAIL_W},
        {TBL_W-RAIL_W, RAIL_W},
        {RAIL_W, TBL_H-RAIL_W},
        {TBL_W*0.5f, TBL_H-RAIL_W},
        {TBL_W-RAIL_W, TBL_H-RAIL_W}
    };
    bool cuePocketed=false, anyPocket=false;
    for(int i=0;i<NUM_BALLS;i++){
        if(g->tblBalls[i].pocketed) continue;
        for(int p=0;p<6;p++){
            if(dist(g->tblBalls[i].pos, pockets[p])<POCK_R){
                g->tblBalls[i].pocketed=true;
                g->tblBalls[i].vel=(Vector2){0,0};
                anyPocket=true;
                if(i==0){
                    cuePocketed=true;
                    g->cuePos = (Vector2){TBL_W*0.25f, TBL_H*0.5f};
                } else{
                    if(!g->assignedType && g->firstShot){
                        if(g->tblBalls[i].type==SOLID){
                            g->players[g->curPlayer].type=SOL;
                            g->players[1-g->curPlayer].type=STR;
                        } else{
                            g->players[g->curPlayer].type=STR;
                            g->players[1-g->curPlayer].type=SOL;
                        }
                        g->assignedType=true;
                    }
                    if(g->tblBalls[i].type==EIGHT){
                        int idx=g->curPlayer;
                        if((g->players[idx].type==SOL && g->players[idx].remain==0) ||
                           (g->players[idx].type==STR && g->players[idx].remain==0))
                            g->state=WON;
                        else g->state=LOST;
                        return;
                    } else{
                        int owner = playerIdxForType(g, g->tblBalls[i].type);
                        if(owner>=0 && g->players[owner].remain>0) g->players[owner].remain--;
                    }
                }
                break;
            }
        }
    }
    if(cuePocketed) applyScratch(g);
    if(anyPocket) sprintf(g->status, "%s pocketed a ball!", g->players[g->curPlayer].name);
}
void applyScratch(Game *g){
    g->state = SCRATCH;
    strcpy(g->status, "Scratch! Place cue ball");
    g->curPlayer = 1-g->curPlayer;
}
void chkWin(Game *g){
    int idx=g->curPlayer;
    if((g->players[idx].type==SOL && g->players[idx].remain==0) ||
       (g->players[idx].type==STR && g->players[idx].remain==0))
       strcpy(g->status, "Shoot the 8-ball!");
}
void nextTurn(Game *g){
    g->curPlayer=1-g->curPlayer;
    sprintf(g->status, "%s's turn", g->players[g->curPlayer].name);
}
void updPhysics(Game *g){
    for(int i=0;i<NUM_BALLS;i++){
        if(g->tblBalls[i].pocketed) continue;
        g->tblBalls[i].pos.x += g->tblBalls[i].vel.x;
        g->tblBalls[i].pos.y += g->tblBalls[i].vel.y;
        g->tblBalls[i].vel.x *= FRICTION;
        g->tblBalls[i].vel.y *= FRICTION;
        if(fabs(g->tblBalls[i].vel.x)<MIN_SPEED) g->tblBalls[i].vel.x=0;
        if(fabs(g->tblBalls[i].vel.y)<MIN_SPEED) g->tblBalls[i].vel.y=0;

        if(g->tblBalls[i].pos.x-BALL_R<RAIL_W){ g->tblBalls[i].pos.x=RAIL_W+BALL_R; g->tblBalls[i].vel.x*=-0.86f; }
        if(g->tblBalls[i].pos.x+BALL_R>TBL_W-RAIL_W){ g->tblBalls[i].pos.x=TBL_W-RAIL_W-BALL_R; g->tblBalls[i].vel.x*=-0.86f; }
        if(g->tblBalls[i].pos.y-BALL_R<RAIL_W){ g->tblBalls[i].pos.y=RAIL_W+BALL_R; g->tblBalls[i].vel.y*=-0.86f; }
        if(g->tblBalls[i].pos.y+BALL_R>TBL_H-RAIL_W){ g->tblBalls[i].pos.y=TBL_H-RAIL_W-BALL_R; g->tblBalls[i].vel.y*=-0.86f; }

        clampSpeed(&g->tblBalls[i], MAX_BALL_SPEED);
    }
    chkCollisions(g);
    chkPockets(g);
}
void handleInput(Game *g){
    if(IsKeyPressed(KEY_R)){ initGame(g); return; }
    Vector2 mouse=GetMousePosition();
    if(g->state==SCRATCH){
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(mouse.x>RAIL_W+BALL_R && mouse.x<TBL_W-RAIL_W-BALL_R &&
               mouse.y>RAIL_W+BALL_R && mouse.y<TBL_H-RAIL_W-BALL_R){
                g->cuePos = mouse;
                g->tblBalls[0].pos = g->cuePos;
                g->tblBalls[0].pocketed=false;
                g->tblBalls[0].vel=(Vector2){0,0};
                g->state=PLAY;
                sprintf(g->status, "Cue placed. %s's turn", g->players[g->curPlayer].name);
            } else strcpy(g->status, "Invalid position! Place inside rails");
        }
        return;
    }
    if(g->ballsMoving) return;
    Vector2 cuePos=g->tblBalls[0].pocketed ? g->cuePos : g->tblBalls[0].pos;
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        if(dist(mouse, cuePos)<=BALL_R*1.6f){
            g->aiming=true;
            g->dragStart=mouse;
            g->pullDist=0; g->power=0;
        }
     }
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) && g->aiming){
        float d = dist(mouse, cuePos);
        if(d>MAX_PULL) d=MAX_PULL;
        g->pullDist=d;
        g->power=g->pullDist/MAX_PULL;
     }
    if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && g->aiming){
        g->aiming=false;
        Vector2 dir = { mouse.x-cuePos.x, mouse.y-cuePos.y };
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
        if(len<0.001f){ g->pullDist=0; g->power=0; return; }
        dir.x/=len; dir.y/=len;

        float shotSpeed=(g->pullDist/MAX_PULL)*MAX_SHOT;
        if(shotSpeed>MAX_SHOT) shotSpeed=MAX_SHOT;
        g->tblBalls[0].vel.x = dir.x*shotSpeed;
        g->tblBalls[0].vel.y = dir.y*shotSpeed;
        g->state=PLAY;
        g->firstShot=false;
        g->stickRecoiling=true;
        g->recoilTimer=0.12f;
        g->power=0;
    }
}
void drawPower(Game *g){
    int x=18, y=TBL_H+70, w=240, h=16;
    DrawText("Power:", x, TBL_H+36, 16, WHITE);
    DrawRectangle(x+80, y, w, h, GRAY);
    int fill = (int)(w*(g->pullDist/MAX_PULL));
    if(fill<0) fill=0; if(fill>w) fill=w;
    DrawRectangle(x+80, y, fill, h, RED);
    DrawRectangleLines(x+80, y, w, h, BLACK);
    char pstr[32]; sprintf(pstr, "%d%%",(int)((g->pullDist/MAX_PULL)*100.0f));
    DrawText(pstr, x+80+w+8, y-2, 16, WHITE);
}

void drawTable(){
    DrawRectangle(RAIL_W, RAIL_W, TBL_W-2*RAIL_W, TBL_H-2*RAIL_W, GREEN);
    DrawRectangle(0,0,TBL_W,RAIL_W,BROWN);
    DrawRectangle(0,TBL_H-RAIL_W,TBL_W,RAIL_W,BROWN);
    DrawRectangle(0,0,RAIL_W,TBL_H,BROWN);
    DrawRectangle(TBL_W-RAIL_W,0,RAIL_W,TBL_H,BROWN);
}
void drawGame(Game *g){
    BeginDrawing();
    ClearBackground((Color){8,80,23,255});
    drawTable();
   
}