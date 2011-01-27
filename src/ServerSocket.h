#pragma once

#include "ClientSocket.h"

namespace streamsocket
{

class ServerSocket : boost::noncopyable
{
public:
    ServerSocket(const std::string& host, const std::string& port);

    virtual ~ServerSocket();

    SOCKET accept();

private:
    SOCKET sockFd;
};

};

