#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"

#include <chrono>
#include <thread>

void fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}

int node(int argc, char **argv)
{
    nng_socket sock;
    int rv;
    size_t sz;

    if ((rv = nng_bus_open(&sock)) != 0)
    {
        fatal("nng_bus_open", rv);
    }

    if ((rv = nng_listen(sock, argv[2], NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }

    using namespace std::chrono_literals;

    std::this_thread::sleep_for(3s);

    if (argc >= 3)
    {
        for (int i = 3; i < argc; i++)
        {
            if ((rv = nng_dial(sock, argv[i], NULL, 0)) != 0)
            {
                fatal("nng_dial", rv);
            }
        }
    }

    std::this_thread::sleep_for(3s);

    sz = strlen(argv[1]) + 1;
    printf("%s:SENDING '%s' ONTO BUS\n", argv[1], argv[1]);
    if ((rv = nng_send(sock, argv[1], sz, 0)) != 0)
    {
        fatal("nng_send", rv);
    }

    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            if (rv == NNG_ETIMEDOUT)
            {
                fatal("nng_recv", rv);
            }
        }
        printf("%s: RECEIVED '%s' FROM BUS\n", argv[1], buf);
        nng_free(buf, sz);
    }
    return nng_close(sock);
}

int main(int argc, char **argv)
{
    if (argc >= 3)
    {
        return node(argc, argv);
    }
    fprintf(stderr, "Usage: busImpl <NODE_NAME> <URL> <URL> ...\n");
    return 1;
}