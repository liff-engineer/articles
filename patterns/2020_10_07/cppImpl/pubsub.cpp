#include <ctime>
#include <cstdio>
#include "nngpp/nngpp.h"
#include "nngpp/protocol/pub0.h"
#include "nngpp/protocol/sub0.h"

#include <chrono>
#include <thread>

#define SERVER "server"
#define CLIENT "client"

char *date(void)
{
    time_t now = time(&now);
    struct tm *info = localtime(&now);
    char *text = asctime(info);
    text[strlen(text) - 1] = '\0'; // remove '\n'
    return (text);
}

using namespace nng;
int server(const char *url)
{
    try
    {
        auto sock = pub::open();
        sock.listen(url);
        while (true)
        {
            char *now = date();
            printf("SERVER: PUBLISHING DATE %s\n", now);
            sock.send(view{now, strlen(now) + 1});

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}

int client(const char *url, const char *name)
{
    try
    {
        auto sock = sub::open();
        nng::sub::set_opt_subscribe(sock, "");
        sock.dial(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("CLIENT (%s): RECEIVED %s\n", name, msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}

int main(const int argc, const char **argv)
{
    if ((argc >= 2) && (strcmp(SERVER, argv[1]) == 0))
        return (server(argv[2]));

    if ((argc >= 3) && (strcmp(CLIENT, argv[1]) == 0))
        return (client(argv[2], argv[3]));

    fprintf(stderr, "Usage: pubsubImpl %s|%s <URL> ...\n", SERVER, CLIENT);
    return (1);
}