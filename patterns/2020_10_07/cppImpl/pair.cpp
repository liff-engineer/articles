#include <cstdio>
#include "nngpp/nngpp.h"
#include "nngpp/protocol/pair1.h"

#include <chrono>
#include <thread>

#define NODE0 "node0"
#define NODE1 "node1"

using namespace nng;

int send_recv(nng::socket_view sock, const char *name)
{
    nng::set_opt_recv_timeout(sock, 3000);
    while (true)
    {
        printf("%s: SENDING \"%s\"\n", name, name);
        sock.send(view{name, strlen(name) + 1});

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);

        auto msg = sock.recv();
        printf("%s: RECEIVED \"%s\"\n", name, msg.data<char>());
    }
}

int node0(const char *url)
{
    try
    {
        auto sock = pair::open();
        sock.listen(url);
        return send_recv(sock, "node0");
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
        auto sock = pair::open();
        sock.dial(url);
        return send_recv(sock, "node1");
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

    if ((argc > 1) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2]));

    fprintf(stderr, "Usage: pairImpl %s|%s <URL> ...\n", NODE0, NODE1);
    return (1);
}