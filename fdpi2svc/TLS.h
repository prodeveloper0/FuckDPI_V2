#pragma once
#include <stdint.h>
#include <memory.h>

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable: 4200)
struct TLS_HEADER
{
	uint8_t type;
	uint16_t version;
	uint16_t length;
	char data[];
};

struct TLS_HANDSHAKE_PROTO
{
	uint8_t handshake[4];
	uint16_t version;
	char data[];
};
#pragma warning(pop)
#pragma pack(pop)

void GetTlsHandshakeInfo(const TLS_HANDSHAKE_PROTO* hsproto, uint8_t* hstype, uint32_t* hslen);
