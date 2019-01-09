#ifndef NDP_COMMON_NDP_HEADER_H_
#define NDP_COMMON_NDP_HEADER_H_


#include "ring.h"
#include <stdint.h>


//16 bits are used just to have a round, 20-byte ndp_header; not anymore apparently
typedef uint16_t ndp_header_flags_t;

//WATCH OUT FOR ENDIANESS!!

#define NDP_HEADER_FLAG_DATA							(1 << 0)
#define NDP_HEADER_FLAG_ACK								(1 << 1)
#define NDP_HEADER_FLAG_PULL							(1 << 2)
#define NDP_HEADER_FLAG_NACK							(1 << 3)
#define NDP_HEADER_FLAG_KEEP_ALIVE						(1 << 4)
#define NDP_HEADER_FLAG_PACER_NUMBER_VALID				(1 << 5)
#define NDP_HEADER_FLAG_FIN								(1 << 6)
#define NDP_HEADER_FLAG_CHOPPED							(1 << 7)
#define NDP_HEADER_FLAG_SYN								(1 << 8)


typedef uint16_t ndp_checksum_t;

typedef uint32_t ndp_port_number_t;

typedef	uint32_t ndp_sequence_number_t;
typedef uint32_t ndp_pull_number_t;
typedef	uint32_t ndp_pacer_number_t;
typedef uint32_t ndp_recv_window_t;


struct ndp_header
{
	ndp_header_flags_t flags;
	ndp_checksum_t checksum;

	ndp_port_number_t src_port;
	ndp_port_number_t dst_port;

	union {
		ndp_sequence_number_t sequence_number;			//for data, fin, ack & nack segments
		ndp_pull_number_t pull_number;					//for pull segments
	};

	union {
		ndp_pacer_number_t pacer_number_echo;			//for data segments
		ndp_pacer_number_t pacer_number;				//for pull segments

		//TODO should the recv_window be sent for NACKs also? if so things become a bit tricky,
		//because NACKs are currently emitted directly by the core

		//as for TCP, the recv_window edge is sequence_number + recv_window
		ndp_recv_window_t recv_window;					//for ack segments
	};

} __attribute__((packed));

DEF_RINGS(struct ndp_header, struct, ndp_header)


#endif /* NDP_COMMON_NDP_HEADER_H_ */
