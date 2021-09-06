#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>

#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

typedef enum _tiles_e
{
    WALL='#'-32,
    FLOOR='.'-32,
    PLAYER='@'-32,
    SPACE=' '-32
} tiles_e;

const int map_width = 35;
const int map_height = 21;

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
    font = TTF_OpenFont(FONT_PATH, 16);
    if (font == NULL)
    {
        fprintf(stderr, "in main: Failed to load font\n");
        return 1;
    }
    SDL_Color color = {255, 255, 255};
    SDL_Texture* texture_cache[128-32];
    for (Uint16 ch=32; ch < 128-32; ++ch)
    {
        SDL_Surface* glyph_surf = TTF_RenderGlyph_Solid(font, ch, color);
        texture_cache[ch-32] = SDL_CreateTextureFromSurface(renderer, glyph_surf);
        SDL_FreeSurface(glyph_surf);
    }
    TTF_CloseFont(font);

    const char* text = "Really long string that will exceed the length of the text box";

    SDL_Rect mapr;
    mapr.x = 0;
    mapr.y = 0;
    SDL_QueryTexture(texture_cache[WALL], NULL, NULL, &mapr.w, &mapr.h);

    SDL_Rect logr;
    logr.w = mapr.w;
    logr.h = mapr.h;

    // Scale map elements by 2
    mapr.w*=2;
    mapr.h*=2;

    logr.x = (mapr.w*map_width)+logr.w;
    logr.y = 0;

    int logr_startx = logr.x, logr_starty = logr.y;

    tiles_e map[map_width*map_height];
    for (int i=0; i < map_width*map_height; i++)
        map[i] = SPACE;

    int nrooms = 1;
    SDL_Rect rm[nrooms];
    memset(rm, 0, sizeof(SDL_Rect)*nrooms);

    // seed randomness
    //srand(time(NULL));

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
                    logr.y += logr.h*2;
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
