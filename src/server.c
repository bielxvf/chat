#define ENET_IMPLEMENTATION
#include <enet.h>
#include <stdio.h>

#define CHECKERR(x, y) if (x != 0) { y; return 1; }

int main(int argc, char **argv)
{
    CHECKERR(enet_initialize(), printf("Error initializing ENet.\n"))
    return 0;
}
