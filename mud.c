#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>

#define _DEBUG
#define FONT_PATH "/home/caleb/.local/share/fonts/ColleenAntics.ttf"
#define MIN_RM_SIZE  3

typedef enum _tiles_e
{
    WALL='#'-32,
    FLOOR='.'-32,
    PLAYER='@'-32,
    SPACE=' '-32
} tiles_e;

typedef struct _bsp_node_t
{
    SDL_Rect r;
    struct _bsp_node_t* children[2];
    struct _bsp_node_t* parent;

} bsp_node_t;

typedef struct _bsp_queue_t
{
    bsp_node_t* data;
    struct _bsp_queue_t* next;

} bsp_queue_t;

const int map_width = 55;
const int map_height = 25;

int insert_into_bsp_tree(bsp_node_t* root, int axis);

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window;
    window = SDL_CreateWindow(
            "muddy",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            1280,
            800,
            SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (window == NULL)
    {
        fprintf(stderr, "in main: Failed to initalize window\n");
        return 1;
    }
    SDL_Renderer* renderer;
    renderer = SDL_CreateRenderer(
            window,
            -1, // index of rendering driver
            0   // flags
    );
    if (renderer == NULL)
    {
        fprintf(stderr, "in main: Failed to initalize renderer\n");
        return 1;
    }

    if (TTF_Init() < 0)
    {
        fprintf(stderr, "in main: Failed to initalize ttf\n");
        return 1;
    }
    TTF_Font *font;
    font = TTF_OpenFont(FONT_PATH, 32);
    if (font == NULL)
    {
        fprintf(stderr, "in main: Failed to load font\n");
        return 1;
    }

    // seed randomness
    srand(time(NULL));

    SDL_Color color = {255, 255, 255};
    SDL_Texture* texture_cache[128-32];
    for (Uint16 ch=32; ch < 128-32; ++ch)
    {
        SDL_Surface* glyph_surf = TTF_RenderGlyph_Solid(font, ch, color);
        texture_cache[ch-32] = SDL_CreateTextureFromSurface(renderer, glyph_surf);
        SDL_FreeSurface(glyph_surf);
    }
    TTF_CloseFont(font);

    SDL_Rect mapr;
    mapr.x = 0;
    mapr.y = 0;
    SDL_QueryTexture(texture_cache[WALL], NULL, NULL, &mapr.w, &mapr.h);

    SDL_Rect logr;
    logr.w = mapr.w;
    logr.h = mapr.h;

    logr.x = (mapr.w*map_width)+logr.w;
    logr.y = 0;

    int logr_startx = logr.x, logr_starty = logr.y;

    tiles_e map[map_width*map_height];
    for (int i=0; i < map_width*map_height; i++)
        map[i] = SPACE;

    int nrooms = 3;
    SDL_Rect rm[nrooms];
    memset(rm, 0, sizeof(SDL_Rect)*nrooms);

    for (int i=0; i < nrooms; i++) {
        rm[i].w = rand()%map_width;
        rm[i].x = rand()%(map_width-rm[i].w);
        rm[i].h = rand()%map_height;
        rm[i].y = rand()%(map_height-rm[i].h);
    for (int y=0; y < map_height; y++)
    {
        for (int x=0; x < map_width; x++)
        {
            if (x >= rm[i].x && x <= rm[i].w + rm[i].x
                        && (y == rm[i].y || y == rm[i].h + rm[i].y))
            {
                map[(y*map_width)+x] = WALL;
            }
            else if (y > rm[i].y && y < rm[i].h + rm[i].y
                    && (x == rm[i].x || x == rm[i].w + rm[i].x))
            {
                map[(y*map_width)+x] = WALL;
            }
            else if (x > rm[i].x && x < rm[i].w + rm[i].x
                    && y > rm[i].y && y < rm[i].h + rm[i].y)
            {
                map[(y*map_width)+x] = FLOOR;
            }
        }
    }}

     // Initalize BSP_node root
     bsp_node_t root;
     root.parent = NULL;
     root.r.w = map_width;
     root.r.h = map_height;

     bsp_queue_t* q = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
     q->data = &root;

     for (int i=0; i < 3; i++)
     {
         bsp_queue_t* qPtr = q;
         bsp_node_t* child1;
         bsp_node_t* child2;
         const int err = insert_into_bsp_tree(qPtr->data, rand()%2);

         if (err == 0)
         {
             child1 =  qPtr->data->children[0];
             child2 =  qPtr->data->children[1];
         }
         else if (err == 1)
             child1 =  qPtr->data->children[0];
         else if (err == 2)
             child2 =  qPtr->data->children[1];

         q->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));

         // get to the end of the list
         for (; qPtr->data != NULL; qPtr=qPtr->next);

         if (err == 0)
         {
             qPtr->data = child1;
             qPtr->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
             qPtr->next->data = child2;
         }
         else if (err == 1)
             qPtr->data = child1;
         else if (err == 2)
             qPtr->data = child2;

         bsp_queue_t* temp = q;
         q = q->next;
         free(temp);
     }

    /*
    * TODO(Caleb): Connect Rooms
    * =====================
    * 1 ) Pick a side of a rm if on the left side continue left until
    * a floor tile is hit or until xpos is 0
    *
    * 2 ) if a floor is found mark this room as "connected"
    * generate a passage connecting the two rooms, continue to next room
    */

    SDL_Point player_pos;
    player_pos.x = (rm[0].w/2) + rm[0].x;
    player_pos.y = (rm[0].h/2) + rm[0].y;
    map[(player_pos.y*map_width)+player_pos.x] = PLAYER;

    SDL_Event event;
    ////////////////////////////
    int initalrender = 1; // dirty hack TODO(Caleb): fix this
    while(1) {
    if (initalrender)  // dirty hack TODO(Caleb): fix this
    {
        for (int y=0; y < map_height; y++)
        {
            for (int x=0; x < map_width; x++)
            {
                mapr.x = x*mapr.w;
                mapr.y = y*mapr.h;
                SDL_RenderCopy(
                        renderer,
                        texture_cache[map[(y*map_width)+x]],
                        NULL,   // srcrect
                        &mapr
                );
            }
        }
        SDL_RenderPresent(renderer);
        initalrender = 0;
    }
    //////////////////////////
    while(SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
            SDL_Rect map_clip;
            map_clip.x = 0;
            map_clip.y = 0;
            map_clip.w = map_width*mapr.w;
            map_clip.h = map_height*mapr.h;
            SDL_RenderFillRect(renderer, &map_clip);

           switch(event.key.keysym.sym)
           {
               case SDLK_d:
                   if (map[(player_pos.y*map_width)+player_pos.x+1] != WALL)
                   {
                       map[(player_pos.y*map_width)+player_pos.x] = FLOOR;
                       map[(player_pos.y*map_width)+player_pos.x+1] = PLAYER;
                       player_pos.x++;
                   }
                   break;
               case SDLK_a:
                   if (map[(player_pos.y*map_width)+player_pos.x-1] != WALL)
                   {
                       map[(player_pos.y*map_width)+player_pos.x] = FLOOR;
                       map[(player_pos.y*map_width)+player_pos.x-1] = PLAYER;
                       player_pos.x--;
                   }
                   break;
               case SDLK_w:
                   if (map[((player_pos.y-1)*map_width)+player_pos.x] != WALL)
                   {
                       map[(player_pos.y*map_width)+player_pos.x] = FLOOR;
                       map[((player_pos.y-1)*map_width)+player_pos.x] = PLAYER;
                       player_pos.y--;
                   }
                   break;
               case SDLK_s:
                   if (map[((player_pos.y+1)*map_width)+player_pos.x] != WALL)
                   {
                       map[(player_pos.y*map_width)+player_pos.x] = FLOOR;
                       map[((player_pos.y+1)*map_width)+player_pos.x] = PLAYER;
                       player_pos.y++;
                   }
                   break;
               case SDLK_q:
                   return 0;
               case SDLK_l:
               {
                    logr.y += logr.h;//*2;
                    char* msgs[] = {
                        "CALEB WAS HIT BY A BUS SHOULD'VE HAD MORE PEOPLE WRITING JPC",
                        "KYLE IS DOING MATH...", "THE PLOT THICKENS",
                        "ALEX WRITE'S A TRIE, IT WAS SUPER EFFECTIVE!",
                        "NATHAN PRESSES A BROWN KEY SWITCH... EVERYONE HAS A MOMENT OF SILENCE TO APPRECIATE ITS CRISPINESS",
                        "STEVE CASTS: MEMORY LEAK, MICRO'S SPRINT TAKES 99 DMG",
                        "SAHAR & SPEPH SUMMON A PYTHON",
                        "WHAT IS THE MEANING OF THIS", "YOU SHALL NOT PASS!!!",
                        "CAN C&E PLS GO TO HANDMADE SEATTLE? :)",
                        "HANDMADE SEATTLE 2021 WILL MAKE C&E 10X MORE PRODUCTIVE!!",
                        "CHRIS USED A \"STATIC\", IT PROBABLY FIXED THE ISSUE"
                    };

                    char* msg = msgs[rand()%12];
                    for (char* msgPtr = msg; *msgPtr != '\0'; msgPtr++)
                    {
                        if (logr.x >= 1280-(logr.w*2))
                        {
                            logr.x = logr_startx;
                            logr.y += logr.h;
                        }

                        SDL_RenderCopy(
                                renderer,
                                texture_cache[*msgPtr-32],
                                NULL,   // srcrect
                                &logr
                        );
                        logr.x += logr.w;
                    }
                    logr.x = logr_startx;

                    if (logr.y >= 800-(logr.h*2))
                    {
                        logr.y = logr_starty;
                        SDL_RenderClear(renderer);
                    }
               }
               break;
            } // switch

            for (int y=0; y < map_height; y++)
            {
                for (int x=0; x < map_width; x++)
                {
                    mapr.x = x*mapr.w;
                    mapr.y = y*mapr.h;
                    SDL_RenderCopy(
                            renderer,
                            texture_cache[map[(y*map_width)+x]],
                            NULL,   // srcrect
                            &mapr
                    );
                }
            }
            SDL_RenderPresent(renderer);

        } // if SDL_KEYDOWN
    }} //  poll event & while(1)

    return 0;
}

