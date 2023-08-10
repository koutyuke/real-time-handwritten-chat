#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat/chat.h"
#include "data/data.h"
#include "lib/error.h"

#define NAME_SIZE 40
#define BUF_SIZE 1024
#define BUF_SIZE_LONG 2048
#define MAX_CLIENTS 10
#define PORT_NO (u_short)20000
#define ErrorExit(x)          \
    {                         \
        fprintf(stderr, "-"); \
        perror(x);            \
        exit(0);              \
    }

int i, j, msgLength, inputLength, result;
char buf[BUF_SIZE], inputBuf[BUF_SIZE], inputCommand[BUF_SIZE],
    strBuf[BUF_SIZE_LONG], hostName[NAME_SIZE], cmd,
    param[sizeof(char) * BUF_SIZE];
struct sockaddr_in sockAddr;
struct hostent *host;
struct timeval timeVal;
fd_set mask;

Display *display;
Window window;
GC gc;
unsigned long currentColor;

void startServer();
void startClient();

int main() {
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 1;
    while (1) {
        printf("Please select your role.\n");
        printf("Server: 1 ; Client: 2 \n");
        scanf("%s", inputBuf);
        if (strcmp(inputBuf, "1") == 0 || strcmp(inputBuf, "2") == 0) break;
    }
    if (strcmp(inputBuf, "1") == 0)
        startServer();
    else if (strcmp(inputBuf, "2") == 0)
        startClient();
}

