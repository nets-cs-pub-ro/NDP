#ifndef NDP_COMMON_DEFS_H_
#define NDP_COMMON_DEFS_H_

#include "config.h"
#include <linux/ip.h>
#include "ndp_header.h"
#include "ring.h"
#include <stdint.h>


/* The dpdk header files define some macros and packet header structs using the same name as their
counterparts from linux headers files, so we have to include (and define some fields) differently,
depending on what is currently compiling (hoping they really have the same structure).
I pretty much don't want to link dpdk to anything I don't have to. This is set at compile time
by the relevant makefiles. */
#ifdef BUILDING_WITH_DPDK
  #include <rte_ether.h>
#else
  #include <netinet/if_ether.h>
#endif


#if NDP_CORE_RECORD_HW_TS_ANY
	//this may be necessary to use ixgbe hardware timestamping
	#define NDP_ETHERTYPE						0x88F7	//PTP
#else
	#define NDP_ETHERTYPE						0x0800	//IPv4
#endif


#define RING_BUF_OF_CHOICE				ROUND_RING_BUF


#define NDP_PROTOCOL_NUMBER					199
//#define NDP_PROTOCOL_NUMBER					6


#define NUM_GLOBAL_COMM_RING_ELEMENTS		128
#define GLOBAL_COMM_RING_SHM_NAME			"/hoover_ndp_global_comm_ring"
#define LIB_INSTANCE_NAME_PREFIX			"hoover_ndp_lib_instance_"



#define NDP_LINK_SPEED_BPS					(10 * 1000 * 1000 * 1000L)

#define NDP_CONTROL_PACKET_SIZE				(sizeof(struct iphdr) + sizeof(struct ndp_header))


/*this is used to determine the order of two unsigned (currently at least 32 bits wide),
sequence number like values, since they can wrap around & stuff*/
#define	NDP_MAX_SEQ_DIFF					1000000000

#define NDP_MAC_ADDR_SIZE	6


struct ndp_mac_addr
{
	unsigned char bytes[NDP_MAC_ADDR_SIZE];
};


/*if these change, something about the tuple hash computation will probably also have to change
(when there's actually going to be a tuple hash computation) */
typedef uint32_t ndp_net_addr_t;

struct ndp_connection_tuple
{
	ndp_net_addr_t local_addr;
	ndp_port_number_t local_port;
	ndp_net_addr_t remote_addr;
	ndp_port_number_t remote_port;
};


typedef int32_t ndp_socket_index_t;
DEF_RINGS(ndp_socket_index_t, ndp_socket_index_t)

typedef int32_t ndp_packet_buffer_index_t;
DEF_RINGS(ndp_packet_buffer_index_t, ndp_packet_buffer_index_t)

typedef uint16_t ndp_packet_buffer_size_t;

struct shm_parameters
{
	ring_size_t num_comm_ring_elements;
	ndp_socket_index_t num_sockets;

	//the following are per socket

	ring_size_t num_tx_ring_elements;			//contains indices
	ring_size_t num_rtx_ring_elements;			//contains indices
	ring_size_t num_to_rtx_ring_elements;		//contains indices
	ring_size_t num_ack_tx_ring_elements;		//contains "struct ndp_packet_headers" elements

	ring_size_t num_rx_ring_elements;			//contains indices
	ring_size_t num_ack_rx_ring_elements;		//contains "struct ndp_header" elements

	ndp_packet_buffer_index_t num_tx_packet_buffers;
	ndp_packet_buffer_index_t num_rx_packet_buffers;
	ndp_packet_buffer_size_t packet_buffer_mtu;
};



typedef int64_t ndp_instance_id_t;
typedef uint32_t ndp_instance_lib_flags_t;
typedef uint32_t ndp_instance_core_flags_t;

struct ndp_instance_flags
{
	//written to only by the lib instance
	struct {
		ndp_instance_lib_flags_t lib_flags;
	};

	//written to only by the core process
	struct {
		ndp_instance_core_flags_t core_instance_id_is_valid:1,
								  core_terminate_acknowledged:1;
	};
};


//TODO: describe the layout here maybe
struct shm_layout
{
	struct ndp_instance_flags flags;

	ndp_instance_id_t instance_id;
	ndp_net_addr_t local_addr;
	double tsc_hz;

	char rest[] __attribute__((aligned(__alignof__(struct RING_BUF_OF_CHOICE(char))))); //comm ring & sockets follow
};


struct ndp_packet_headers
{
#ifdef BUILDING_WITH_DPDK
	struct ether_hdr eth;
#else
	struct ether_header eth;
#endif
	struct iphdr ip;
	struct ndp_header ndp;

} __attribute__((packed));

DEF_RINGS(struct ndp_packet_headers, struct, ndp_packet_headers)


typedef uint32_t ndp_socket_lib_flags_t;
typedef uint32_t ndp_socket_core_flags_t;

struct ndp_socket_flags
{
	struct {
		ndp_socket_lib_flags_t 		lib_in_use:1,
									lib_is_listening:1;
	};

	struct {
		ndp_socket_core_flags_t		core_connect_acknowledged:1,
									core_connection_can_be_accepted:1,
									core_close_acknowledged:1;
	};
};

struct ndp_socket
{
	struct ndp_socket_flags flags;
	struct ndp_connection_tuple tuple;

	char local_mac[6];
	char remote_mac[6];

	struct ndp_packet_headers nack_template;
	struct ndp_packet_headers pull_template;

