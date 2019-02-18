#include "TLS.h"

#include <Windows.h>

void GetTlsHandshakeInfo(const TLS_HANDSHAKE_PROTO* hsproto, uint8_t* hstype, uint32_t* hslen)
{
	uint8_t handshake[4];
	uint8_t type;
	uint32_t len;

	memcpy(handshake, hsproto->handshake, sizeof(handshake));
	type = handshake[0];
	handshake[0] = 0;
	len = ntohl(*((uint32_t*)(handshake)));

	*hstype = type;
	*hslen = len;
}
