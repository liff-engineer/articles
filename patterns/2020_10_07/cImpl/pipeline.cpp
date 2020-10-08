#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"

#include <chrono>
#include <thread>

#define NODE0 "node0"
#define NODE1 "node1"

void fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}

int node0(const char *url)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_pull_open(&sock)) != 0)
    {
        fatal("nng_pull_open", rv);
    }
    if ((rv = nng_listen(sock, url, nullptr, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        uint64_t val;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        printf("NODE0:RECEIVED \"%s\"\n", buf);
        nng_free(buf, sz);
    }
}
int node1(const char *url, const char *msg)
{
    int sz_msg = strlen(msg) + 1; //'\0'
    nng_socket sock;
    int rv;
    size_t sz;
    if ((rv = nng_push_open(&sock)) != 0)
    {
        fatal("nng_push_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    printf("NODE1: SENDING \"%s\"\n", msg);
    if ((rv = nng_send(sock, (void *)msg, sz_msg, 0)) != 0)
    {
        fatal("nng_send", rv);
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);

    nng_close(sock);
    return 0;
}
int main(const int argc, const char **argv)
{
    if ((argc > 1) && (strcmp(NODE0, argv[1]) == 0))
        return (node0(argv[2]));
    if ((argc > 2) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2], argv[3]));
    fprintf(stderr, "Usage: pipelineImpl %s|%s <URL> <ARG> ...\n", NODE0, NODE1);
    return 1;
}