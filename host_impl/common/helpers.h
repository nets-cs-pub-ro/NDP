#ifndef NDP_COMMON_HELPERS_H_
#define NDP_COMMON_HELPERS_H_

#include <arpa/inet.h>
#include <assert.h>
#include "defs.h"
#include "logger.h"
#include <sys/un.h>



#define BYTE_N(p, n)			( ((const unsigned char*) (p))[n] )

#define MAC_PRINTF_FORMAT				"%x:%x:%x:%x:%x:%x"
#define MAC_PRINTF_VALUES(p)			BYTE_N(p, 0), BYTE_N(p, 1), BYTE_N(p, 2), \
										BYTE_N(p, 3), BYTE_N(p, 4), BYTE_N(p, 5)

#define MAKE_IPV4(a,b,c,d)			((a) | (b) << 8 | (c) << 16 | (d) << 24)
#define MY_IPV4_FORMAT				"%d.%d.%d.%d"
#define MY_IPV4_VALUES(x)			(x) << 24 >> 24, (x) << 16 >> 24, (x) << 8 >> 24, (x) >> 24

#ifdef BUILDING_WITH_DPDK
  #define MAC_SRCFLD	 		s_addr
  #define MAC_DSTFLD			d_addr
#else
  #define MAC_SRCFLD	 		ether_shost
  #define MAC_DSTFLD			ether_dhost
#endif

#define NDP_PACKET_HDRS_FORMAT			MAC_PRINTF_FORMAT" "MY_IPV4_FORMAT":%lu > "MAC_PRINTF_FORMAT" "MY_IPV4_FORMAT":%lu seq %u"

//p should be <struct ndp_packet_headers*>
#define NDP_PACKET_HDRS_VALUES(p)		MAC_PRINTF_VALUES(&(p)->eth.MAC_SRCFLD), MY_IPV4_VALUES((p)->ip.saddr), ndp_port_ntoh((p)->ndp.src_port), \
										MAC_PRINTF_VALUES(&(p)->eth.MAC_DSTFLD), MY_IPV4_VALUES((p)->ip.daddr), ndp_port_ntoh((p)->ndp.dst_port), \
										ndp_seq_ntoh((p)->ndp.sequence_number)

#define LOG_NDP_PACKET_HDRS(level, p)	  { \
	  										const struct ndp_packet_headers *x = (const struct ndp_packet_headers*)(p); \
		  									LOG_FMSG(level, NDP_PACKET_HDRS_FORMAT, NDP_PACKET_HDRS_VALUES(x)) \
										  }

// a = newer seq ; b = older seq
/*
#define SEQ_DIFF(a, b)			({ \
									__typeof__ (a) _a = (a); 	\
									__typeof__ (b) _b = (b); 	\
																\
									static_assert(sizeof(_a) == sizeof(_b), "SEQ_DIFF applied on values of different width");	\
									static_assert(sizeof(_a) >= 4, "SEQ_DIFF values aren't at least 32 bits wide");		\
																						\
									__typeof__ (a) _d = _a > _b ? _a - _b : _b - _a; 	\
									( (_a > _b && _d > NDP_MAX_SEQ_DIFF) || (_a < _b && _d <= NDP_MAX_SEQ_DIFF) ) ? NDP_MAX_SEQ_DIFF + 1 : _d; \
								 })
*/

/*
#define DISTANCE(a, b)			({ \
									__typeof__ (a) _a = (a); \
									__typeof__ (b) _b = (b); \
									_a > _b ? _a - _b : _b - _a; \
								})
*/


static inline ndp_sequence_number_t ndp_seq_hton(ndp_sequence_number_t seq)
{
	return htonl(seq);
}

static inline ndp_sequence_number_t ndp_seq_ntoh(ndp_sequence_number_t seq)
{
	return ntohl(seq);
}

static inline ndp_pull_number_t ndp_pull_hton(ndp_pull_number_t pull_number)
{
	return ndp_seq_hton(pull_number);
}

static inline ndp_pull_number_t ndp_pull_ntoh(ndp_pull_number_t pull_number)
{
	return ndp_seq_ntoh(pull_number);
}

static inline ndp_port_number_t ndp_port_hton(ndp_port_number_t port)
{
	return htonl(port);
}

static inline ndp_port_number_t ndp_port_ntoh(ndp_port_number_t port)
{
	return ntohl(port);
}

static inline ndp_recv_window_t ndp_rwnd_hton(ndp_recv_window_t rwnd)
{
	return htonl(rwnd);
}

static inline ndp_recv_window_t ndp_rwnd_ntoh(ndp_recv_window_t rwnd)
{
	return ntohl(rwnd);
}


static inline ndp_timestamp_ns_t tsc_delta_to_ns(ndp_cycle_count_t delta, double tsc_hz)
{
	return delta / tsc_hz * NS_PER_SECOND;
}

