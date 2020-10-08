#include <time.h>
#include <cstdio>
#include <ctime>
#include "nngpp/nngpp.h"
#include "nngpp/protocol/survey0.h"
#include "nngpp/protocol/respond0.h"

#define SERVER "server"
#define CLIENT "client"
#define DATE "DATE"

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
        auto sock = survey::open();
        survey::set_opt_survey_time(sock, 1000);
        sock.listen(url);

        while (true)
        {
            printf("SERVER: SENDING DATE SURVEY REQUEST\n");
            sock.send("DATE");

            while (true)
            {
                try
                {
                    auto msg = sock.recv();
                    printf("SERVER: RECEIVED \"%s\" SURVEY RESPONSE\n", msg.data<char>());
                }
                catch (const nng::exception &e)
                {
                    if (e.get_error() == nng::error::timedout)
                        break;
                    else
                    {
                        printf("%s: %s\n", e.who(), e.what());
                        return 1;
                    }
                }
            }
            printf("SERVER:SURVERY COMPLETE\n");
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
        auto sock = respond::open();
        sock.dial(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("CLIENT (%s): RECEIVED \"%s\" SURVERY REQUEST\n", name, msg.data<char>());
            char *now = date();
            printf("CLIENT (%s): SENDING DATE SURVEY RESPONSE\n", name);
            sock.send(view{now, strlen(now) + 1});
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

    fprintf(stderr, "Usage: surveyImpl %s|%s <URL> <ARG>...\n", SERVER, CLIENT);
    return (1);
}