#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */

#include "../lib/_vars.h"

static void set_rect_xy(SDL_FRect *r, short x, short y)
/*小小的便利函数*/
{
    r->x = (float)(x * BLOCK_SIZE_IN_PIXELS);
    r->y = (float)(y * BLOCK_SIZE_IN_PIXELS);
}

static void set_player_xy(SDL_FRect *r, float x, float y)
{
    r->x = (int) x;
    r->y = (int) y;
}

void put_cell_at(StageInfo *s, int x, int y, Cell ct)
/*将cells[x][y]赋值为ct*/
{
    s->cells[y*s->game_width+x] = ct;
}

Cell locate_cell_at(const StageInfo *s, int x, int y)
/*查询位于cells[y][x]的cell是什么，返回值为enum Cells*/
{
    return (Cell)s->cells[y*s->game_width+x];
}

/*通过put_cell_at函数和locate_cell_at函数实现对一维数组的二维访问*/

void movement_handler(StageInfo *s, PlayerInfo *p)
{
    const bool *kb = p->kbstate_ptr;
    float *x = &p->x;                                                                  //不避免重名了，凑合看吧
    float *y = &p->y;
    float *vx = &p->vx;
    float *vy = &p->vy;
    float next_vx;
    float next_vy;
    // int hb_x = *x + PLAYER_HITBOX_WIDTH_OFFSET;
    // int hb_y = *y + PLAYER_HITBOX_HEIGHT_OFFSET;
    int game_width = s->game_width;
    int game_height = s->game_height;
    static bool last_space = false;
    int min_grid_x = (int)(*x) / BLOCK_SIZE_IN_PIXELS;
    int max_grid_x = (int)(*x + PLAYER_HITBOX_WIDTH - 1) / BLOCK_SIZE_IN_PIXELS;
    //全是直接按逻辑帧运算的。逻辑帧务必均匀。
    //碰撞检测思路credit to: https://hdmmy.github.io/2017/07/31/2D-%E5%B9%B3%E5%8F%B0%E6%B8%B8%E6%88%8F%E7%9A%84%E7%A2%B0%E6%92%9E%E6%A3%80%E6%B5%8B%E6%96%B9%E6%B3%95/

    // 判断玩家是否在地面上
    bool on_ground = false;
    int foot_y = (int)((*y + PLAYER_HITBOX_HEIGHT) / BLOCK_SIZE_IN_PIXELS);
    float foot_pixel = *y + PLAYER_HITBOX_HEIGHT;
    for (int gx = min_grid_x; gx <= max_grid_x; ++gx) {
        if (foot_y >= 0 && foot_y < game_height) {
            switch (locate_cell_at(s, gx, foot_y))
            {
            case CELL_WALL: case CELL_PIPE: case CELL_QUESTION:
                float block_top = foot_y * BLOCK_SIZE_IN_PIXELS;
                if (fabsf(foot_pixel - block_top) < EPS && *vy >= 0) {
                    on_ground = true;
                    break;
                }
                break;
            default:
                break;
            }
        }
    }


    /*地面上摩擦*/
    /*为了实现“松开按键后减速比人物跑的加速度大”的效果（即启动惯性比停止惯性大），不能按真实物理规则来 e.g.蔚蓝中，人物6f达到慢速，陆地上仅需3f自然停下https://zhuanlan.zhihu.com/p/186713195*/
    if ((kb[SDL_SCANCODE_RIGHT] == false && kb[SDL_SCANCODE_LEFT] == false) || (kb[SDL_SCANCODE_RIGHT] == true && kb[SDL_SCANCODE_LEFT] == true) && on_ground)
    {
        if ((fabsf(*vx) - PLAYER_GROUND_FRICTION) < EPS)          //“刹车问题”
            *vx = 0;
        else if (*vx > 0)
            *vx -= PLAYER_GROUND_FRICTION;
        else if (*vx < 0)
            *vx += PLAYER_GROUND_FRICTION;

    }

    /*按键->加速度->速度*/

    if (kb[SDL_SCANCODE_RIGHT] == true && kb[SDL_SCANCODE_LEFT] == false)    //右
    {
        next_vx = *vx+PLAYER_ACC;
        *vx = (fabsf(next_vx) < PLAYER_MAX_SPEED) ? next_vx : PLAYER_MAX_SPEED;
    }
    else if (kb[SDL_SCANCODE_LEFT] == true && kb[SDL_SCANCODE_RIGHT] == false)   //左
    {
        next_vx = *vx-PLAYER_ACC;
        *vx = (fabsf(next_vx) < PLAYER_MAX_SPEED) ? next_vx : -PLAYER_MAX_SPEED;
    }
    /*跳跃处理*/
    // 按下空格且在地面上且上次没按下空格时跳跃
    if (kb[SDL_SCANCODE_SPACE] && on_ground && !last_space) {
        *vy = -PLAYER_JUMP_INIT_VY;
    }
    last_space = kb[SDL_SCANCODE_SPACE];

    // 重力
    if (!on_ground)
    {
    *vy += G;
    }


    /*碰撞检测*/

    /*速度->位移*/
    //水平方向移动
    float next_x = *x + *vx;
    float final_x = *x;
    if (*vx != 0) {
        int dir = (*vx > 0) ? 1 : -1;
        // 计算碰撞盒边缘
        int edge_x = (dir > 0) ? (*x + PLAYER_HITBOX_WIDTH) : *x;
        int min_grid_y = (int)(*y) / BLOCK_SIZE_IN_PIXELS;
        int max_grid_y = (int)(*y + PLAYER_HITBOX_HEIGHT -1) / BLOCK_SIZE_IN_PIXELS;    //-1作用：确保碰撞箱的下边界/右边界正好落在最后一个像素时，仍然属于当前格子，而不是“溢出”到下一个格子。
        int grid_x;
        float min_dist = fabsf(*vx);
        // 遍历所有可能碰撞的格子
        for (int gy = min_grid_y; gy <= max_grid_y; ++gy) {
            grid_x = (int)((edge_x + dir * min_dist) / BLOCK_SIZE_IN_PIXELS);
            if (grid_x < 0 || grid_x >= game_width || gy < 0 || gy >= game_height)
                continue;   //边缘碰撞交给后面
                switch (locate_cell_at(s, grid_x, gy))
                {
                case CELL_WALL: case CELL_PIPE: case CELL_QUESTION:
                    float wall_edge = grid_x * BLOCK_SIZE_IN_PIXELS;
                    float dist;
                    if (dir > 0)
                        dist = wall_edge - (edge_x -1);    //dist->碰撞箱边缘与墙边缘的距离     //-1作用：同上
                    else
                        dist = (wall_edge + BLOCK_SIZE_IN_PIXELS) - edge_x;
                    if (fabsf(dist) < min_dist)
                        min_dist = fabsf(dist);
                    break;
                default:
                    break;
                }
        }
        // 实际移动距离
        final_x = *x + dir * min_dist;

        if (min_dist < fabsf(*vx)) {   //如果马上撞墙...
            *vx = 0;
            if (dir > 0) {
                // 向右撞墙，对齐到墙的左边界减去玩家宽度
                final_x = ((int)(final_x / BLOCK_SIZE_IN_PIXELS)) * BLOCK_SIZE_IN_PIXELS;
            } 
            else {
                // 向左撞墙，对齐到墙的右边界
                final_x = ((int)(final_x / BLOCK_SIZE_IN_PIXELS) + 1) * BLOCK_SIZE_IN_PIXELS - PLAYER_HITBOX_WIDTH;
            }
        }
    }
    else {
        final_x = *x;
    }
    *x = final_x;

    //竖直方向移动
    float next_y = *y + *vy;
    float final_y = *y;
    bool hit_head = false;      // 头部碰撞
    if (*vy != 0) {
        int dir = (*vy > 0) ? 1 : -1;
        int edge_y = (*vy > 0) ? (*y + PLAYER_HITBOX_HEIGHT) : *y;
        int grid_y;
        float min_dist = fabsf(*vy);
        for (int gx = min_grid_x; gx <= max_grid_x; ++gx) {
            grid_y = (int)((edge_y + dir * min_dist) / BLOCK_SIZE_IN_PIXELS);
            if (gx < 0 || gx >= game_width || grid_y < 0 || grid_y >= game_height)
                continue;
            switch (locate_cell_at(s, gx, grid_y))
            {
            case CELL_WALL: case CELL_PIPE: case CELL_QUESTION:
                float wall_edge = grid_y * BLOCK_SIZE_IN_PIXELS;
                float dist;
                if (dir > 0)
                    dist = wall_edge - (edge_y - 1);
                else
                    dist = (wall_edge + BLOCK_SIZE_IN_PIXELS) - edge_y;
                if (fabsf(dist) < min_dist)
                    min_dist = fabsf(dist);
                break;
            default:
                break;
            }
        }
        final_y = *y + dir * min_dist;
        if (min_dist < fabsf(*vy)) {
            *vy = 0;
            if (dir > 0) {
                // 下落，踩在地面上，强制对齐
                final_y =(int) (final_y / BLOCK_SIZE_IN_PIXELS)* BLOCK_SIZE_IN_PIXELS;
            }
            else {
                hit_head = true;
                final_y =(int) (final_y / BLOCK_SIZE_IN_PIXELS)* BLOCK_SIZE_IN_PIXELS;
            }
        }
    } 
    else {
        final_y = *y;
    }
    *y = final_y;

    /*边缘碰撞*/
    if (next_x > game_width*BLOCK_SIZE_IN_PIXELS-PLAYER_HITBOX_WIDTH)    //右边缘
    {
        *x = game_width*BLOCK_SIZE_IN_PIXELS-PLAYER_HITBOX_WIDTH;
        *vx = 0;
    }
    else if (next_x < 0)          //左边缘
    {
        *x = 0;
        *vx = 0;
    }
    /*不将p->x p->y合成到s->cells。*/
    if (DEBUG_INFO)
    printf("L%d R%d J%d\n", kb[SDL_SCANCODE_LEFT], kb[SDL_SCANCODE_RIGHT], kb[SDL_SCANCODE_SPACE]);

}