static inline ndp_timestamp_ns_t tsc_delta_to_us(ndp_cycle_count_t delta, double tsc_hz)
{
	return delta / tsc_hz * (NS_PER_SECOND / 1000);
}


void exit_msg(int status, const char *format, ...);

void init_lib_instance_name(char *buf, pid_t pid);

//size_t aligned_size(size_t at_least, unsigned int alignment);
//size_t aligned_ring_size(ring_size_t buf_size, unsigned int alignment);
//size_t aligned_ring_buf_size(ring_size_t at_least, unsigned int alignment);

static inline size_t global_comm_ring_size(ring_size_t num_elements)
{
	return offsetof(struct global_communication_ring, inner) +
			RING_SIZE(RING_BUF_OF_CHOICE(struct, ndp_instance_setup_request), num_elements);
}

size_t aligned_ndp_packet_buffer_size(ndp_packet_buffer_size_t mtu);
size_t aligned_ndp_socket_size(const struct shm_parameters *shm_params);
size_t shm_bytes_until_first_socket(const struct shm_parameters *shm_params);
size_t shm_size(const struct shm_parameters *shm_params);

//sets everything up except the instance id
void prepare_ndp_instance_info(void *shm_begins, const struct shm_parameters *shm_params,
		struct ndp_instance_info *instance_info);

void init_ndp_socket_info(const struct ndp_instance_info *instance_info, ndp_socket_index_t socket_index,
		struct ndp_socket_info *socket_info);


static inline struct ndp_socket* get_ndp_socket(struct ndp_instance_info *instance_info, ndp_socket_index_t index)
{
	#if DEBUG_LEVEL > 0
		if(UNLIKELY(index >= instance_info->shm_params.num_sockets))
			LOG_MSG_DO(1, exit(1), "index >= instance_data->shm_params.num_sockets");
	#endif

	return (struct ndp_socket*)(instance_info->sockets_begin + index * instance_info->aligned_socket_size);
}


static inline struct ndp_packet_buffer* get_tx_packet_buffer(struct ndp_socket_info *socket_info, ndp_packet_buffer_index_t index)
{
	#if DEBUG_LEVEL > 0
		if(UNLIKELY(index >= socket_info->num_tx_packet_buffers))
			LOG_MSG_DO(1, exit(1), "index >= instance_data->shm_params.num_tx_packet_buffers");
	#endif

	return (struct ndp_packet_buffer*)(socket_info->tx_packet_buffers + index * socket_info->aligned_packet_buffer_size);
}


static inline struct ndp_packet_buffer* get_rx_packet_buffer(struct ndp_socket_info *socket_info, ndp_packet_buffer_index_t index)
{
	#if DEBUG_LEVEL > 0
		if(UNLIKELY(index >= socket_info->num_rx_packet_buffers))
			LOG_MSG_DO(1, exit(1), "index >= instance_data->shm_params.num_rx_packet_buffers");
	#endif

	return (struct ndp_packet_buffer*)(socket_info->rx_packet_buffers + index * socket_info->aligned_packet_buffer_size);
}

void prepare_ndp_packet_headers(ndp_net_addr_t saddr, ndp_net_addr_t daddr, ndp_port_number_t sport,
		ndp_port_number_t dport, void *smac, void *dmac, struct ndp_packet_headers *h);

ndp_packet_buffer_index_t find_tx_pkb_for_seq(struct ndp_socket_info *socket_info, ndp_packet_buffer_index_t num_pkbs,
												ndp_sequence_number_t seq_in_network_order);

void log_packet(void *p);

//estimates time to put frame on the wire
double ns_per_frame_len(uint32_t len);
ndp_cycle_count_t cycles_per_frame_len(uint32_t len, double tsc_hz);

#if NDP_HARDCODED_SAME_NEXT_HOP
	int get_mac_address(ndp_net_addr_t ip, void *buf, ndp_net_addr_t local_addr);
#else
	int get_mac_address(ndp_net_addr_t ip, void *buf);
#endif


#ifdef NDP_BUILDING_FOR_XEN

	size_t num_pages_for_bytes(size_t num_bytes);
	size_t num_meta_pages(size_t num_pages);

#endif


static inline ndp_checksum_t nack_seq_checksum(ndp_sequence_number_t seq_in_network_order)
{
	return ndp_seq_ntoh(seq_in_network_order) & ((1 >> 16) - 1);
}


//zzzzzzzzzzzzzzzz

#define ADDR_10_0_0_6					100663306	// 10.0.0.6 in network order
#define ADDR_10_0_0_10					167772170
#define NDP_DEF_REMOTE_PORT				3892510720	// 1000 for uint32_t in network order

void aux_sleep(long ns);

#endif /* NDP_COMMON_HELPERS_H_ */
