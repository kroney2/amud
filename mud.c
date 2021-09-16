#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>

#define _DEBUG
#define FONT_PATH "ColleenAntics.ttf"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define FONT_SIZE 25
#define MAP_WIDTH 40
#define MAP_HEIGHT 25
#define MIN_RM_SIZE 4

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

int insert_into_bsp_tree(bsp_node_t* root, int axis);

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window;
    window = SDL_CreateWindow(
            "muddy",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
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
    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (font == NULL)
    {
        fprintf(stderr, "in main: Failed to load font\n");
        return 1;
    }
    srand(time(NULL));

    SDL_Color c;
    SDL_Texture* texture_cache[128-32];
    for (Uint16 ch=32; ch < 128-32; ++ch)
    {
        if (ch == '@')
        {
            c.r = rand()%255; c.g = rand()%255; c.b = rand()%255;
        }
        else
        {
            c.r = 255; c.g = 255; c.b = 255;
        }
        SDL_Surface* glyph_surf = TTF_RenderGlyph_Solid(font, ch, c);
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

    mapr.w = ((SCREEN_WIDTH/4)*3)/MAP_WIDTH;
    mapr.h = SCREEN_HEIGHT/MAP_HEIGHT;

    logr.x = (mapr.w*MAP_WIDTH)+logr.w;
    logr.y = 0;

    int logr_startx = logr.x, logr_starty = logr.y;

    bsp_node_t root;
    root.parent = NULL;
    root.r.w = MAP_WIDTH;
    root.r.h = MAP_HEIGHT;
    root.r.x = 0;
    root.r.y = 0;

    bsp_queue_t* q = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
    q->data = &root;
    q->next = NULL;

    for (bsp_queue_t* qPtr = q; qPtr->data != NULL; qPtr = qPtr->next)
    {
         const int err = insert_into_bsp_tree(qPtr->data, rand()%2);
         bsp_queue_t* eol;
         for (eol = qPtr; eol->next != NULL; eol=eol->next);

         eol->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
         if (err == 0)
         {
             eol->next->data = qPtr->data->children[0];
             eol->next->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
             eol->next->next->data = qPtr->data->children[1];
         }
         else if (err == 1)
             eol->next->data = qPtr->data->children[0];
         else if (err == 2)
             eol->next->data = qPtr->data->children[1];
    }

    tiles_e map[MAP_WIDTH*MAP_HEIGHT];
    for (int i=0; i < MAP_WIDTH*MAP_HEIGHT; i++)
        map[i] = SPACE;

    for (; q->data != NULL; )
    {
        SDL_Rect rm = q->data->r;
        for (int y=0; y < MAP_HEIGHT; y++)
        {
            for (int x=0; x < MAP_WIDTH; x++)
            {
                if (x >= rm.x && x <= rm.w + rm.x
                            && (y == rm.y || y == rm.h + rm.y))
                {
                    map[(y*MAP_WIDTH)+x] = WALL;
                }
                else if (y > rm.y && y < rm.h + rm.y
                        && (x == rm.x || x == rm.w + rm.x))
                {
                    map[(y*MAP_WIDTH)+x] = WALL;
                }
                else if (x > rm.x && x < rm.w + rm.x
                        && y > rm.y && y < rm.h + rm.y)
                {
                    map[(y*MAP_WIDTH)+x] = FLOOR;
                }
            }
        }
        bsp_queue_t* temp = q;
        q = q->next;
        free(temp);
    }

    SDL_Point player_pos;
    player_pos.x = MAP_WIDTH/2;
    player_pos.y = MAP_HEIGHT/2;
    map[(player_pos.y*MAP_WIDTH)+player_pos.x] = PLAYER;

    SDL_Event event;
    ////////////////////////////
    int initalrender = 1; // dirty hack TODO(Caleb): fix this
    while(1) {
    if (initalrender)  // dirty hack TODO(Caleb): fix this
    {
        for (int y=0; y < MAP_HEIGHT; y++)
        {
            for (int x=0; x < MAP_WIDTH; x++)
            {
                mapr.x = x*mapr.w;
                mapr.y = y*mapr.h;
                SDL_RenderCopy(
                        renderer,
                        texture_cache[map[(y*MAP_WIDTH)+x]],
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
            map_clip.w = MAP_WIDTH*mapr.w;
            map_clip.h = MAP_HEIGHT*mapr.h;
            SDL_RenderFillRect(renderer, &map_clip);

           switch(event.key.keysym.sym)
           {
               case SDLK_d:
                   if (map[(player_pos.y*MAP_WIDTH)+player_pos.x+1] != WALL)
                   {
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x] = FLOOR;
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x+1] = PLAYER;
                       player_pos.x++;
                   }
                   break;
               case SDLK_a:
                   if (map[(player_pos.y*MAP_WIDTH)+player_pos.x-1] != WALL)
                   {
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x] = FLOOR;
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x-1] = PLAYER;
                       player_pos.x--;
                   }
                   break;
               case SDLK_w:
                   if (map[((player_pos.y-1)*MAP_WIDTH)+player_pos.x] != WALL)
                   {
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x] = FLOOR;
                       map[((player_pos.y-1)*MAP_WIDTH)+player_pos.x] = PLAYER;
                       player_pos.y--;
                   }
                   break;
               case SDLK_s:
                   if (map[((player_pos.y+1)*MAP_WIDTH)+player_pos.x] != WALL)
                   {
                       map[(player_pos.y*MAP_WIDTH)+player_pos.x] = FLOOR;
                       map[((player_pos.y+1)*MAP_WIDTH)+player_pos.x] = PLAYER;
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
                        if (logr.x >= SCREEN_WIDTH-(logr.w*2))
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

                    if (logr.y >= SCREEN_HEIGHT-(logr.h*2))
                    {
                        logr.y = logr_starty;
                        SDL_RenderClear(renderer);
                    }
               }
               break;
            } // switch

            for (int y=0; y < MAP_HEIGHT; y++)
            {
                for (int x=0; x < MAP_WIDTH; x++)
                {
                    mapr.x = x*mapr.w;
                    mapr.y = y*mapr.h;
                    SDL_RenderCopy(renderer,
                                   texture_cache[map[(y*MAP_WIDTH)+x]],
                                   NULL,   // srcrect
                                   &mapr);
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

     int sizey = (root->r.h / 2)+(rand()%2);
     int sizex = (root->r.w / 2)+(rand()%2);

     int part1_sz = (axis) ? sizey : sizex;
     int part2_sz = (axis) ? root->r.h - sizey
         : root->r.w - sizex;

    root->children[0] = NULL;
    root->children[1] = NULL;

    if (part1_sz >= MIN_RM_SIZE)
    {
        root->children[0] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        root->children[0]->r.w = (axis) ? root->r.w : part1_sz;
        root->children[0]->r.h = (axis) ? part1_sz : root->r.h;
        root->children[0]->parent = root;
    }

    if (part2_sz >= MIN_RM_SIZE)
    {
        root->children[1] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        root->children[1]->r.w = (axis) ? root->r.w : root->r.w - part1_sz;
        root->children[1]->r.h = (axis) ? root->r.h - part1_sz : root->r.h;
        root->children[1]->parent = root;
    }

#ifdef _DEBUG
    if (root->children[0] != NULL || root->children[1] != NULL)
        printf("ROOT: W: %d, H: %d\n", root->r.w, root->r.h);
    if (root->children[0] != NULL)
        printf("|-Child1 W: %d, H: %d\n", root->children[0]->r.w, root->children[0]->r.h);
    if (root->children[1] != NULL)
        printf("|-Child2 W: %d, H: %d\n", root->children[1]->r.w, root->children[1]->r.h);
#endif

    if (root->children[0] != NULL && root->children[1] != NULL)
    {
        root->children[0]->r.x = root->r.x;
        root->children[0]->r.y = root->r.y;
        root->children[1]->r.x = root->r.w;
        root->children[1]->r.y = root->r.h;
        return 0;
    }
    else if (root->children[0] != NULL)
    {
        root->children[0]->r.x = root->r.x;
        root->children[0]->r.y = root->r.y;
        return 1;
    }
    else if (root->children[1] != NULL)
    {
        root->children[1]->r.x = root->r.x;
        root->children[1]->r.y = root->r.y;
        return 2;
    }

    return -1;
}
