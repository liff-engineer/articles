#include <cstdio>

#include "nngpp/nngpp.h"
#include "nngpp/protocol/bus0.h"

#include <chrono>
#include <thread>

void fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}

using namespace nng;

int node(int argc, char **argv)
{
    try
    {
        auto sock = bus::open();
        sock.listen(argv[2]);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);

        if (argc >= 3)
        {
            for (int i = 3; i < argc; i++)
            {
                sock.dial(argv[i]);
            }
        }

        std::this_thread::sleep_for(3s);

        printf("%s:SENDING '%s' ONTO BUS\n", argv[1], argv[1]);
        sock.send(view{argv[1], strlen(argv[1]) + 1});

        while (true)
        {
            auto msg = sock.recv();
            printf("%s: RECEIVED '%s' FROM BUS\n", argv[1], msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
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