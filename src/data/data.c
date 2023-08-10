#include "data.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

Data* createData() {
    Data* data = (Data*)malloc(sizeof(Data));
    DrowPoint* drowPoint = createDrowPoint();
    ChatText* chatText = createChatText();
    data->point = *drowPoint;
    data->comment = *chatText;
    data->id = 0;
    data->next = NULL;
    return data;
}

DrowPoint* createDrowPoint() {
    DrowPoint* drowPoint = (DrowPoint*)malloc(sizeof(DrowPoint));
    drowPoint->startX = 0;
    drowPoint->startY = 0;
    drowPoint->endX = 0;
    drowPoint->endY = 0;
    drowPoint->color = 0;
    return drowPoint;
}

ChatText* createChatText() {
    ChatText* chatText = (ChatText*)malloc(sizeof(ChatText));
    chatText->color = 0;
    return chatText;
}

void addData(Data* data, Data* head) {
    Data* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = data;
}

void createOutput(Data* data, char* output) {
    char* buf = (char*)malloc(sizeof(char) * 1100);
    if (data->type == DRAW) {
        sprintf(buf, "\x1b[38;5;%dm%s\x1b[0m", data->comment.color,
                data->comment.text);
        strncpy(output, buf, 1024);
    }
    free(buf);
}
