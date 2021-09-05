#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>

#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

typedef enum _tiles_e
{
    WALL=0,
    FLOOR,
    PLAYER,
    SPACE
} tiles_e;

const int map_width = 30;
const int map_height = 18;

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window;
    window = SDL_CreateWindow(
            "muddy",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            800,
            600,
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
    font = TTF_OpenFont(FONT_PATH, 25);
    if (font == NULL)
    {
        fprintf(stderr, "in main: Failed to load font\n");
        return 1;
    }
    SDL_Color color = {255, 255, 255};
    SDL_Surface* glyph_cache[128-32];
    for (Uint16 ch=32; ch < 128-32; ++ch)
        glyph_cache[ch-32] = TTF_RenderGlyph_Solid(font, ch, color);

    SDL_Texture* wall_texture = SDL_CreateTextureFromSurface(renderer, glyph_cache['#'-32]);
    SDL_Texture* floor_texture = SDL_CreateTextureFromSurface(renderer, glyph_cache['.'-32]);
    SDL_Texture* player_texture = SDL_CreateTextureFromSurface(renderer, glyph_cache['@'-32]);
    SDL_Texture* space_texture = SDL_CreateTextureFromSurface(renderer, glyph_cache[' '-32]);

    SDL_Rect tile;
    tile.x = 0;
    tile.y = 0;
    SDL_QueryTexture(wall_texture, NULL, NULL, &tile.w, &tile.h);

    tiles_e map[map_width*map_height];
    for (int i=0; i < map_width*map_height; i++)
        map[i] = SPACE;

    int nrooms = 3;
    SDL_Rect rm[nrooms];
    memset(rm, 0, sizeof(rm)*nrooms);

    rm[0].x = 5;
    rm[0].w = 5;
    rm[0].y = 7;
    rm[0].h = 3;

    rm[1].x = 10;
    rm[1].w = 6;
    rm[1].y = 4;
    rm[1].h = 7;

    rm[2].x = 7;
    rm[2].w = 7;
    rm[2].y = 7;
    rm[2].h = 7;

    for (int i=0; i < nrooms; i++) {
    for (int y=0; y < map_height; y++)
    {
        for (int x=0; x < map_width; x++)
        {

            //TODO(Caleb): Fix bug with walls on top of screen
            if ((x >= rm[i].x && x <= rm[i].w + rm[i].x
                && (y == rm[i].y || y == rm[i].h + rm[i].y))
                || (y > rm[i].y && y < rm[i].h + rm[i].y
                && (x == rm[i].x || x == rm[i].w + rm[i].x)))
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

    SDL_Point player_pos;
    player_pos.x = map_width/2;
    player_pos.y = map_height/2;
    map[(player_pos.y*map_width)+player_pos.x] = PLAYER;

    while(1)
    {
        SDL_RenderClear(renderer);

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
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
               }

            }
        }

        for (int y=0; y < map_height; y++)
        {
            for (int x=0; x < map_width; x++)
            {
                SDL_Texture* texture;
                switch(map[(y*map_width)+x])
                {
                    case WALL:
                        texture = wall_texture;
                        break;
                    case FLOOR:
                        texture = floor_texture;
                        break;
                    case PLAYER:
                        texture = player_texture;
                        break;
                    case SPACE:
                        texture = space_texture;
                        break;
                }

                tile.x = x*tile.w;
                tile.y = y*tile.h;
                SDL_RenderCopy(
                        renderer,
                        texture,
                        NULL,   // srcrect
                        &tile
                );
            }
        }

        SDL_RenderPresent(renderer);
    }
    return 0;
}
