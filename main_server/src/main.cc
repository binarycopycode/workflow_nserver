#include "main_server.h"

int main(int argc, char *argv[])
{
    unsigned short port;
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);
    int server_start_status = main_server::run(port);
    if (server_start_status < 0)
    {
        std::cerr << "main_server start failed" << std::endl;
    }
    return 0;
}