void background_init(char path[], StageInfo *s)
/*
从.txt文件读取地图。
.txt不是二进制文件，而是普通文本文件。游戏里每个格子占的字节数与unsigned char相同。
0x0d0a -> '\r' '\n' -> windows换行符
0x0a -> '\n' -> posix换行符
*/
{
    int matrix_size = 0;
    unsigned char *token = NULL;
    unsigned char *buffer = NULL;
    unsigned char cells[MAP_SIZE_MAX] = "";
    int width = 0;
    int height = 0;
    /*fopen_s 只在windows上可用*/
    // FILE *map_p = NULL;
    // errno_t err = fopen_s(&map_p, path, "rb");
    // if (err!=0)
    // {
    //     printf("Failed to open map %s, error code = %d", path, err);
    //     exit(1);
    // }

    FILE *map_p = NULL;
    map_p = fopen(path, "rb");
    if (map_p == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        exit(1);
    }

    fseek(map_p, 0, SEEK_END);
    matrix_size = ftell(map_p)/sizeof(unsigned char);      //ftell得到的是字节数，map_length为字节个数  //这个matrix_size受换行符长度影响

    buffer = (unsigned char *) SDL_calloc(1, sizeof(unsigned char)*matrix_size +1);  //+1用来存'\0'
    if(buffer == NULL)          //防止内存分配失败
    {
        perror("Map initialization failed");
        exit(-1);
    }

    // memset(cells, 0, sizeof(char)*matrix_size +1);         //给cells填充'\0' SDL_calloc帮我们做了。
    fseek(map_p, 0, SEEK_SET);
    fread(buffer, sizeof(unsigned char), matrix_size, map_p);     //fread不会给cells加'\0'，需要自己加
    fclose(map_p);

    token = strtok(buffer, "\r\n");         //太伟大了strtok，直接解决了\r\n问题（但cells末尾的\0还是要加的）
    width = strlen(token);

    while( token != NULL ) 
    {
        strcat(cells, token);
        token = strtok(NULL, "\r\n");
        height++;
    }
    matrix_size = width*height;
    s->game_width = width;
    s->game_height = height;
    s->matrix_size = matrix_size;
    s->cells = (unsigned char *) SDL_calloc (1, sizeof(unsigned char)*matrix_size);
    // s->cells = cells;        把s->cells覆盖成了cells的地址。
    memcpy(s->cells, cells, matrix_size);

    SDL_free(buffer);
    if (DEBUG_INFO)
    printf("Map loaded successful! width=%d, height=%d\n", width, height);
}

