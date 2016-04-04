/*
 * gti - a git launcher
 *
 * Copyright 2012 by Richard Wossal <richard@r-wos.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation.  No representations are made about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 */

#if defined (_WIN32) && !defined(WIN32)
#define WIN32
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#include <process.h>
#include <io.h>

// usleep() doesn't exist on MSVC, instead use Sleep() from Win32 API
#define usleep(a) Sleep((a) / 1000)

// exec*() on MSVC makes the parent process exit; that means that gti.exe will finish as git is starting,
// which causes cmd.exe to print its prompt over git's output (because it sees that the child process has
// finished). The solution is to use synchronous spawn*(): it will make gti.exe to wait until git finishes.
#define execv(a, b) do { i = _spawnv(_P_WAIT, (a), (b)); if (i != -1) return i; } while(0)
#define execvp(a, b) do { i = _spawnvp(_P_WAIT, (a), (b)); if (i != -1) return i; } while(0)

#else
#include <unistd.h>
#endif

#ifndef WIN32
#include <sys/ioctl.h>
#else
#include <windows.h>
#endif

// SunOS defines winsize in termios.h
#if defined(__sun) && defined(__SVR4)
#include <sys/termios.h>
#endif


#define GIT_NAME "git"

#ifndef GTI_SPEED
#define GTI_SPEED 50
#endif

int  term_width(void);
void other(void);
void merge(void);
void init_space(void);
void open_term();
void move_to_top(void);
void line_at(int start_x, const char *s);
void draw_car(int x);
void draw_cow(int x);
void draw_merged(int x);
void clear_car(int x);
void clear_cow(int x);

int TERM_WIDTH;
FILE *TERM_FH;
int SLEEP_DELAY;

int main(int argc, char **argv)
{
    char *git_path;
    (void) argc;

    open_term();
    TERM_WIDTH = term_width();
    SLEEP_DELAY = 1000000 / (TERM_WIDTH + GTI_SPEED);

    init_space();

    if(argc > 1){
    if(strcmp(argv[1],"merge")==0){
	merge();
    } else {
	other();
    }
    }

    fflush(TERM_FH);
    git_path = getenv("GIT");
    if (git_path) {
      execv(git_path, argv);
    } else {
      execvp(GIT_NAME, argv);
    }
    /* error in exec if we land here */
    perror(GIT_NAME);
    return 1;
}

void other(void){
    int i;
    for (i = -20; i < TERM_WIDTH; i++) {
        draw_car(i);
        usleep(SLEEP_DELAY);
	clear_car(i);
    }
    move_to_top();
}

void merge(void){
    int i;
    for (i = -20; i < TERM_WIDTH; i++) {
        draw_car(i);
        draw_cow(TERM_WIDTH-38);
        usleep(SLEEP_DELAY);
	clear_car(i);
    }
    clear_cow(TERM_WIDTH-38);
    draw_merged(TERM_WIDTH-58);
}

void init_space(void)
{
    fputs("\n\n\n\n\n\n\n", TERM_FH); /* 8 lines, to not remove the PS1 line */
#ifdef WIN32
    fflush(TERM_FH);
#endif
}

#ifndef WIN32

void open_term()
{
    TERM_FH = fopen("/dev/tty", "w");
    if (!TERM_FH)
        TERM_FH = stdout;
}

int term_width(void)
{
    struct winsize w;
    ioctl(fileno(TERM_FH), TIOCGWINSZ, &w);
    return w.ws_col;
}

void move_to_top(void)
{
    fprintf(TERM_FH, "\033[7A");
}

void move_to_x(int x)
{
    fprintf(TERM_FH, "\033[%dC", x);
}

#else

HANDLE con;

void open_term()
{
    TERM_FH = fopen("CONOUT$", "w+");
    con = (HANDLE)_get_osfhandle(fileno(TERM_FH));
}

int term_width(void)
{
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(con, &ci);
    return ci.dwSize.X;
}

void move_to_top(void)
{
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(con, &ci);
    ci.dwCursorPosition.X = 0;
    ci.dwCursorPosition.Y -= 7;
    SetConsoleCursorPosition(con, ci.dwCursorPosition);
}

void move_to_x(int x)
{
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(con, &ci);
    ci.dwCursorPosition.X = x;
    SetConsoleCursorPosition(con, ci.dwCursorPosition);
}

#endif

void line_at(int start_x, const char *s)
{
    int x;
    size_t i;
    if (start_x > 1)
        move_to_x(start_x);
    for (x = start_x, i = 0; i < strlen(s); x++, i++) {
        if (x > 0 && x < TERM_WIDTH)
            fputc(s[i], TERM_FH);
    }
#ifdef WIN32
    /*
     * It seems Windows wraps on whe cursor when it's about to overflow,
     * rather than after it has overflown (unless the overflowing character
     * is a newline), as other systems seems to do.
     */
    if (x < TERM_WIDTH)
#endif
    fputc('\n', TERM_FH);
    
#ifdef WIN32
    fflush(TERM_FH);
#endif
}

void draw_car(int x)
{
    move_to_top();
    line_at(x, "   ,---------------.");
    line_at(x, "  /  /``````|``````\\\\");
    line_at(x, " /  /_______|_______\\\\________");
    line_at(x, "|]      GTI |'       |        |]");
    if (x % 2) {
    line_at(x, "=  .-:-.    |________|  .-:-.  =");
    line_at(x, " `  -+-  --------------  -+-  '");
    line_at(x, "   '-:-'                '-:-'  ");
    } else {
    line_at(x, "=  .:-:.    |________|  .:-:.  =");
    line_at(x, " `   X   --------------   X   '");
    line_at(x, "   ':-:'                ':-:'  ");
    }
}

void draw_cow(int x)
{
    move_to_top();
    line_at(x,"");
    line_at(x,"  ^__^");
    line_at(x,"  (oo)\_______");
    line_at(x,"  (__)\       )\/\ ");
    line_at(x,"     ||----w |");
    line_at(x,"     ||     ||");
    line_at(x,"     ||     ||");
}

void draw_merged(int x){
    move_to_top();
    line_at(x,"");
    line_at(x,"  ^__^");
    line_at(x,"  (oo)\____________________");
    line_at(x,"  (__)\     merged!        )\/\ ");
    line_at(x, "   .-:-.                .-:-.  ");
    line_at(x, "    -+-  -----------w--  -+-  ");
    line_at(x, "   '-:-'                '-:-'  ");
}

void clear_car(int x)
{
    move_to_top();
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
}

void clear_cow(int x)
{
    move_to_top();
    line_at(x, "                  ");
    line_at(x, "                  ");
    line_at(x, "                  ");
    line_at(x, "                  ");
    line_at(x, "                  ");
    line_at(x, "                  ");
    line_at(x, "                  ");
}


