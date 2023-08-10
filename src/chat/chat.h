#ifndef CHAT_H
#define CHAT_H

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#include "data/data.h"
#include "stdbool.h"

#define NAME_SIZE 40
#define BUF_SIZE 1024
#define BUF_SIZE_LONG 2048
#define MAX_CLIENTS 10
#define PORT_NO (u_short)20000
#define GRID_X 5
#define GRID_Y 4

typedef struct _stroke {
    XPoint p0;
    XPoint p1;
    unsigned long color;
    bool en;
    struct _stroke *next;
} Stroke;

void createWindow(int x, int y, char *title, Display **display, Window *window,
                  GC *gc, unsigned long *color);
void onClientEvent(Display *display, Window *window, GC *gc, int serverFd,
                   unsigned long *color);
void onServerEvent(Display *display, Window *window, GC *gc, int *clientFdList,
                   Data **head, unsigned long *color);
unsigned long parseColor(char *colorHex, Display *display);

#endif
