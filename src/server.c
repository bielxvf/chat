#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdio.h>

#define PORT 7878 /* TODO: cmdline argument */

int main(int argc, char **argv)
{
    int s = socket(AF_INET, SOCK_STREAM, 0); /* TODO: Magic number */
    if (s < 0) return 1; /* TODO: Error msg */

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(s, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0) return 1; /* TODO: Error msg */

    if (listen(s, 5) != 0) return 1; /* TODO: Error msg */ /* TODO: Magic number */

    struct sockaddr_in client_addr;
    int client_addr_sz = sizeof(client_addr);
    int connfd = accept(s, (struct sockaddr*) &client_addr, &client_addr_sz);
    if (connfd < 0) return 1; /* TODO: Error msg */

    char buf[1024];
    while (true) {
        memset(buf, 0, sizeof(buf));
        if (read(connfd, buf, sizeof(buf)) > 0) {
            printf("Received:\n%s\n", buf);
        }
    }

    close(s);

    return 0;
}
