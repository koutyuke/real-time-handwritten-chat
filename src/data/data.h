#ifndef DATA_H
#define DATA_H

typedef enum {
    DRAW,
    CHAT,
    LOADDRAW,
    LOADCHAT,
} DataType;

typedef struct {
    short startX;
    short startY;
    short endX;
    short endY;
    unsigned long color;
} DrowPoint;

typedef struct {
    char text[1024];
    int color;
} ChatText;

typedef struct _data {
    DataType type;
    int id;
    ChatText comment;
    DrowPoint point;
    struct _data* next;
} Data;

Data* createData();
DrowPoint* createDrowPoint();
ChatText* createChatText();

void addData(Data* data, Data* head);

#endif
