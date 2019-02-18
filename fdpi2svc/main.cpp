#include <iostream>
#include <thread>

#include <windivert.h>

#include "TLS.h"

const char* const gl_WinDivertFilter = "outbound && !loopback && (ip || ipv6) && tcp.PayloadLength >= 11";

const UINT gl_MinBufferSize = 4096;								// Minimum buffer size is 4KB
const UINT gl_MaxBufferSize = 67108864;							// Maximum buffer size is 64MB
const int gl_DefaultNumberOfThread = 2;							// Default 2 Thread(s)
const UINT gl_DefaultBufferSize = 65536;						// Default 64KB

static int gl_NumberOfThread = gl_DefaultNumberOfThread;		// Default 2 Thread(s)
static UINT gl_BufferSize = gl_DefaultBufferSize;				// Default 64KB

void PrintHelp()
{
	printf("Usage: fdpi2svc [--help] [--numThread=value] [--bufferSize=value]\n"
		   "\n"
		   "--help\tShow usages\n"
		   "--numThread\tNumber of threads to process packet (Minimum 1, Default %d)\n"
		   "--bufferSize\tBuffer size of packet per threads (Minimum %u, Maximum %u, Default %u)\n"
		   "\n",
		   gl_DefaultNumberOfThread, gl_MinBufferSize, gl_MaxBufferSize, gl_DefaultBufferSize);
	exit(0);
}

void ParseOption(int argc, char** argv)
{
	int NumberOfThread = gl_NumberOfThread;
	UINT BufferSize = gl_BufferSize;

	for(int i = 1;i < argc;++i)
	{
		char opt[64] = {0x00,};
		char* opt_key;
		char* opt_value;
		char* opt_context;

		if(sscanf_s(argv[i], "--%s", opt, sizeof(opt)) <= 0)
			goto param_exception;

		opt_key = strtok_s(opt, "=", &opt_context);
		opt_value = strtok_s(NULL, "=", &opt_context);

		if(!strcmp(opt_key, "help"))
		{
			PrintHelp();
		}
		else if(!strcmp(opt_key, "numThread"))
		{
			if(opt_value == NULL)
				goto param_exception;

			int v = atoi(opt_value);
			if(v < 1)
				goto param_exception;
			NumberOfThread = v;
		}
		else if(!strcmp(opt_key, "bufferSize"))
		{
			if(opt_value == NULL)
				goto param_exception;

			int v = atoi(opt_value);
			if(!(v >= gl_MinBufferSize && v <= gl_MaxBufferSize))
				goto param_exception;
			BufferSize = v;
		}
		else
		{
			param_exception:
			printf("Incorrect paramater\n");
			exit(-1);
		}
	}

	gl_NumberOfThread = NumberOfThread;
	gl_BufferSize = BufferSize;
}

int FilterPacket(HANDLE handle, UINT bufsz)
{
	WINDIVERT_ADDRESS addr;
	char* packet;
	UINT packetLen;

	// Allocate buffer memory
	if((packet = (char*)malloc(bufsz)) == NULL)
	{
		printf("Cannot allcate packet buffer.\n");
		return -1;
	}

	// Capture-Modify-Reinject Loop
	while(WinDivertRecv(handle, packet, bufsz, &addr, &packetLen) == TRUE)
	{
		// Modify - Spliting TLS ClientHello
		PWINDIVERT_IPHDR iphdr;
		PWINDIVERT_IPV6HDR ipv6hdr;
		PWINDIVERT_TCPHDR tcphdr;
		char* payload;
		UINT payloadLen;
		UINT16* outputPacketLen;
		
		// IPv4, IPv6 packets only
		if(WinDivertHelperParsePacket(packet, packetLen, &iphdr, NULL, NULL, NULL, &tcphdr, NULL, (PVOID*)&payload, &payloadLen))
			outputPacketLen = &iphdr->Length;
		else if(WinDivertHelperParsePacket(packet, packetLen, NULL, &ipv6hdr, NULL, NULL, &tcphdr, NULL, (PVOID*)&payload, &payloadLen))
			outputPacketLen = &ipv6hdr->Length;
		else
			goto bypass; // Bypassing other packets

		TLS_HEADER* tls_header = (TLS_HEADER*)payload; // TLS Header
		if(tls_header->type == 0x16) // It's TLS handshake (0x16)
		{
			TLS_HANDSHAKE_PROTO* tls_hsproto = (TLS_HANDSHAKE_PROTO*)tls_header->data; // TLS Handshake Protocol
			uint8_t hstype;
			uint32_t hslen;

			GetTlsHandshakeInfo(tls_hsproto, &hstype, &hslen);
			if(hstype == 0x01) // It's TLS ClientHello (0x01)
			{
				// Split ClientHello and re-inject packets
				UINT fragment_size = payloadLen / 2;

				// First framgment
				*outputPacketLen = htons(ntohs(*outputPacketLen) - payloadLen + fragment_size);
				WinDivertHelperCalcChecksums(packet, packetLen - payloadLen + fragment_size, &addr, 0);
				WinDivertSend(handle, packet, packetLen - payloadLen + fragment_size, &addr, NULL);

				// Second fragment
				*outputPacketLen = htons(ntohs(*outputPacketLen) - (2 * fragment_size) + payloadLen);
				memmove(payload, payload + fragment_size, payloadLen - fragment_size);
				tcphdr->SeqNum = htonl(ntohl(tcphdr->SeqNum) + fragment_size);
				WinDivertHelperCalcChecksums(packet, packetLen, &addr, 0);
				WinDivertSend(handle, packet, packetLen, &addr, NULL);
				continue;
			}
		}

		// Reinject - Bypass
		bypass:
		WinDivertSend(handle, packet, packetLen, &addr, NULL);
	}

	printf("Thread is terminated. error=%d\n", GetLastError());
	free(packet);

	return 0;
}

int main(int argc, char** argv)
{
	HANDLE handle;	
	
	printf("FuckDPI Version 2\n"
		   "Fuck the government's HTTPS SNI field inspection.\n"
		   "\n");

	ParseOption(argc, argv);

	printf("numThread=%d\n"
		   "bufferSize=%u\n"
		   "\n",
		   gl_NumberOfThread, gl_BufferSize);

	handle = WinDivertOpen(gl_WinDivertFilter, WINDIVERT_LAYER_NETWORK, 0, 0);
	if(handle == INVALID_HANDLE_VALUE)
	{
		printf("fdpi2svc cannot be started. error=%d\n", GetLastError());
		return -1;
	}

	for(int i = 0;i < gl_NumberOfThread;++i)
		std::thread(FilterPacket, handle, gl_BufferSize).detach();

	printf("All process is done. Minimize this window.\n");
	printf("Press return key to exit...");
	fgetc(stdin);
	
	WinDivertClose(handle);

	return 0;
}
