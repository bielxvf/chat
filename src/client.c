#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 7878 /* TODO: cmdline argument */

int main(int argc, char **argv)
{
    int s = socket(AF_INET, SOCK_STREAM, 0); /* TODO: Magic number */
    if (s < 0) return 1; /* TODO: Error msg */

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* TODO: cmdline argument */
    server_addr.sin_port = htons(PORT);

    if (connect(s, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0) return 1; /* TODO: Error msg */

    /* Use s to write and read */
    while (true) {
        char buf[5] = "Test";
        write(s, buf, sizeof(buf));
    }

    close(s);

    return 0;
}
