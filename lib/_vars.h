#ifndef __VARS_H
#define __VARS_H

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
// #include "SDL3_image/SDL_image.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define DEBUG_INFO false

#define STEP_RATE_IN_MILLISECONDS 20
#define BLOCK_SIZE_IN_PIXELS 48
#define EPS 1e-5
#define SDL_WINDOW_WIDTH           1296
#define SDL_WINDOW_HEIGHT          960
#define MAP_SIZE_MAX 1024

#define BACKGROUND_COLOR_R 90
#define BACKGROUND_COLOR_G 147
#define BACKGROUND_COLOR_B 255

//均以左上角作为原点

#define PLAYER_HITBOX_WIDTH BLOCK_SIZE_IN_PIXELS
#define PLAYER_HITBOX_HEIGHT BLOCK_SIZE_IN_PIXELS

#define PLAYER_MAX_SPEED 10
#define PLAYER_ACC 0.2
#define PLAYER_GROUND_FRICTION 0.24
#define PLAYER_JUMP_INIT_VY 16
#define G 0.6

typedef enum
{
    CELL_AIR = '0',
    CELL_WALL,
    CELL_QUESTION,
    CELL_PIPE,
    CELL_PLAYER_SPAWN
} Cell;

typedef struct
{
    unsigned char *cells;
    int game_width;
    int game_height;
    int matrix_size;
} StageInfo;

typedef struct
{
    float x;
    float y;
    float vx;
    float vy;
    const bool *kbstate_ptr;
    /* kbstate-> esc right left up down space */
} PlayerInfo;


typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioStream *stream;
    StageInfo s;
    PlayerInfo p;
    // Uint64 last_step;
} AppState;

#endif