void startServer() {
    int fdToListen, fdClientList[MAX_CLIENTS], fd, fdTemp, fdMax, clientCnt = 0;
    int inputLength;

    char space[1100];
    char *token, separator[2] = "\n";
    Data *clientBuf = NULL, *serverBuf = NULL, *setData = NULL;
    Data *drowData = NULL, *chatData = NULL;

    bool hasNext;

    // windowの作成
    createWindow(200, 200, "Server", &display, &window, &gc, &currentColor);

    // 初期化
    bzero(fdClientList, sizeof(int) * MAX_CLIENTS);

    if (gethostname(hostName, sizeof(hostName)) < 0) {
        ErrorExit("gethostname")
    };
    printf("Your hostname is '%s'\n", hostName);

    host = gethostbyname(hostName);
    if (host == NULL) {
        ErrorExit("gethostbyname")
    };

    bzero((char *)&sockAddr, sizeof(sockAddr));

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(PORT_NO);
    bcopy((char *)host->h_addr_list[0], (char *)&sockAddr.sin_addr,
          host->h_length);

    // socketのfdがreturn
    fdToListen = socket(AF_INET, SOCK_STREAM, 0);
    if (fdToListen < 0) {
        ErrorExit("socket")
    };
    int option = 1;
    setsockopt(fdToListen, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // socketの設定
    if (bind(fdToListen, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0) {
        ErrorExit("bind")
    };

    // 接続待ち
    listen(fdToListen, 1);
    // 標準出力
    write(1, "Waiting for someone to connect...\n", 34);

    while (1) {
        // Eventの監視
        onServerEvent(display, &window, &gc, fdClientList, &drowData,
                      &currentColor);

        FD_ZERO(&mask);
        FD_SET(fdToListen, &mask);
        FD_SET(0, &mask);

        fdMax = fdToListen;
        hasNext = true;
        for (i = 0; i < MAX_CLIENTS; i++) {
            fd = fdClientList[i];
            if (fd > 0) {
                FD_SET(fd, &mask);
            }
            if (fd > fdMax) {
                fdMax = fd;
            }
        }

        result = select(fdMax + 1, &mask, NULL, NULL, &timeVal);
        if (result < 0) {
            ErrorExit("select")
        };

        // A new client called connect()
        if (FD_ISSET(fdToListen, &mask)) {
            fd = accept(fdToListen, NULL, NULL);
            if (fd < 0) {
                ErrorExit("accept")
            };
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (fdClientList[i] == 0) {
                    fdClientList[i] = fd;
                    clientCnt++;
                    break;
                }
            }
        }

        // Message input via stdio (fd=0 means standard input)
        inputLength = 0;
        if (FD_ISSET(0, &mask)) {
            bzero(inputBuf, sizeof(char) * BUF_SIZE);
            bzero(inputCommand, sizeof(char) * BUF_SIZE);
            inputLength = read(0, inputBuf, BUF_SIZE);
            strcpy(inputCommand, inputBuf);
            token = strtok(inputCommand, separator);
            sscanf(token, "%c-%s", &cmd, param);
            // "Quit" command input
            if (strcmp(inputBuf, "Q\n") == 0 || inputLength == 0) {
                printf("Quit\n");
                strcpy(inputBuf, "Q\n");
                hasNext = false;
            }
        }

        // Iterate through all active clients

        // サーバーからのメッセージ入力
        if (inputLength > 0) {
            serverBuf = createData();
            serverBuf->type = CHAT;
            serverBuf->id = -1;
            serverBuf->comment.color = 1;

            if (strcmp(inputBuf, "Q\n") == 0) {
                strcpy(serverBuf->comment.text, inputBuf);
            } else if (cmd == 'C' || cmd == 'c') {
                currentColor = parseColor(param, display);
            } else {
                sprintf(space, "\x1b[38;5;%dmserver: %s\x1b[0m", 1, inputBuf);
                serverBuf->comment.color = 1;
                strncpy(serverBuf->comment.text, space, BUF_SIZE);
            }

            setData = createData();
            *setData = *serverBuf;
            if (chatData == NULL) {
                chatData = setData;
            } else {
                addData(setData, chatData);
            }
            free(serverBuf);
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            fd = fdClientList[i];
            if (fd == 0) {
                continue;
            }

            // サーバーからのメッセージを反映
            if (inputLength > 0) {
                serverBuf = createData();
                serverBuf->type = CHAT;
                serverBuf->id = -1;
                serverBuf->comment.color = 1;

                if (strcmp(inputBuf, "Q\n") == 0) {
                    strcpy(serverBuf->comment.text, inputBuf);
                } else if (cmd != 'C' && cmd != 'c') {
                    sprintf(space, "\x1b[38;5;%dmserver: %s\x1b[0m", 1,
                            inputBuf);
                    serverBuf->comment.color = 1;
                    strncpy(serverBuf->comment.text, space, BUF_SIZE);
                }

                Data *sample = chatData;

                write(fd, serverBuf, sizeof(Data));
                free(serverBuf);
            }

            // クライアントからのメッセージ確認
            if (FD_ISSET(fd, &mask)) {
                bzero(buf, sizeof(char) * BUF_SIZE);
                clientBuf = createData();
                msgLength = read(fd, clientBuf, sizeof(Data));

                if (clientBuf->type == CHAT) {
                    // "Quit" command received
                    if (strcmp(clientBuf->comment.text, "Q\n") == 0 ||
                        msgLength == 0) {
                        close(fd);
                        fdClientList[i] = 0;
                        clientCnt--;
                        continue;
                    }
                    strncpy(buf, clientBuf->comment.text, BUF_SIZE);

                    clientBuf->id = i;
                    clientBuf->comment.color = i * 10 + 11;
                    sprintf(space, "\x1b[38;5;%dmclient[%02d]: %s\x1b[0m",
                            i * 10 + 11, i, buf);
                    strncpy(clientBuf->comment.text, space, BUF_SIZE);

                    // Print the received message (fd=1 means standard output)
                    write(1, clientBuf->comment.text, sizeof(char) * BUF_SIZE);
                    // And also send that message to all other clients,
                    // except for the client who sent the message just now
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        fdTemp = fdClientList[j];
                        if (j == i || fdTemp == 0) continue;
                        write(fdTemp, clientBuf, sizeof(Data));
                    }

                    setData = createData();
                    *setData = *clientBuf;

                    if (chatData == NULL) {
                        chatData = setData;
                    } else {
                        addData(setData, chatData);
                    }
                } else if (clientBuf->type == LOADCHAT) {
                    if (chatData != NULL) {
                        setData = chatData;
                        while (setData != NULL) {
                            write(fd, setData, sizeof(Data));
                            setData = setData->next;
                        }
                    }
                } else if (clientBuf->type == DRAW) {
                    clientBuf->id = i;
                    XSetForeground(display, gc, clientBuf->point.color);
                    XDrawLine(display, window, gc, clientBuf->point.startX,
                              clientBuf->point.startY, clientBuf->point.endX,
                              clientBuf->point.endY);

                    for (j = 0; j < MAX_CLIENTS; j++) {
                        fdTemp = fdClientList[j];
                        if (j == i || fdTemp == 0) continue;
                        write(fdTemp, clientBuf, sizeof(Data));
                    }

                    setData = createData();
                    *setData = *clientBuf;

                    if (drowData == NULL) {
                        drowData = setData;
                    } else {
                        addData(setData, drowData);
                    }
                } else if (clientBuf->type == LOADDRAW) {
                    if (drowData != NULL) {
                        setData = drowData;
                        while (setData != NULL) {
                            write(fd, setData, sizeof(Data));
                            setData = setData->next;
                        }
                    }
                }

                free(clientBuf);
            }
        }
        if (!hasNext) {
            break;
        }
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        fd = fdClientList[i];
        if (fd > 0) {
            close(fd);
        }
    }
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    close(fdToListen);
}

