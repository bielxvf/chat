#define ENET_IMPLEMENTATION
#include <enet.h>
#include <stdio.h>

#define MAX_CLIENTS 32
#define CHANNEL_LIMIT 2
#define IN_BANDWIDTH 0 /* Unlimited */
#define OUT_BANDWIDTH 0 /* Unlimited */
#define TIMEOUT 3600

int main(int argc, char **argv)
{
    if (enet_initialize() != 0) return 1; /* TODO: Error msg */

    ENetAddress addr = {0};
    addr.host = ENET_HOST_ANY;
    addr.port = 7878; /* TODO: Accept commandline arguments */

    ENetHost *server = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_LIMIT, IN_BANDWIDTH, OUT_BANDWIDTH);
    if (server == NULL) return 1; /* TODO: Error msg */

    ENetEvent event;

    int v;
    for (v = enet_host_service(server, &event, TIMEOUT); v > 0; ) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            printf("Client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
            event.peer->data = "Client info";
        } break;
        case ENET_EVENT_TYPE_RECEIVE: {
            printf("%lu bytes received from %s on channel %u.\n", event.packet->dataLength, event.peer->data, event.channelID);
        } break;
        case ENET_EVENT_TYPE_DISCONNECT: {
            printf("Client %s disconnected.\n", event.peer->data);
            event.peer->data = NULL;
        } break;
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
            printf("Client %s timed out.\n", event.peer->data);
        } break;
        case ENET_EVENT_TYPE_NONE: break;
        }
    }
    if (v < 0) return 1; /* TODO: Error msg */

    enet_host_destroy(server);
    enet_deinitialize();

    return 0;
}
