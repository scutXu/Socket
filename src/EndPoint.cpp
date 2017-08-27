#include "EndPoint.h"
#include <string.h>
#include <arpa/inet.h>

EndPoint::EndPoint()
{
	memset(this, 0, sizeof(*this));
}

EndPoint::EndPoint(const char * ipAddress, uint16_t port)
{
	memset(this, 0, sizeof(*this));
	if (strchr(ipAddress, ':') == NULL) {
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(port);
		inet_pton(AF_INET, ipAddress, &addr4.sin_addr);
	}
	else {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
		inet_pton(AF_INET6, ipAddress, &addr6.sin6_addr);
	}
}