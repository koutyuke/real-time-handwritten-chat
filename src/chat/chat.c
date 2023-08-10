#include "chat.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "data/data.h"
#include "lib/error.h"

void createWindow(int x, int y, char *title, Display **display, Window *window,
                  GC *gc, unsigned long *color) {
    XSetWindowAttributes attributes;
    GC spaceGC;
    *display = XOpenDisplay(NULL);

    XSizeHints hints = {0};
    hints.flags = PPosition | PSize;
    hints.x = x;
    hints.y = y;
    hints.width = 300;
    hints.height = 300;

    *window = XCreateSimpleWindow(*display, RootWindow(*display, 0), x, y, 400,
                                  300, 5, BlackPixel(*display, 0),
                                  WhitePixel(*display, 0));
    attributes.backing_store = WhenMapped;
    XChangeWindowAttributes(*display, *window, CWBackingStore, &attributes);
    XSelectInput(*display, *window,
                 ExposureMask | ButtonPressMask | ButtonMotionMask);
    XStoreName(*display, *window, title);
    XSetNormalHints(*display, *window, &hints);
    XMapWindow(*display, *window);
    *gc = XCreateGC(*display, DefaultRootWindow(*display), 0, 0);
    XSetForeground(*display, *gc, parseColor("000000", *display));
    *color = parseColor("000000", *display);
    XSetLineAttributes(*display, *gc, 3, LineSolid, CapRound, JoinRound);
}

void onClientEvent(Display *display, Window *window, GC *gc, int serverFd,
                   unsigned long *color) {
    XEvent event;
    XPoint startPoint;
    Data *sendData;
    if (XEventsQueued(display, QueuedAfterFlush) == 0) {
        return;
    }
    XNextEvent(display, &event);
    switch (event.type) {
        case Expose:
            sendData = createData();
            sendData->type = LOADDRAW;
            write(serverFd, sendData, sizeof(Data));
            break;
        case ButtonPress:
            startPoint.x = event.xbutton.x;
            startPoint.y = event.xbutton.y;
            break;
        case MotionNotify:
            XSetForeground(display, *gc, *color);
            XDrawLine(display, *window, *gc, startPoint.x, startPoint.y,
                      event.xmotion.x, event.xmotion.y);

            sendData = createData();
            sendData->point.startX = startPoint.x;
            sendData->point.startY = startPoint.y;
            sendData->point.endX = event.xmotion.x;
            sendData->point.endY = event.xmotion.y;
            sendData->point.color = *color;
            sendData->type = DRAW;

            startPoint.x = event.xmotion.x;
            startPoint.y = event.xmotion.y;
            write(serverFd, sendData, sizeof(Data));
            free(sendData);
            break;
    }
}

void onServerEvent(Display *display, Window *window, GC *gc, int *clientFdList,
                   Data **head, unsigned long *color) {
    XEvent event;
    XPoint startPoint;
    Data *sendData, *setData, *spaceData;
    if (XEventsQueued(display, QueuedAfterFlush) == 0) {
        return;
    }
    XNextEvent(display, &event);
    switch (event.type) {
        case Expose:
            spaceData = *head;
            while (spaceData != NULL) {
                if ((spaceData)->type == DRAW) {
                    XSetForeground(display, *gc, (spaceData)->point.color);
                    XDrawLine(display, *window, *gc, (spaceData)->point.startX,
                              (spaceData)->point.startY,
                              (spaceData)->point.endX, (spaceData)->point.endY);
                }
                spaceData = spaceData->next;
            }
            break;
        case ButtonPress:
            startPoint.x = event.xbutton.x;
            startPoint.y = event.xbutton.y;
            break;
        case MotionNotify:
            XSetForeground(display, *gc, *color);
            XDrawLine(display, *window, *gc, startPoint.x, startPoint.y,
                      event.xmotion.x, event.xmotion.y);

            sendData = createData();
            sendData->point.startX = startPoint.x;
            sendData->point.startY = startPoint.y;
            sendData->point.endX = event.xmotion.x;
            sendData->point.endY = event.xmotion.y;
            sendData->point.color = *color;
            sendData->type = DRAW;

            startPoint.x = event.xmotion.x;
            startPoint.y = event.xmotion.y;

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientFdList[i] != 0) {
                    write(clientFdList[i], sendData, sizeof(Data));
                }
            }

            setData = createData();
            *setData = *sendData;

            if (*head == NULL) {
                *head = setData;
            } else {
                addData(setData, *head);
            }
            free(sendData);
            break;
    }
}

unsigned long parseColor(char *colorHex, Display *display) {
    unsigned int r = 0, g = 0, b = 0;
    unsigned long colorVal;
    char colorBuf[16];
    if (sscanf(colorHex, "%lx", &colorVal) == 1) {
        if (strlen(colorHex) == 3) {
            sscanf(colorHex, "%1x%1x%1x", &r, &g, &b);
            r += r * 16;
            g += g * 16;
            b += b * 16;
        } else if (strlen(colorHex) >= 6)
            sscanf(colorHex, "%02x%02x%02x", &r, &g, &b);
    }
    sprintf(colorBuf, "rgb:%02x/%02x/%02x", r, g, b);
    XColor scrColor, exactColor;
    XAllocNamedColor(display, DefaultColormap(display, DefaultScreen(display)),
                     colorBuf, &scrColor, &exactColor);
    printf("\33[48;2;%d;%d;%dm          ", r, g, b);
    printf("\33[0m\n");
    return scrColor.pixel;
}
