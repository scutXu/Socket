#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

union EndPoint
{
public:
	EndPoint();
	EndPoint(const char * ipAddress, uint16_t port);

	sockaddr_in addrV4;
	sockaddr_in6 addrV6;
	sockaddr addr;
};