	//ring_bufs & tx packet buffers follow
	char rest[] __attribute__((aligned(__alignof__(struct RING_BUF_OF_CHOICE(char)))));
};

typedef uint64_t ndp_timestamp_ns_t;

typedef uint8_t ndp_packet_buffer_lib_flags_t;
typedef uint8_t ndp_packet_buffer_core_flags_t;

struct ndp_packet_buffer_flags
{
	struct {
		ndp_packet_buffer_lib_flags_t 		lib_in_use:1,
											lib_ready_to_send:1,
											lib_was_retransmitted:1;
	};

	struct {
		ndp_packet_buffer_core_flags_t		core_sent_to_nic:1; //presently not used anymore
	};

	//int in_to_rtx_ring;
};

typedef uint64_t ndp_cycle_count_t;

struct ndp_packet_buffer
{
	struct ndp_packet_buffer_flags flags;
	//TODO this field is a bit awkward
	int sync_helper;
	ndp_packet_buffer_size_t bytes_left;
	ndp_cycle_count_t timeout_ref;

	struct ndp_packet_headers hdr;

	char payload[];
};

struct ndp_socket_info
{
	ndp_socket_index_t index;
	struct ndp_socket *socket;

	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *tx_ring;
	struct RING_BUF(ndp_socket_index_t) *listen_ring;
 	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *rtx_ring;
	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *to_rtx_ring;
	struct RING_BUF_OF_CHOICE(struct, ndp_packet_headers) *ack_tx_ring;

	struct RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t) *rx_ring;

	//TODO maybe there should be a separate ring just for NACKs
	struct RING_BUF_OF_CHOICE(struct, ndp_header) *ack_rx_ring;

	char *tx_packet_buffers;
	char *rx_packet_buffers;
	ndp_packet_buffer_size_t aligned_packet_buffer_size;

#if DEBUG_LEVEL > 0
	ndp_packet_buffer_index_t num_tx_packet_buffers;
	ndp_packet_buffer_index_t num_rx_packet_buffers;
#endif

};



struct ack_ring_msg
{
	ndp_sequence_number_t seq;

} __attribute__((packed));



typedef uint64_t ndp_instance_magic_number_t;

//fancy!
#define NDP_INSTANCE_SETUP_MAGIC_NUMBER				0x174f8db309e5c62aul
#define NDP_XEN_SETUP_MAGIC_NUMBER					0xa63e7da29ae4a51bul

#ifdef NDP_BUILDING_FOR_XEN

	#define ZZZ_PAGE_SIZE      			(1ul << 12)

	struct ndp_xen_grant_params
	{
		uint32_t domid;
		uint32_t main_ref;
		uint32_t num_pages;
	};

	struct ndp_instance_setup_request
	{
		ndp_instance_magic_number_t magic_number;

		//union
		//{
			//struct
			//{
				//pid_t pid;
				//struct shm_parameters shm_params;
			//};

			struct ndp_xen_grant_params grant_params;
		//};
	};
#else
	struct ndp_instance_setup_request
	{
		ndp_instance_magic_number_t magic_number;
		pid_t pid;
		struct shm_parameters shm_params;
	};
#endif

DEF_RINGS(struct ndp_instance_setup_request, struct, ndp_instance_setup_request)


//kinda awkward stopgap solution (from a synchronization perspective at least)
struct global_communication_ring
{
	ring_size_t p_sync;
	ring_size_t c_sync;
	struct RING_BUF_OF_CHOICE(struct, ndp_instance_setup_request) inner;
};


typedef uint8_t ndp_instance_op_t;

#define NDP_INSTANCE_OP_CONNECT					1
#define NDP_INSTANCE_OP_LISTEN					2
#define NDP_INSTANCE_OP_CLOSE					3
#define NDP_INSTANCE_OP_TERMINATE				4

struct ndp_instance_op_msg
{
	ndp_instance_op_t type;
	ndp_socket_index_t socket_index;

} __attribute__((packed));

DEF_RINGS(struct ndp_instance_op_msg, struct, ndp_instance_op_msg)


struct ndp_instance_info
{
	ndp_instance_id_t id;
	struct shm_parameters shm_params;
	struct shm_layout *shm_begins;
	struct RING_BUF_OF_CHOICE(struct, ndp_instance_op_msg) *comm_ring;
	char *sockets_begin;
	size_t aligned_socket_size;
};


//TODO I had to place it here due to the stupid ring implementation having to see it at all times
struct core_socket_descriptor
{
	//TODO these flags are awkward
	char	socket_is_active:1,

	#if NDP_CORE_NACKS_FOR_HOLES
			got_at_least_one_data_packet:1,
	#endif

			socket_is_passive:1;

	/*TODO I currently assume this number never overflows; this is probably reasonable as long as the underlying
	data type is sufficiently large, or at least as reasonable as having that MAX_SEQ_DIFF thing */
	ndp_pull_number_t tx_allowance;

	ndp_pull_number_t highest_pull_sent;
	ndp_pull_number_t highest_pull_rcvd;


	struct ndp_socket_info socket_info;
	struct ndp_instance_info *instance_info;

	int index;

	#if NDP_CORE_NACKS_FOR_HOLES
		ndp_sequence_number_t last_recv_seq_number;
		struct ROUND_RING_BUF(char) *n4h_ring;
	#endif
};

DEF_RINGS(struct core_socket_descriptor*, struct, core_socket_descriptor, ptr);


typedef uint32_t ndp_lock_t;


#endif /* NDP_COMMON_DEFS_H_ */
