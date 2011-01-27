#pragma once

#include <boost/noncopyable.hpp>
#include <iostream>

namespace streamsocket
{

#ifdef _WIN32

typedef int ssize_t;
typedef char* RecvBufferType;

#else

typedef int SOCKET;
typedef void* RecvBufferType;

#endif

bool setSocketFlag(SOCKET sock, int level, int optname, bool value);

const char* getLastErrorMessage();

class ClientSocket : boost::noncopyable
{
public:
    ClientSocket(const char* host, const char* port);

    void tcpNoDelay(bool enable);

    void write(const void* data, size_t len);

    size_t read(void* data, size_t len);

    virtual ~ClientSocket();

    std::streambuf* getStreamBuf();

    class StreamBuf : public std::streambuf
    {
    public:
        StreamBuf(ClientSocket* conn);

        int_type overflow(int_type c);

        std::streamsize xsputn(const char* buf, std::streamsize size);

        int sync();

        int_type underflow();

        std::streamsize xsgetn(char* dest, std::streamsize size);

    private:
        char outBuffer[1400];
        char inBuffer[1400];
        ClientSocket* conn;
    };

private:
    SOCKET sockFd;
    StreamBuf streamBuf;
};

};

