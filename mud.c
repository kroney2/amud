#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>

#define _DEBUG

// server settings
#define REMOTEHOST "localhost"
#define PORT "3491"
// font stuff
#define ASCII_OFFSET 32
#define FONT_PATH "ColleenAntics.ttf"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define LOG_SPEED 150
// map dims
#define FONT_SIZE 25
#define MAP_WIDTH 30
#define MAP_HEIGHT 19
#define MIN_RM_WIDTH 3
#define MIN_RM_HEIGHT 5

typedef enum _tiles_e
{
    WALL='#'-ASCII_OFFSET,
    FLOOR='.'-ASCII_OFFSET,
    PLAYER='@'-ASCII_OFFSET,
    SPACE=' '-ASCII_OFFSET
} tiles_e;

typedef struct _as_arg
{
    SDL_Renderer* renderer;
    SDL_Rect* logr;
    int logr_startx;
    int logr_starty;
    SDL_Texture** texture_cache;
    char* msg;
} as_arg_t;

void* poll_serv(void* as_arg);
void* tf_log_handler(void* as_arg);
int get_conn();

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
    SDL_Texture* texture_cache[128 - ASCII_OFFSET];
    for (Uint16 ch = ASCII_OFFSET; ch < 128; ++ch)
    {
        if (ch == PLAYER + ASCII_OFFSET)
        {
            c.r = rand()%255; c.g = rand()%255; c.b = rand()%255;
        }
        else
        {
            c.r = 0; c.g = 255; c.b = 0;
        }
        SDL_Surface* glyph_surf = TTF_RenderGlyph_Solid(font, ch, c);
        texture_cache[ch - ASCII_OFFSET] = SDL_CreateTextureFromSurface(renderer,
                                                                        glyph_surf);
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

    mapr.w = ((SCREEN_WIDTH / 4) * 3)/MAP_WIDTH;
    mapr.h = ((SCREEN_HEIGHT / 4) * 3)/MAP_HEIGHT;

//    logr.x = (mapr.w * MAP_WIDTH)+logr.w;
    logr.x = 0;
    logr.y = (mapr.h * MAP_HEIGHT);

    int logr_startx = logr.x, logr_starty = logr.y;

    char buf[256]; // msg data;
    strcpy(buf, "MSG BUFFER IS NOT INITALIZED");
    pthread_t th;
    as_arg_t as_arg = {renderer, &logr, logr_startx,
                      logr_starty ,texture_cache, buf};
    pthread_create(&th, NULL, poll_serv, &as_arg);

    tiles_e map[MAP_WIDTH * MAP_HEIGHT];
    for (int i=0; i < MAP_WIDTH * MAP_HEIGHT; i++)
        map[i] = FLOOR;

    for (int y=0; y < MAP_HEIGHT; y++)
    {
        for (int x=0; x < MAP_WIDTH; x++)
        {
            // top
            if (y == 0)
               map[(y * MAP_WIDTH) + x] = WALL;
            // right
            if (x == MAP_WIDTH - 1)
                map[(y * MAP_WIDTH) + x] = WALL;
            // bottom
            if (y == MAP_HEIGHT - 1)
                map[(y * MAP_WIDTH) + x] = WALL;
            // left
            if (x == 0)
                map[(y * MAP_WIDTH) + x] = WALL;

        }
    }

    SDL_Point player_pos;
    player_pos.x = MAP_WIDTH / 2;
    player_pos.y = MAP_HEIGHT / 2;
    map[(player_pos.y * MAP_WIDTH) + player_pos.x] = PLAYER;

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
                        texture_cache[map[(y * MAP_WIDTH) + x]],
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

int get_conn(void)
{
    int sockfd, err;
    int yes=1; // setsockopt() SO_REUSEADDR
    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (( err = getaddrinfo(REMOTEHOST, PORT, &hints, &ai)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
        {
            perror("socket");
            continue;
        }

        // if the socket is already in use, use it.
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("connect");
            continue;
        }
        break;
    }
    freeaddrinfo(ai);

    if (p == NULL)
    {
        fprintf(stderr, "failed to connect to server\n");
        return 2;
    }

    return sockfd;
}

void* poll_serv(void* as_arg)
{
    int sockfd = get_conn();
    if (sockfd == -1)
    {
        fprintf(stderr, "error connecting to server\n");
        exit(1);
    }
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

    int fd_count = 0;
    int fd_size = 1;
    fd_count = 1;
    char* msg = (char*)((as_arg_t*)as_arg)->msg;
    while(1)
    {
        int poll_count = poll(&pfd, fd_count, -1);
        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }
        if (pfd.revents & POLLIN)
        {
            int nbytes = recv(pfd.fd, msg, 255, 0);
            if (nbytes <= 0)
            {
                if (nbytes == 0)
                {
                    printf("connection closed\n");
                    break;
                }
                else
                    perror("recv");

                close(pfd.fd);
                exit(1);
            }
            else
            {
                msg[nbytes] = 0;
                tf_log_handler(as_arg);
            }
        }
    }
    pthread_exit(0);
}

void* tf_log_handler(void* as_arg)
{
    as_arg_t *arg = (as_arg_t*)as_arg;
    for (char* msgPtr = arg->msg; *msgPtr != 0; msgPtr++)
    {
        if (*msgPtr == '\n')
            *msgPtr = ' ';

        if (arg->logr->x >= SCREEN_WIDTH - arg->logr->w)
        {
            arg->logr->x = arg->logr_startx;
            arg->logr->y += arg->logr->h;
        }

        SDL_Delay(LOG_SPEED);
        SDL_RenderCopy(
                arg->renderer,
                arg->texture_cache[(int)*msgPtr-ASCII_OFFSET],
                NULL,   // srcrect
                arg->logr
        );
        SDL_RenderPresent(arg->renderer);
        arg->logr->x += arg->logr->w;
    }
    arg->logr->y += arg->logr->h;
    arg->logr->x = arg->logr_startx;

    if (arg->logr->y >= SCREEN_HEIGHT - arg->logr->h)
    {
        arg->logr->y = arg->logr_starty;
        SDL_RenderClear(arg->renderer); //TODO: this wipes game view as
                                        // well, update only logr bit...
    }
}
