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
		addrV4.
	}
	else {

	}
}