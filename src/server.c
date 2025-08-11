#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

struct server_ctx;

/* Linked list */
struct client_ctx {
    struct bufferevent *bev;
    struct sockaddr_in addr;
    struct client_ctx *next;
    struct server_ctx *server;
};

struct server_ctx {
    struct event_base *base;
    struct evconnlistener *listener;
    struct client_ctx *clients; /* First of the linked list */
};

static size_t client_count = 0;

void add_client(struct server_ctx *server, struct client_ctx *client)
{
    /* Insert at the start */
    client->server = server;
    client->next = server->clients;
    server->clients = client;
    client_count++;
}

void remove_client(struct server_ctx *server, struct client_ctx *client)
{
    if (server == NULL) {
        /* TODO: Maybe crash? */
    }
    if (client == NULL) {
        /* TODO: Maybe crash? */
    }
    for (
        struct client_ctx **p = &server->clients;
        *p != NULL;
        p = &((*p)->next) /* TODO: Ugly code. */
    ) {
        if (*p == client) {
            *p = client->next;
            break;
        }
    }

    bufferevent_free(client->bev);
    free(client);
    client_count--;
}

void broadcast_message(
    struct server_ctx *server,
    struct evbuffer *msg,
    struct client_ctx *sender
)
{
    size_t len = evbuffer_get_length(msg);
    if (len == 0) return;
    char *s = malloc(len);
    if (s == NULL) {
        fprintf(
            stderr,
            "Error allocating memory for broadcast message: %s\n",
            strerror(errno)
        );
        return;
    }
    evbuffer_remove(msg, s, len);
    char *msg_header;
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender->addr.sin_addr), ipstr, INET_ADDRSTRLEN);
    asprintf(&msg_header, "(%s):(%d) ", ipstr, ntohs(sender->addr.sin_port));

    for (
        struct client_ctx *client = server->clients;
        client != NULL;
        client = client->next
    ) {
        if (client != sender) {
            bufferevent_write(client->bev, msg_header, strlen(msg_header));
        } else {
            char *youstr = "(YOU) ";
            bufferevent_write(client->bev, youstr, strlen(youstr));
        }
        bufferevent_write(client->bev, s, len);
    }

    free(msg_header);
    free(s);
}

void read_cb(struct bufferevent *bev, void *ctx)
{
    struct client_ctx *client = ctx;
    struct server_ctx *server = client->server;
    struct evbuffer *input = bufferevent_get_input(bev);

    broadcast_message(server, input, client);
}

void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    struct client_ctx *client = ctx;
    struct server_ctx *server = client->server;

    if (events & BEV_EVENT_ERROR) {
        fprintf(
            stderr,
            "Error: %s\n",
            evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
        );
    }

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client->addr.sin_addr), ipstr, INET_ADDRSTRLEN);
        uint16_t port = ntohs(client->addr.sin_port);
        printf(
            "Connection closed: %s:%d\n",
            ipstr,
            port
        );
        remove_client(server, client);

        char *dcstr;
        asprintf(&dcstr, "\e[0;34m(%s)\e[0m:\e[0;30m(%d)\e[0m has left the chat.\n", ipstr, port);
        for (
            struct client_ctx *client = server->clients;
            client != NULL;
            client = client->next
        ) {
            bufferevent_write(client->bev, dcstr, strlen(dcstr));
        }
        free(dcstr);
        printf("Number of clients connected: %zu\n", client_count);
    }
}

/* Handle connection */
void accept_cb(
    struct evconnlistener *listener,
    evutil_socket_t fd,
    struct sockaddr *addr,
    int socklen,
    void *ctx
)
{
    struct server_ctx *server = ctx;
    struct client_ctx *client = malloc(sizeof(struct client_ctx));
    if (client == NULL) {
        fprintf(
            stderr,
            "Error allocating client_ctx: %s.\n",
            strerror(errno)
        );
        return;
    }

    client->bev = bufferevent_socket_new(server->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (client->bev == NULL) {
        fprintf(
            stderr,
            "Error creating bufferevent: %s.\n",
            strerror(errno)
        );
        event_base_loopbreak(server->base);
        return;
    }
    add_client(server, client);

    memcpy(&client->addr, addr, sizeof(client->addr));
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->addr.sin_addr), ipstr, INET_ADDRSTRLEN);
    uint16_t port = ntohs(client->addr.sin_port);
    printf(
        "New connection from %s:%d\n",
        ipstr,
        port
    );

    printf("Number of clients connected: %ld\n", client_count);

    char *connstr;
    asprintf(&connstr, "(%s):(%d) has joined the chat.\n", ipstr, port);
    char *youstr = "(YOU) have joined the chat.\n";
    for (
        struct client_ctx *it = server->clients;
        it != NULL;
        it = it->next
    ) {
        if (it != client) {
            bufferevent_write(it->bev, connstr, strlen(connstr));
        } else {
            bufferevent_write(it->bev, youstr, strlen(youstr));
        }
    }
    free(connstr);
    bufferevent_setcb(client->bev, read_cb, NULL, event_cb, client);
    bufferevent_enable(client->bev, EV_READ | EV_WRITE);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(
            stderr,
            "Usage: %s <ip> <port> <max_clients>\n",
            argv[0]
        );
        return 1;
    }

    uint16_t port = 0;
    if (sscanf(argv[2], "%hu", &port) <= 0) {
        fprintf(
            stderr,
            "Error: Could not parse '%s' into a size_t\n",
            argv[2]
        );
    }

    size_t max_clients = 0;
    if (sscanf(argv[3], "%zu", &max_clients) <= 0) {
        fprintf(
            stderr,
            "Error: Could not parse '%s' into a size_t\n",
            argv[3]
        );
    }

    struct server_ctx *server = malloc(sizeof(struct server_ctx));
    if (server == NULL) {
        fprintf(stderr, "Error creating server context: %s.\n", strerror(errno));
        return errno;
    }

    server->base = event_base_new();
    if (server->base == NULL) {
        fprintf(stderr, "Error creating event base: %s.\n", strerror(errno));
        return errno;
    }

    server->clients = NULL;

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(sin.sin_addr));
    sin.sin_port = htons(port);

    server->listener = evconnlistener_new_bind(
            server->base,
            accept_cb,
            server,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
            max_clients,
            (struct sockaddr*) &sin,
            sizeof(sin)
    );
    if (server->listener == NULL) {
        fprintf(stderr, "Error creating listener: %s.\n", strerror(errno));
        return errno;
    }

    /* Event loop */
    event_base_dispatch(server->base);

    /* Cleanup */
    evconnlistener_free(server->listener);
    event_base_free(server->base);
    free(server);
    return 0;
}
