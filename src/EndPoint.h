#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

union EndPoint
{
	EndPoint();
	EndPoint(const char * ipAddress, uint16_t port);

    size_t getAddressLength();
    
	sockaddr_in addr4;
	sockaddr_in6 addr6;
	sockaddr addr;
};
