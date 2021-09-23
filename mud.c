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

//#define _DEBUG

// server settings
#define REMOTEHOST "localhost"
#define PORT "3491"
// font stuff
#define ASCII_OFFSET 32
#define FONT_PATH "ColleenAntics.ttf"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
// map dims
#define FONT_SIZE 25
#define MAP_WIDTH 30
#define MAP_HEIGHT 20
#define MIN_RM_SIZE 8
#define MAX_RM_SIZE MAP_WIDTH - MIN_RM_SIZE

typedef enum _tiles_e
{
    WALL='#'-ASCII_OFFSET,
    FLOOR='.'-ASCII_OFFSET,
    PLAYER='@'-ASCII_OFFSET,
    SPACE=' '-ASCII_OFFSET
} tiles_e;

typedef struct _bsp_node_t
{
    SDL_Rect r;
    int connected;
    struct _bsp_node_t* children[2];
    struct _bsp_node_t* parent;
} bsp_node_t;

typedef struct _bsp_queue_t
{
    bsp_node_t* data;
    struct _bsp_queue_t* next;
} bsp_queue_t;

int make_rms(bsp_node_t* root, int axis);
int get_conn();
void* poll_serv(void* arg);

int main()
{
    char buf[256]; // msg data;
    strcpy(buf, "NOT CONNECTED TO SERV!");
    pthread_t th;
    pthread_create(&th, NULL, poll_serv, buf);

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
            c.r = 255; c.g = 255; c.b = 255;
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
    mapr.h = SCREEN_HEIGHT/MAP_HEIGHT;

    logr.x = (mapr.w * MAP_WIDTH)+logr.w;
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
        int bsp_axis = rand()%2;
        int nrms = make_rms(qPtr->data, bsp_axis);

        bsp_queue_t* tail;
        for (tail = qPtr; tail->next != NULL; tail = tail->next);
        tail->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));

        if (nrms == 2)
        {
            tail->next->data = qPtr->data->children[0];
            tail->next->next = (bsp_queue_t*)calloc(1, sizeof(bsp_queue_t));
            tail->next->next->data = qPtr->data->children[1];
        }
        else if (nrms == 1)
        {
            if (qPtr->data->children[0] != NULL)
                tail->next->data = qPtr->data->children[0];
            else
                tail->next->data = qPtr->data->children[1];
        }
        else
            break;
#ifdef _DEBUG
    bsp_node_t* p = NULL;
    if ((p = qPtr->data->children[0]) != NULL)
        printf("RM1: x: %d, y: %d, w: %d, h: %d\n", p->r.x, p->r.y, p->r.w, p->r.h);
    if ((p = qPtr->data->children[1]) != NULL)
        printf("RM2: x: %d, y: %d, w: %d, h: %d\n", p->r.x, p->r.y, p->r.w, p->r.h);
#endif
    }

    tiles_e map[MAP_WIDTH * MAP_HEIGHT];
    for (int i=0; i < MAP_WIDTH * MAP_HEIGHT; i++)
        map[i] = FLOOR;
    for (bsp_queue_t* qPtr = q; qPtr->data != NULL; qPtr = qPtr->next)
    {
        SDL_Rect rm = qPtr->data->r;
        for (int y=0; y < MAP_HEIGHT; y++)
        {
            for (int x=0; x < MAP_WIDTH; x++)
            {
                // top
                if (y == rm.y)
                   map[(y * MAP_WIDTH) + x] = WALL;
                // right
                if (y >= rm.y && y < rm.y + rm.h && x == (rm.x + rm.w) - 1)
                    map[(y * MAP_WIDTH) + x] = WALL;
                // bottom
                if (y == (rm.y + rm.h) - 1)
                    map[(y * MAP_WIDTH) + x] = WALL;
                // left
                if (y >= rm.y && y < rm.y + rm.h && x == rm.x)
                    map[(y * MAP_WIDTH) + x] = WALL;
            }
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
               case SDLK_l:
               {
                    logr.y += logr.h;//*2;
                    for (char* msgPtr = buf; *msgPtr != 0; msgPtr++)
                    {
                        if (*msgPtr == '\n')
                            *msgPtr = ' ';

                        if (logr.x >= SCREEN_WIDTH-(logr.w*2))
                        {
                            logr.x = logr_startx;
                            logr.y += logr.h;
                        }

                        SDL_RenderCopy(
                                renderer,
                                texture_cache[(int)*msgPtr-ASCII_OFFSET],
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

void init_bsp_node(bsp_node_t* node, bsp_node_t* parent,
    int w, int h, int x, int y)
{
    node->r.w = w;
    node->r.h = h;
    node->r.x = x;
    node->r.y = y;
    node->parent = parent;
    node->connected = 0;
}

int make_rms(bsp_node_t* root, int axis)
{
    int nrms = 0;

    float range = (rand() % (MAX_RM_SIZE - MIN_RM_SIZE)) / 10.0f;
    int sizey = root->r.h * range;
    int sizex = root->r.w * range;
    int part1_sz = (axis) ? sizey : sizex;
    int part2_sz = (axis) ? root->r.h - sizey : root->r.w - sizex;

    root->children[0] = NULL;
    root->children[1] = NULL;

    if (part1_sz >= MIN_RM_SIZE)
    {
        root->children[0] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        int w = (axis) ? root->r.w : part1_sz;
        int h = (axis) ? part1_sz : root->r.h;
        int x = root->r.x;
        int y = root->r.y;
        init_bsp_node(root->children[0], root, w, h, x, y);
        nrms++;
    }
    if (part2_sz >= MIN_RM_SIZE)
    {
        root->children[1] = (bsp_node_t*)calloc(1, sizeof(bsp_node_t));
        int w = (axis) ? root->r.w : part2_sz;
        int h = (axis) ? part2_sz : root->r.h;
        int x = root->r.x;
        int y = root->r.y;
        if (root->children[0] != NULL)
        {
            if (axis)
                y+=h;
            else
             x+=w;
        }
        init_bsp_node(root->children[1], root, w, h, x, y);
        nrms++;
    }

    return nrms;
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

void* poll_serv(void* arg)
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
    char* buf = (char*)arg;
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
            int nbytes = recv(pfd.fd, buf, 255, 0);
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
                buf[nbytes] = 0;
        }
    }
    pthread_exit(0);
}
