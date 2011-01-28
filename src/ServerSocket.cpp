#include "ServerSocket.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

namespace streamsocket
{

extern int close(SOCKET sock);

};

#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <poll.h>
#endif

namespace streamsocket
{

ServerSocket::ServerSocket(const std::string& host, const std::string& port)
: sockFd(-1)
{
    struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;// use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;// fill in my IP for me
    if(getaddrinfo(host.c_str(), port.c_str(), &hints, &res))
    {
        throw std::runtime_error(std::string("error in getaddrinfo for command port: ") + strerror(errno));
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockFd < 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("error creating socket: ") + strerror(errno));
    }
    setSocketFlag(sockFd, SOL_SOCKET, SO_REUSEADDR, true);
    if(bind(sockFd, res->ai_addr, res->ai_addrlen))
    {
        freeaddrinfo(res);
        close(sockFd);
        throw std::runtime_error(std::string("error binding port: ") + strerror(errno));
    }
    if(listen(sockFd, 5))
    {
        freeaddrinfo(res);
        close(sockFd);
        throw std::runtime_error(std::string("error listening port: ") + strerror(errno));
    }

    freeaddrinfo(res);
}

ServerSocket::~ServerSocket()
{
    if(sockFd >= 0)
    {
        close(sockFd);
    }
}

SOCKET ServerSocket::accept()
{
    SOCKET ret = ::accept(sockFd, NULL, NULL);
    if(ret < 0)
    {
        throw std::runtime_error(std::string("error accepting new client: ") + strerror(errno));
    }
    return ret;
}

bool ServerSocket::ready()
{
    struct pollfd pfd;
    pfd.fd = sockFd;
    pfd.events = POLLIN;
    const int ret = poll(&pfd, 1, 0);
    if(ret < 0)
    {
        throw std::runtime_error(std::string("error polling socket: ") + getLastErrorMessage());
    }
    return ret > 0;
}

};

