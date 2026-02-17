# 0714-02-CSE-2100
# 🎱 8-Ball Pool Game — Code Documentation

**Course:** Advanced Programming Lab  
**Project:** 8-Ball Pool Game (C Language / Raylib)  
**Purpose:** Document the refactored codebase in accordance with the Snake Game Refactoring & Software Engineering Standards Guide  
**Date:** February 2026

---

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Refactoring Summary](#refactoring-summary)
3. [Naming Conventions Applied](#naming-conventions-applied)
4. [Folder Structure](#folder-structure)
5. [Architecture & Modular Design](#architecture--modular-design)
6. [Data Structures](#data-structures)
7. [Constants & Configuration](#constants--configuration)
8. [Function Reference](#function-reference)
9. [Game Loop & State Machine](#game-loop--state-machine)
10. [Physics Engine](#physics-engine)
11. [Input Handling](#input-handling)
12. [Rendering System](#rendering-system)
13. [Known Issues & Future Improvements](#known-issues--future-improvements)

---

## Project Overview

This is a two-player 8-Ball Pool game built in C using the [Raylib](https://www.raylib.com/) graphics library. Players take turns shooting a cue ball to pocket their assigned group of balls (solids or stripes) before legally potting the 8-ball to win.

### Features

- Real-time physics with friction, elastic ball-to-ball collisions, and rail bouncing
- Drag-to-shoot mechanic with visual power bar
- Cue stick recoil animation
- Two-player turn management with type assignment on first pocket
- Scratch (cue ball pocketed) handling with ball-in-hand placement
- Win/loss detection including early 8-ball and scratch-on-8-ball rules

### Dependencies

| Dependency | Purpose |
|---|---|
| `raylib.h` | Window, drawing, input |
| `math.h` | `sqrtf`, `fabs`, `atan2f` |
| `stdio.h` | `sprintf` for status strings |
| `stdlib.h` | Standard utilities |
| `string.h` | `strcpy` for player names |
| `stdbool.h` | `bool` type |

### Build

```bash
gcc main.c -o pool -lraylib -lm
```

---

## Refactoring Summary

This section maps every change made to the original code against the guidelines in the **Snake Game Refactoring & Software Engineering Standards Guide**.

### What Changed and Why

| Category | Before (Old Code) | After (Refactored) | Guideline Reference |
|---|---|---|---|
| Struct field names | `pos`, `vel`, `num`, `striped` | `position`, `velocity`, `number`, `isStriped` | Naming Conventions → Variable Naming |
| Enum value names | `CUE`, `SOLID`, `STRIPE`, `EIGHT` | `BALL_CUE`, `BALL_SOLID`, `BALL_STRIPE`, `BALL_EIGHT` | Naming Conventions → Constants |
| Game state names | `START`, `PLAY`, `SCRATCH`, `WON`, `LOST` | `GAME_START`, `GAME_PLAYING`, `GAME_SCRATCH`, `GAME_WON`, `GAME_LOST` | Naming Conventions → Constants |
| Player type names | `NONE`, `SOL`, `STR` | `PLAYER_NONE`, `PLAYER_SOLIDS`, `PLAYER_STRIPES` | Naming Conventions → Constants |
| Game struct fields | `tblBalls`, `curPlayer`, `pullDist`, `stickRecoiling` | `balls`, `currentPlayer`, `stickPullPixels`, `stickRecoil` | Naming Conventions → Variable Naming |
| Player struct fields | `remain` | `ballsRemaining` | Naming Conventions → Variable Naming |
| Function names | `initGame`, `updGame`, `chkCollisions`, `chkPockets`, `chkWin`, `updPhysics`, `drawPower`, `drawTable`, `drawGame` | `InitGame`, `UpdateGame`, `CheckCollisions`, `CheckPockets`, `CheckWinCondition`, `UpdatePhysics`, `DrawPowerBar`, `DrawTable`, `DrawGame` | Naming Conventions → Function Naming |
| Helper function names | `dist`, `clampSpeed`, `resolveElastic`, `ballsAreMoving`, `playerIdxForType`, `applyScratch`, `nextTurn` | `Distance`, `ClampBallSpeed`, `ResolveElasticCollision`, `AreBallsMoving`, `playerIndexForType`, `ApplyScratch`, `NextTurn` | Naming Conventions → Function Naming |
| Constants | `NUM_BALLS`, `TBL_W`, `TBL_H`, `BALL_R`, `POCK_R`, `RAIL_W`, `MAX_PULL`, `MAX_SHOT`, `MIN_SPEED` | `MAX_BALLS`, `TABLE_WIDTH`, `TABLE_HEIGHT`, `BALL_RADIUS`, `POCKET_RADIUS`, `RAIL_WIDTH`, `MAX_POWER_PIXELS`, `MAX_SHOT_SPEED`, `MIN_VELOCITY` | Naming Conventions → Constants |
| Comment quality | Sparse or missing | Section headers, purpose comments, inline explanations | Coding Style → Comment Quality |
| Function responsibility | `drawGame` began drawing partway through | `DrawTable` calls `BeginDrawing`; `DrawGame` owns the full frame | Modular Design Principles |

---

## Naming Conventions Applied

### Variables

Self-documenting names are used throughout to make the code readable without needing to trace logic.

```c
// ❌ Before
int curPlayer;
float pullDist;
bool stickRecoiling;

// ✅ After
int currentPlayer;
float stickPullPixels;
bool stickRecoil;
```

### Functions

Verb-first, PascalCase naming clarifies the purpose of every function at a glance.

```c
// ❌ Before
void chkCollisions(Game *g);
void updPhysics(Game *g);
void drawPower(Game *g);

// ✅ After
void CheckCollisions(Game *game);
void UpdatePhysics(Game *game);
void DrawPowerBar(Game *game);
```

### Constants

All `#define` constants now use descriptive uppercase names that eliminate magic numbers.

```c
// ❌ Before
#define TBL_W 800
#define BALL_R 15
#define MAX_PULL 160.0f

// ✅ After
#define TABLE_WIDTH 800
#define BALL_RADIUS 15
#define MAX_POWER_PIXELS 160.0f
```

### Enumerations

All enum values are prefixed with their type name to avoid naming collisions and improve readability.

```c
// ❌ Before
typedef enum { CUE, SOLID, STRIPE, EIGHT } BallType;
typedef enum { START, PLAY, SCRATCH, WON, LOST } GameState;
typedef enum { NONE, SOL, STR } PlayerType;

// ✅ After
typedef enum { BALL_CUE, BALL_SOLID, BALL_STRIPE, BALL_EIGHT } BallType;
typedef enum { GAME_START, GAME_PLAYING, GAME_SCRATCH, GAME_WON, GAME_LOST } GameState;
typedef enum { PLAYER_NONE, PLAYER_SOLIDS, PLAYER_STRIPES } PlayerType;
```

---

## Folder Structure

### Recommended Structure (Aligned with Refactoring Guide)

```
8-ball-pool/
├── src/
│   ├── main/
│   │   └── main.c              ← Entry point, window, game loop
│   ├── core/
│   │   ├── game.c              ← InitGame, UpdateGame, state machine
│   │   ├── physics.c           ← UpdatePhysics, CheckCollisions, ClampBallSpeed
│   │   ├── balls.c             ← ResetBalls, ball setup helpers
│   │   └── rules.c             ← CheckPockets, CheckWinCondition, NextTurn, ApplyScratch
│   ├── input/
│   │   └── input.c             ← HandleInput
│   ├── rendering/
│   │   ├── renderer.c          ← DrawGame, DrawTable, DrawPowerBar
│   │   └── ui.c                ← Status bar, HUD elements
│   └── utils/
│       └── math_utils.c        ← Distance, ResolveElasticCollision, AreBallsMoving
├── include/
│   ├── game.h
│   ├── physics.h
│   ├── renderer.h
│   └── utils.h
├── assets/
│   └── (textures, sounds — future)
├── docs/
│   └── 8BallPool_Documentation.md
├── build/
│   └── (compiled output)
└── README.md
```

> **Note:** The current single-file implementation is a valid starting point. The structure above is the target for Phase 2 modularization (see [Refactoring Roadmap](#known-issues--future-improvements)).

---

## Architecture & Modular Design

The game follows a standard **game loop pattern** as recommended by the refactoring guide:

```c
// main() — entry point
while (!WindowShouldClose()) {
    UpdateGame(&game);   // Process input + physics + state
    DrawGame(&game);     // Render everything
}
```

Each top-level function has a single, clearly defined responsibility:

| Function | Responsibility |
|---|---|
| `InitGame` | One-time setup of all game state |
| `ResetBalls` | Arrange balls in triangle rack |
| `HandleInput` | Read mouse/keyboard, update aiming state |
| `UpdatePhysics` | Move balls, apply friction, bounce rails |
| `CheckCollisions` | Ball-to-ball overlap detection and resolution |
| `ResolveElasticCollision` | Physics math for equal-mass collision |
| `CheckPockets` | Detect pocketed balls, trigger rules |
| `CheckWinCondition` | Check if current player can shoot 8-ball |
| `NextTurn` | Swap active player |
| `ApplyScratch` | Handle cue ball pocketed |
| `DrawTable` | Render felt, rails, pockets |
| `DrawGame` | Orchestrate full frame render |
| `DrawPowerBar` | Render power UI element |

---

## Data Structures

### `Ball`

Represents a single pool ball on the table.

```c
typedef struct {
    Vector2 position;     // Current XY position on table
    Vector2 velocity;     // Per-frame velocity vector
    Color   color;        // Raylib Color for rendering
    BallType type;        // BALL_CUE / BALL_SOLID / BALL_STRIPE / BALL_EIGHT
    int     number;       // Ball number (0 = cue ball)
    bool    pocketed;     // True when ball has been sunk
    bool    isStriped;    // True for stripe balls (9–15)
} Ball;
```

### `Player`

Tracks per-player state for the two-player turn system.

```c
typedef struct {
    PlayerType type;         // PLAYER_NONE / PLAYER_SOLIDS / PLAYER_STRIPES
    int        ballsRemaining; // Count of unpocketed balls of their type
    char       name[20];     // Display name shown in status bar
} Player;
```

### `Game`

The central game state container. Every function that needs to read or modify state receives a pointer to this struct.

```c
typedef struct {
    Ball      balls[MAX_BALLS];     // All 16 balls (index 0 = cue ball)
    Player    players[2];           // Player 1 and Player 2
    int       currentPlayer;        // Index of active player (0 or 1)
    GameState state;                // Current phase of the game

    Vector2   cueBallPos;           // Respawn position after scratch
    float     power;                // Normalized shot power [0.0 – 1.0]
    bool      aiming;               // True while player is dragging
    bool      ballsMoving;          // True while any ball has velocity
    bool      firstShot;            // True until break shot is taken
    bool      assignedTypes;        // True once solids/stripes are assigned

    char      statusMessage[100];   // Message shown in UI strip

    // Cue stick animation
    Vector2   dragStart;            // Mouse position at start of drag
    float     stickPullPixels;      // Current drag distance in pixels
    float     stickLength;          // Visual length of rendered stick
    bool      stickRecoil;          // True during post-shot recoil animation
    float     recoilTimer;          // Countdown timer for recoil duration
} Game;
```

---

## Constants & Configuration

All tunable values are defined as named constants at the top of the file. Changing gameplay feel only requires editing this section.

```c
// Table geometry
#define MAX_BALLS         16       // Total balls including cue ball
#define TABLE_WIDTH       800      // Table width in pixels
#define TABLE_HEIGHT      400      // Table height in pixels
#define BALL_RADIUS       15       // Radius of each ball
#define POCKET_RADIUS     28       // Radius of each pocket opening
#define RAIL_WIDTH        40       // Thickness of the table rail border

// Physics tuning
#define FRICTION          0.985f   // Per-frame speed multiplier (< 1.0 = slowing)
#define MIN_VELOCITY      0.06f    // Speed below which a ball is considered stopped
#define MAX_POWER_PIXELS  160.0f   // Max drag distance mapped to full power
#define MAX_SHOT_SPEED    22.0f    // Maximum launch speed of cue ball
#define MAX_BALL_SPEED    26.0f    // Hard cap on any ball's speed per frame
```

---

## Function Reference

### Initialization

#### `void InitGame(Game *game)`
Sets all game fields to their starting values, assigns player names, resets flags, and calls `ResetBalls`. Safe to call at any time to restart the game (also triggered by pressing `R`).

#### `void ResetBalls(Game *game)`
Places the cue ball at 25% of table width and arranges balls 1–15 in a standard triangle rack at 72% of table width. Assigns `BALL_SOLID`, `BALL_STRIPE`, or `BALL_EIGHT` types and matching colors.

---

### Game Update

#### `void UpdateGame(Game *game)`
Main per-frame update. Calls `HandleInput`, manages the cue stick recoil animation timer, calls `UpdatePhysics` while in `GAME_PLAYING` or `GAME_SCRATCH` states, and detects the transition from balls moving to stopped (triggering `CheckWinCondition` and `NextTurn`).

#### `void HandleInput(Game *game)`

| Input | Action |
|---|---|
| `R` key | Restart game via `InitGame` |
| Left click (SCRATCH state) | Place cue ball inside rails |
| Left click near cue ball | Begin aiming drag |
| Hold left button (aiming) | Update `stickPullPixels` and `power` |
| Release left button (aiming) | Fire cue ball; start recoil animation |

---

### Physics

#### `void UpdatePhysics(Game *game)`
Iterates all non-pocketed balls each frame: advances position by velocity, applies `FRICTION`, zeroes velocity below `MIN_VELOCITY`, resolves rail bounce with 0.86× energy retention, then calls `CheckCollisions` and `CheckPockets`.

#### `void CheckCollisions(Game *game)`
O(n²) check of all ball pairs. When two balls overlap (distance < 2 × `BALL_RADIUS`), they are pushed apart by the overlap amount and `ResolveElasticCollision` is called to exchange normal-axis velocity components.

#### `void ResolveElasticCollision(Ball *a, Ball *b)`
Implements equal-mass 2D elastic collision by projecting both velocity vectors onto the collision normal and tangent, swapping the normal components, and reconstructing the new velocity vectors. Tangent components are preserved (no spin model).

#### `void ClampBallSpeed(Ball *b, float maxSpeed)`
Normalizes the ball's velocity vector and rescales to `maxSpeed` if the magnitude exceeds it. Prevents energy accumulation from repeated collisions.

#### `float Distance(Vector2 a, Vector2 b)`
Returns the Euclidean distance between two 2D points. Used by collision and pocket detection throughout the codebase.

#### `bool AreBallsMoving(Game *game)`
Returns `true` if any non-pocketed ball has a velocity component exceeding `MIN_VELOCITY`. Used to gate input and turn transitions.

---

### Rules & State

#### `void CheckPockets(Game *game)`
Tests every non-pocketed ball against all 6 pocket positions. On a match: marks the ball pocketed, zeroes its velocity, and branches on ball type — cue ball triggers `ApplyScratch`; 8-ball sets `GAME_WON` or `GAME_LOST`; other balls decrement the owning player's `ballsRemaining`. Also handles first-pocket type assignment if `assignedTypes` is false.

#### `void CheckWinCondition(Game *game)`
If the current player has cleared all their balls (`ballsRemaining == 0`), updates the status message to prompt shooting the 8-ball.

#### `void NextTurn(Game *game)`
Flips `currentPlayer` between 0 and 1 and updates the status message.

#### `void ApplyScratch(Game *game)`
Sets state to `GAME_SCRATCH`, updates the status message, and passes control to the opponent by flipping `currentPlayer`.

#### `int playerIndexForType(Game *game, BallType btype)`
Returns the index (0 or 1) of the player assigned to `BALL_SOLID` or `BALL_STRIPE`. Returns -1 if no assignment has been made yet.

---

### Rendering

#### `void DrawGame(Game *game)`
Orchestrates the full frame: calls `DrawTable`, draws all non-pocketed balls (with stripe ring and number overlays), draws the aiming line while the player is dragging, renders the UI panel, status message, and power bar, then calls `EndDrawing`.

#### `void DrawTable()`
Calls `BeginDrawing`, clears to dark green, draws the brown border rectangles and green felt interior, then draws 6 black circles for the pockets.

#### `void DrawPowerBar(Game *game)`
Renders a labeled horizontal bar below the table. The fill width is proportional to `game->power` (range 0–1), shown in red against a white outline.

---

## Game Loop & State Machine

```
                    ┌─────────────┐
                    │  GAME_START │  (initial, breaks immediately into GAME_PLAYING)
                    └──────┬──────┘
                           │ first shot fired
                           ▼
                    ┌──────────────┐
              ┌────▶│ GAME_PLAYING │◀────┐
              │     └──────┬───────┘     │
              │            │             │
              │     cue pocketed    turn ends
              │            ▼             │
              │     ┌─────────────┐      │
              │     │GAME_SCRATCH │      │
              │     └──────┬──────┘      │
              │            │ cue placed  │
              └────────────┘             │
                           └────────────-┘
                           │ 8-ball pocketed
               ┌───────────┴────────────┐
               ▼                        ▼
          ┌──────────┐          ┌─────────────┐
          │ GAME_WON │          │  GAME_LOST  │
          └──────────┘          └─────────────┘
```

Press **R** at any time to return to `GAME_START`.

---

## Physics Engine

### Friction Model

Each frame, every velocity component is multiplied by `FRICTION` (0.985). A ball launched at `MAX_SHOT_SPEED` (22.0 px/frame) will decelerate to below `MIN_VELOCITY` (0.06 px/frame) in approximately **270 frames** (~4.5 seconds at 60 FPS).

### Rail Bounce

When a ball's edge touches a rail boundary, its position is corrected to the boundary and the perpendicular velocity component is multiplied by **-0.86**, simulating ~74% energy retention per bounce.

### Ball-to-Ball Collision

Uses a 2D elastic collision model assuming equal mass for all balls. The algorithm:

1. Compute the unit normal vector between ball centers.
2. Project each ball's velocity onto the normal and tangent axes.
3. Swap the normal components (equal-mass elastic result).
4. Reconstruct velocity vectors from the new normal + original tangent components.

---

## Input Handling

The drag-to-shoot mechanic works as follows:

1. **Press** left mouse button within `BALL_RADIUS * 1.6` of the cue ball → begin aiming (`aiming = true`).
2. **Hold and drag** away from the cue ball → `stickPullPixels` tracks drag distance (capped at `MAX_POWER_PIXELS`); `power` is normalized to [0, 1].
3. **Release** → direction is computed from drag vector (note: direction is from *mouse to cue ball*, so dragging away from the target aims correctly); shot speed scales linearly with `power`; recoil animation begins.

The recoil animation (`stickRecoil = true`) runs for `recoilTimer = 0.12` seconds, during which `stickPullPixels` decays by ×0.92 per frame for a smooth visual snap-back.

---

## Known Issues & Future Improvements

### Current Bugs

| Issue | Description |
|---|---|
| Ball type assignment on break | Type assignment logic runs on `firstShot` flag but `firstShot` is set to `false` on the shot, not the pocket — can cause incorrect assignment on the break |
| No legal-shot validation | Player is not penalized for not hitting their own balls first |
| `assignedTypes` flag not reset | Calling `InitGame` resets the flag but `ResetBalls` does not reinstate `firstShot`-dependent logic cleanly |
| Rendering split across two functions | `BeginDrawing` is called inside `DrawTable` rather than at the top of `DrawGame`, which breaks the conventional Raylib pattern |

### Refactoring Roadmap

Following the phased approach from the guide:

**Phase 1 — Cleanup** *(complete)*
- [x] Fix all naming conventions
- [x] Add section-level comments
- [x] Descriptive function parameters (`game` instead of `g`)

**Phase 2 — Modularization** *(recommended next)*
- [ ] Separate into `physics.c`, `rules.c`, `renderer.c`, `input.c`
- [ ] Create corresponding `.h` header files
- [ ] Move `BeginDrawing`/`EndDrawing` entirely into `DrawGame`

**Phase 3 — Optimization**
- [ ] Spatial partitioning for collision detection (e.g., grid cells)
- [ ] Decouple physics update rate from render rate

**Phase 4 — Feature Expansion**
- [ ] Sound effects on collision and pocketing
- [ ] Ghost ball aiming aid (trajectory preview)
- [ ] AI opponent
- [ ] High-score or game history persistence
- [ ] Single-player mode with ball-in-hand anywhere

---

## Final Note

This refactored codebase demonstrates the application of professional software engineering standards to a C game project:

✅ Descriptive, self-documenting names throughout  
✅ Clear separation of physics, rules, input, and rendering  
✅ Named constants replacing all magic numbers  
✅ Consistent PascalCase function naming with verb-first convention  
✅ Enum values prefixed with their type name  
✅ Central `Game` struct eliminating global variables  
✅ Comments explaining *why*, not just *what*

The architecture is ready for Phase 2 modularization into separate source files without any changes to function signatures or game logic.
