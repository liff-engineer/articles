#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nng/nng.h"
#include "nng/protocol/reqrep0/req.h"
#include "nng/protocol/reqrep0/rep.h"

#define NODE0 "node0"
#define NODE1 "node1"
#define DATE "DATE"

void fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}

char *date(void)
{
    time_t now = time(&now);
    struct tm *info = localtime(&now);
    char *text = asctime(info);
    text[strlen(text) - 1] = '\0'; // remove '\n'
    return (text);
}

int node0(const char *url)
{
    int sz_date = strlen(DATE) + 1; //'\0'
    nng_socket sock;
    int rv;
    if ((rv = nng_rep_open(&sock)) != 0)
    {
        fatal("nng_rep_open", rv);
    }
    if ((rv = nng_listen(sock, url, NULL, 0)) != 0)
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
        if ((sz == sz_date) && (strcmp(DATE, buf) == 0))
        {
            printf("NODE0: RECEIVED DATE REQUEST\n");
            char *now = date();
            int sz_now = strlen(now) + 1;
            printf("NODE0: SENDING DATE %s\n", now);
            if ((rv = nng_send(sock, now, sz_now, 0)) != 0)
            {
                fatal("nng_send", rv);
            }
        }
        nng_free(buf, sz);
    }
}

int node1(const char *url)
{
    int sz_date = strlen(DATE) + 1; //'\0'
    nng_socket sock;
    int rv;
    char *buf = NULL;
    size_t sz = -1;

    if ((rv = nng_req_open(&sock)) != 0)
    {
        fatal("nng_req_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    printf("NODE1: SENDING DATE REQUEST %s\n", DATE);
    if ((rv = nng_send(sock, DATE, sz_date, 0)) != 0)
    {
        fatal("nng_send", rv);
    }
    if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
    {
        fatal("nng_recv", rv);
    }
    printf("NODE1: RECEIVED DATE %s\n", buf);
    nng_free(buf, sz);
    return nng_close(sock);
}

int main(const int argc, const char **argv)
{
    if ((argc > 1) && (strcmp(NODE0, argv[1]) == 0))
        return (node0(argv[2]));

    if ((argc > 1) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2]));

    fprintf(stderr, "Usage: reqrepImpl %s|%s <URL> ...\n", NODE0, NODE1);
    return (1);
}