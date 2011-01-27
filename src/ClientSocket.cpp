#include "ClientSocket.h"
#include <errno.h>
#include <stdio.h>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

namespace streamsocket
{

int close(SOCKET sock)
{
    return closesocket(sock);
}

bool setSocketFlag(SOCKET sock, int level, int optname, bool value)
{
    BOOL val = value ? TRUE : FALSE;
    return 0 == setsockopt(sock, level, optname, (char*)&val, sizeof(value));
}

const char* getLastErrorMessage()
{
    return gai_strerror(WSAGetLastError());
}

};

#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <poll.h>

namespace streamsocket
{

bool setSocketFlag(SOCKET sock, int level, int optname, bool value)
{
    int val = value ? 1 : 0;
    return 0 == setsockopt(sock, level, optname, &val, sizeof(value));
}

const char* getLastErrorMessage()
{
    return strerror(errno);
}

};

#endif

namespace streamsocket
{

ClientSocket::ClientSocket(SOCKET sockFd)
: sockFd(sockFd), streamBuf(this)
{}

ClientSocket::ClientSocket(const char* host, const char* port)
: sockFd(-1), streamBuf(this)
{
    struct addrinfo hints;
    struct addrinfo* res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;// use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;// fill in my IP for me
    if(getaddrinfo(host, port, &hints, &res))
    {
        throw std::runtime_error(std::string("error getting address info for ") + host + ":" + port + " (" + getLastErrorMessage() + ")");
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockFd < 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("error connecting to ") + host + ":" + port + " (" + getLastErrorMessage() + ")");
    }

    setSocketFlag(sockFd, SOL_SOCKET, SO_REUSEADDR, true);
    setSocketFlag(sockFd, SOL_SOCKET, SO_KEEPALIVE, true);

    if(connect(sockFd, res->ai_addr, res->ai_addrlen))
    {
        freeaddrinfo(res);
        close();
        throw std::runtime_error(std::string("error connecting to ") + host + ":" + port + "(" + getLastErrorMessage() + ")");
    }

    freeaddrinfo(res);
}

void ClientSocket::tcpNoDelay(bool enable)
{
    const bool ret = setSocketFlag(sockFd, IPPROTO_TCP, TCP_NODELAY, enable);
    if(!ret)
    {
        throw std::runtime_error(std::string("error setting TCP_NODELAY: ") + getLastErrorMessage());
    }
}

void ClientSocket::write(const void* data, size_t len)
{
    size_t sent = 0;
    while(sent < len)
    {
        const ssize_t ret = ::send(sockFd, (const char*)data + sent, len - sent, 0);
        if(ret <= 0)
        {
            throw std::runtime_error(std::string("error writing to socket: ") + getLastErrorMessage());
        }
        sent += ret;
    }
}

size_t ClientSocket::read(void* data, size_t len)
{
    const ssize_t ret = ::recv(sockFd, (RecvBufferType)data, len, 0);
    if(ret <= 0)
    {
        throw std::runtime_error(std::string("error reading from socket: ") + getLastErrorMessage());
    }
    return ret;
}

bool ClientSocket::readable()
{
    if(sockFd < 0)
        return false;

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

bool ClientSocket::writable()
{
    if(sockFd < 0)
        return false;

    struct pollfd pfd;
    pfd.fd = sockFd;
    pfd.events = POLLOUT;
    const int ret = poll(&pfd, 1, 0);
    if(ret < 0)
    {
        throw std::runtime_error(std::string("error polling socket: ") + getLastErrorMessage());
    }
    return ret > 0;
}

void ClientSocket::close()
{
    if(sockFd >= 0)
    {
        ::close(sockFd);
    }
    sockFd = -1;
}

ClientSocket::~ClientSocket()
{
    close();
}

std::streambuf* ClientSocket::getStreamBuf()
{
    return &streamBuf;
}


ClientSocket::StreamBuf::StreamBuf(ClientSocket* conn)
: conn(conn)
{
    setp(outBuffer, outBuffer + sizeof(outBuffer) - 1);
}

ClientSocket::StreamBuf::int_type ClientSocket::StreamBuf::overflow(int_type c)
{
    if (!traits_type::eq_int_type(traits_type::eof(), c))
    {
        traits_type::assign(*pptr(), traits_type::to_char_type(c));
        pbump(1);
    }
    return sync() == 0 ? traits_type::not_eof(c): traits_type::eof();
}

std::streamsize ClientSocket::StreamBuf::xsputn(const char* buf, std::streamsize size)
{
    sync();
    conn->write(buf, size);
    return size;
}

int ClientSocket::StreamBuf::sync()
{
    if(pbase() != pptr())
    {
        conn->write(pbase(), pptr() - pbase());
        setp(outBuffer, outBuffer + sizeof(outBuffer) - 1);
    }
    return 1;
}

ClientSocket::StreamBuf::int_type ClientSocket::StreamBuf::underflow()
{
    const size_t got = conn->read(inBuffer, sizeof(inBuffer));
    setg(inBuffer, inBuffer, inBuffer + got);
    return traits_type::to_int_type(*gptr());
}

std::streamsize ClientSocket::StreamBuf::xsgetn(char* dest, std::streamsize size)
{
    std::streamsize numRead = 0;
    char* const beg = gptr();
    char* const end = egptr();
    if(beg < end)
    {
        const std::streamsize avail = end - beg;
        numRead = std::min<std::streamsize>(size, avail);
        char* const newBeg = beg + numRead;
        std::copy(beg, newBeg, dest);
        if(newBeg != end)
        {
            setg(newBeg, newBeg, end);
            return numRead;
        }
    }
    do
    {
        numRead += conn->read(dest + numRead, size - numRead);
    } while(numRead < size);
    return numRead;
}
};

