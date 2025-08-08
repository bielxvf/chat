#include <task.h>
#include <stdio.h>

void handle_connection(void *fd)
{
}

void taskmain(int argc, char **argv)
{
    int s = netannounce(TCP, "localhost", 7878);

    char client[16];
    int port;
    while (true) {
        if (int fd = netaccept(s, client, &port) >= 0) {
            printf("Connection from %s:%d", client, port);
            taskcreate(handle_connection, &fd);
        }
    }

    taskexit(0);
}
