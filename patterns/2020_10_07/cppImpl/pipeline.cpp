#include "nngpp/nngpp.h"
#include "nngpp/protocol/pull0.h"
#include "nngpp/protocol/push0.h"
#include <chrono>
#include <thread>
#include <cstdio>

#define NODE0 "node0"
#define NODE1 "node1"

using namespace nng;

int node0(const char *url)
{
    try
    {
        socket sock = pull::open();
        sock.listen(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("NODE0:RECEIVED \"%s\"\n", msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
int node1(const char *url, const char *msg)
{
    try
    {
        socket sock = push::open();
        sock.dial(url);

        printf("NODE1: SENDING \"%s\"\n", msg);
        sock.send(view{msg, strlen(msg) + 1});

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);
        return 1;
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
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