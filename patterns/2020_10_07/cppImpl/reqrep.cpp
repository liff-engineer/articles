// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
#include <ctime>
#include <cstdio>
#include "nngpp/nngpp.h"
#include "nngpp/protocol/req0.h"
#include "nngpp/protocol/rep0.h"

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

using namespace nng;
int node0(const char *url)
{
    try
    {
        auto sock = rep::open();
        sock.listen(url);
        while (true)
        {
            auto msg = sock.recv();
            if (msg == "DATE")
            {
                printf("NODE0: RECEIVED DATE REQUEST\n");
                char *now = date();
                printf("NODE0: SENDING DATE %s\n", now);
                sock.send(view{now, strlen(now) + 1});
            }
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}

int node1(const char *url)
{
    try
    {
        auto sock = req::open();
        sock.dial(url);
        printf("NODE1: SENDING DATE REQUEST %s\n", DATE);
        sock.send("DATE");
        auto msg = sock.recv();
        printf("NODE1: RECEIVED DATE %s\n", msg.data<char>());
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
    return 1;
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