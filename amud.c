#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define N_ROOMS 5
#define MAX_ROOM_W 30
#define MIN_ROOM_W 10
#define MAX_ROOM_H 20
#define MIN_ROOM_H 5

typedef struct _win_border_struct {
    chtype ls, rs, ts, bs,
           tl, tr, bl, br,
           fl; // floor
} WIN_BORDER;

typedef struct _WIN_struct
{
    int startx, starty;
    int height, width;
    WIN_BORDER border;
} WIN;

void init_win_params(WIN *p_win, int width, int height);
void print_win_params(WIN *p_win);
void create_box(WIN *p_win, bool FLAG);

int main()
{
    WIN win[N_ROOMS];
    int ch;

    initscr();                  /* Start curses mode            */
    start_color();              /* Start Color functionallity   */
    cbreak();                   /* Line Buffering disabled      */
    keypad(stdscr, TRUE);       /* F keys...                    */
    noecho();                   /* Don't echo chars             */

    init_pair(1, COLOR_CYAN, COLOR_BLACK);

    // seed randomness
    srand(time(NULL));
    printw("f2 to quit");

    attron(COLOR_PAIR(1));
    refresh();
    attroff(COLOR_PAIR(1));

    for (int i=0; i < N_ROOMS; i++)
    {
        init_win_params(&win[i], rand()%MAX_ROOM_W-1, rand()%MAX_ROOM_H-1);
        print_win_params(&win[i]);
        create_box(&win[i], TRUE);
    }

    while((ch = getch()) != KEY_F(2))
    {
        switch(ch)
        {
            case KEY_F(3):
                for (int i=0; i < N_ROOMS; i++)
                {
                    create_box(&win[i], FALSE);
                    refresh();
                    init_win_params(&win[i], rand()%MAX_ROOM_W-1, rand()%MAX_ROOM_H-1);
                    create_box(&win[i], TRUE);
                }
                break;
        }
    }

    endwin();
    return 0;
}

void init_win_params(WIN *p_win, int width, int height)
{
    if (width < MIN_ROOM_W)
        width = MIN_ROOM_W;
    if (height < MIN_ROOM_H)
        height = MIN_ROOM_H;

    int starty = (rand() % LINES) - height;
    int startx = (rand() % COLS) - width;

    p_win->starty = starty < 0 ? 0 : starty;
    p_win->startx = startx < 0 ? 0 : startx;
    p_win->height = height;
    p_win->width = width;
    p_win->border.ls = '|';
    p_win->border.rs = '|';
    p_win->border.ts = '-';
    p_win->border.bs = '-';
    p_win->border.tl = '+';
    p_win->border.tr = '+';
    p_win->border.bl = '+';
    p_win->border.br = '+';
    p_win->border.fl = '.';
}

void print_win_params(WIN *p_win)
{
#ifdef _DEBUG
    mvprintw(25, 0, "%d %d %d %d", p_win->startx, p_win->starty,
            p_win->width, p_win->height);
    refresh();
#endif
}

void create_box(WIN *p_win, bool flag)
{
    int i, j;
    int x, y, w, h;
    x = p_win->startx;
    y = p_win->starty;
    w = p_win->width;
    h = p_win->height;

    if (flag == TRUE)
    {
        mvaddch(y, x, p_win->border.tl);
        mvaddch(y, x + w, p_win->border.tr);
        mvaddch(y + h, x, p_win->border.bl);
        mvaddch(y + h, x + w, p_win->border.br);

        mvhline(y, x + 1, p_win->border.ts, w - 1);
        mvhline(y + h, x + 1, p_win->border.ts, w - 1);
        mvvline(y + 1, x, p_win->border.ls, h - 1);
        mvvline(y + 1, x + w, p_win->border.rs, h - 1);

        for (j = y+1; j < y + h; ++j)
            for (i = x+1; i < x + w; ++i)
                mvaddch(j, i, p_win->border.fl);
    }
    else
        for (j = y; j <= y + h; ++j)
            for (i = x; i <= x + w; ++i)
                mvaddch(j, i, ' ');
}
