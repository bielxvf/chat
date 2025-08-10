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

struct server_ctx;

struct client_ctx {
    struct bufferevent *bev;
    struct sockaddr_in addr;
    struct client_ctx *next;
    struct server_ctx *server;
};

struct server_ctx {
    struct event_base *base;
    struct evconnlistener *listener;
    struct client_ctx *clients;
};

size_t client_count = 0;

void add_client(struct server_ctx *server, struct client_ctx *client)
{
    /* Insert at the start */
    client->server = server;
    client->next = server->clients;
    server->clients = client;
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
        p = &((*p)->next)
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
    char *s = malloc(len + 1);
    if (s == NULL) {
        fprintf(
            stderr,
            "Error allocating memory for broadcast message: %s\n",
            strerror(errno)
        );
        return;
    }
    evbuffer_remove(msg, s, len);
    
    for (
        struct client_ctx *client = server->clients;
        client != NULL;
        client = client->next
    ) {
        if (client != sender) {
            bufferevent_write(client->bev, s, len);
        }
    }
    free(s);
}

/* Read from client + do whatever */
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
        printf(
            "Connection closed: %s:%d\n",
            inet_ntoa(client->addr.sin_addr),
            ntohs(client->addr.sin_port)
        );
        remove_client(server, client);
        bufferevent_free(bev);
        free(client);
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
    struct event_base *base = ctx;
    struct client_ctx *client = malloc(sizeof(struct client_ctx));
    if (client == NULL) {
        fprintf(
            stderr,
            "Error allocating client_ctx: %s.\n",
            strerror(errno)
        );
        return;
    }

    client->bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (client->bev == NULL) {
        fprintf(
            stderr,
            "Error creating bufferevent: %s.\n",
            strerror(errno)
        );
        event_base_loopbreak(base);
        free(client);
        return;
    }

    memcpy(&client->addr, addr, sizeof(client->addr));
    printf("New connection from %s:%d\n",
            inet_ntoa(client->addr.sin_addr),
                ntohs(client->addr.sin_port)
    );

    client_count++;
    printf("Number of clients connected: %ld\n", client_count);
    bufferevent_setcb(client->bev, read_cb, NULL, event_cb, client);
    bufferevent_enable(client->bev, EV_READ | EV_WRITE);
}

#define PORT 8484 /* TODO: cmdline argument */
#define MAX_CLIENTS 1024  /* TODO: cmdline argument */

int main(int argc, char **argv)
{
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
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT); /* TODO: cmdline argument */

    server->listener = evconnlistener_new_bind(
            server->base,
            accept_cb,
            server->base,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
            MAX_CLIENTS, /* TODO: cmdline argument */
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