int insert_into_bsp_tree(bsp_node_t* root, int axis)
{
    if (root->r.w < MIN_RM_SIZE && root->r.h < MIN_RM_SIZE)
        return -1;

    int new_size;
    int new_size2;
    int new_sizey = rand()%root->r.w;
    int new_sizex = rand()%root->r.h;

    new_size = (axis) ? new_sizex : new_sizey;
    new_size2 = (axis) ? root->r.h - new_sizex : root->r.w - new_sizey;

    if (new_size >= MIN_RM_SIZE)
    {
        root->children[0] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        root->children[0]->r.w = (axis) ? root->r.w : new_size;
        root->children[0]->r.h = (axis) ? new_size : root->r.h;
        root->children[0]->parent = root;
    }

    if (new_size2 >= MIN_RM_SIZE)
    {
        root->children[1] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        root->children[1]->r.w = (axis) ? root->r.w : root->r.w - new_size;
        root->children[1]->r.h = (axis) ? root->r.h - new_size : root->r.h;
        root->children[1]->parent = root;
    }

#ifdef _DEBUG
    if (root->children[0] != NULL)
        printf("child 1 w: %d, h; %d\n", root->children[0]->r.w, root->children[0]->r.h);
    if (root->children[1] != NULL)
        printf("child 2 w: %d, h; %d\n", root->children[1]->r.w, root->children[1]->r.h);
#endif

    if (root->children[0] != NULL && root->children[1] != NULL)
        return 0;
    if (root->children[0] != NULL)
        return 1;
    return 2;
}
