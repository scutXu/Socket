#include "EndPoint.h"
#include "Log.h"
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
		int status = inet_pton(AF_INET, ipAddress, &addr4.sin_addr);
        if(status != 1) {
            Log("error when parsing ipv4 address:%s", ipAddress);
        }
	}
	else {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
		int status = inet_pton(AF_INET6, ipAddress, &addr6.sin6_addr);
        if(status != 1) {
            Log("error when parsing ipv6 address:%s", ipAddress);
        }
	}
}

size_t EndPoint::getAddressLength()
{
    if (addr.sa_family == AF_INET) {
        return sizeof(addr4);
    }
    else {
        return sizeof(addr6);
    }
}