void startClient() {
    int fdToConnect;
    char *token, separator[2] = "\n";

    Data *clientBuf = NULL, *serverBuf = NULL;

    printf("Please enter the server's hostname:\n");
    scanf("%s", hostName);
    host = gethostbyname(hostName);
    if (host == NULL) {
        ErrorExit("gethostbyname")
    };
    bzero((char *)&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(PORT_NO);
    bcopy((char *)host->h_addr_list[0], (char *)&sockAddr.sin_addr,
          host->h_length);
    fdToConnect = socket(AF_INET, SOCK_STREAM, 0);
    if (fdToConnect < 0) {
        ErrorExit("socket")
    };
    int option = 1;
    setsockopt(fdToConnect, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    connect(fdToConnect, (struct sockaddr *)&sockAddr, sizeof(sockAddr));

    // create a window
    createWindow(200, 200, "Client", &display, &window, &gc, &currentColor);

    // 1 標準出力
    write(1, "Please wait until someone sends a message...\n", 45);

    clientBuf = createData();
    clientBuf->type = LOADCHAT;
    write(fdToConnect, clientBuf, sizeof(Data));
    while (1) {
        onClientEvent(display, &window, &gc, fdToConnect, &currentColor);
        FD_ZERO(&mask);
        FD_SET(fdToConnect, &mask);
        FD_SET(0, &mask);
        result = select(fdToConnect + 1, &mask, NULL, NULL, &timeVal);
        if (result < 0) {
            ErrorExit("select")
        };

        // サーバーからのメッセージ
        if (FD_ISSET(fdToConnect, &mask)) {
            serverBuf = createData();
            msgLength = read(fdToConnect, serverBuf, sizeof(Data));
            // "Quit" command received
            if (strcmp(serverBuf->comment.text, "Q\n") == 0 || msgLength == 0) {
                break;
            }

            // Print the received message (fd=1 means standard output)
            // 標準出力
            if (serverBuf->type == CHAT) {
                write(1, serverBuf->comment.text, sizeof(char) * BUF_SIZE);
            } else if (serverBuf->type == DRAW) {
                XSetForeground(display, gc, serverBuf->point.color);
                XDrawLine(display, window, gc, serverBuf->point.startX,
                          serverBuf->point.startY, serverBuf->point.endX,
                          serverBuf->point.endY);
            }
            free(serverBuf);
        }
        // クライアントからのメッセージ入力
        if (FD_ISSET(0, &mask)) {
            bzero(inputBuf, sizeof(char) * BUF_SIZE);
            bzero(inputCommand, sizeof(char) * BUF_SIZE);
            clientBuf = createData();
            clientBuf->type = CHAT;

            inputLength = read(0, inputBuf, BUF_SIZE);
            strcpy(clientBuf->comment.text, inputBuf);
            strcpy(inputCommand, inputBuf);
            token = strtok(inputCommand, separator);
            sscanf(token, "%c-%s", &cmd, param);
            // Send the message to server
            if (cmd == 'C' || cmd == 'c') {
                currentColor = parseColor(param, display);
            } else {
                write(fdToConnect, clientBuf, sizeof(Data));
            }
            // "Quit" command input
            if (strcmp(inputBuf, "Q\n") == 0 || inputLength == 0) {
                break;
            }

            free(clientBuf);
        }
    }
    close(fdToConnect);
}