void game_init(StageInfo *s, PlayerInfo *p)
{
    bool player_spawn_point_found = false;
    int inp = 0;
    char path[30];
    printf("Which level?(1/2)");
    scanf("%d", &inp);
    switch (inp)
    {
    case 1:
        strcpy(path, "../maps/map1.txt");
        break;
    case 2:
        strcpy(path, "../maps/map2.txt");
        break;
    default:
        break;
    }

    background_init(path, s);
    p->vx = 0;
    p->vy = 0;
    p->x = 0;
    p->y = 0;
    for (int j = 0; j < s->game_height; j++)
    {
        if (player_spawn_point_found == true)
        {
            break;
        }
        for (int i = 0; i < s->game_width; i++)
        {
            if (locate_cell_at(s, i, j) == CELL_PLAYER_SPAWN)
            {
                p->x = i*BLOCK_SIZE_IN_PIXELS;
                p->y = j*BLOCK_SIZE_IN_PIXELS;
                player_spawn_point_found = true;
                break;
            }
        }
        
    }
    if (player_spawn_point_found == false)
    {
        printf("Player spawn point not found. Check your map.");
    }
}

SDL_AppResult key_event_handler(StageInfo *s, PlayerInfo *p, const bool *kbstate)
{
    p->kbstate_ptr = kbstate;   //这个函数最重要的功能：把kbstate的指针存在p里，等待movement_handler取用；把kbstate的内容传过去没有这种方法好，因为kbstate是sdl内部的数组。

    /* Quit. */
    if (kbstate[SDL_SCANCODE_ESCAPE] || kbstate[SDL_SCANCODE_Q]) {
        SDL_Quit();
        return SDL_APP_SUCCESS;
    }
    /* Restart the game as if the program was launched. */
    if(kbstate[SDL_SCANCODE_R])
        game_init(s, p);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO || SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        return SDL_APP_FAILURE;
    }

    *appstate = as;

    if (!SDL_CreateWindowAndRenderer("project neo", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0, &as->window, &as->renderer)) {
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec spec;
    char wavepath[] = "../sfx";

    // SDL_SetRenderDrawBlendMode(as->renderer, SDL_BLENDMODE_ADD);

    game_init(&as->s, &as->p);

    // as->last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;
    StageInfo *stageinfo = &as->s;
    PlayerInfo *playerinfo = &as->p;
    // const Uint64 now = SDL_GetTicks();
    SDL_FRect r;
    unsigned i;
    unsigned j;
    int ct;

    key_event_handler(stageinfo, playerinfo, SDL_GetKeyboardState(NULL));
    movement_handler(stageinfo, playerinfo);


    /*从这里开始渲染*/
    r.w = r.h = BLOCK_SIZE_IN_PIXELS;
    SDL_SetRenderDrawColor(as->renderer, BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(as->renderer);

    /*绘制玩家（玩家位置不在网格里记录）*/
    set_player_xy(&r, playerinfo->x, playerinfo->y);
    SDL_SetRenderDrawColor(as->renderer, 128, 94, 168, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(as->renderer, &r);


    for (i = 0; i < stageinfo->game_width; i++) {
        for (j = 0; j < stageinfo->game_height; j++) {
            set_rect_xy(&r, i, j);
            switch (locate_cell_at(stageinfo, i, j))
            {
            case CELL_AIR: case CELL_PLAYER_SPAWN:
                continue;
                break;
            case CELL_WALL:
                SDL_SetRenderDrawColor(as->renderer, 200, 76, 12, SDL_ALPHA_OPAQUE);
                break;
            case CELL_QUESTION:
                SDL_SetRenderDrawColor(as->renderer, 251, 151, 54, SDL_ALPHA_OPAQUE);
                break;
            case CELL_PIPE:
                SDL_SetRenderDrawColor(as->renderer, 127, 207, 0, SDL_ALPHA_OPAQUE);
                break;
            default:
                SDL_SetRenderDrawColor(as->renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
                break;
            }
            SDL_RenderFillRect(as->renderer, &r);
        }
    }

    SDL_RenderPresent(as->renderer);
    if (DEBUG_INFO)
    printf("x=%f, vx=%f, y=%f, vy=%f\n", playerinfo->x, playerinfo->vx, playerinfo->y, playerinfo->vy);
    SDL_Delay(STEP_RATE_IN_MILLISECONDS);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    StageInfo *stageinfo = &((AppState *)appstate)->s;
    PlayerInfo *playerinfo = &((AppState *)appstate)->p;
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    // case SDL_EVENT_KEY_DOWN: 
    // {
    //     return key_event_handler(stageinfo, playerinfo, SDL_GetKeyboardState(NULL));
    // }
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (appstate != NULL) {
        AppState *as = (AppState *)appstate;
        SDL_DestroyRenderer(as->renderer);
        SDL_DestroyWindow(as->window);
        SDL_free(as);
    }
}
