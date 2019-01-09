#ifndef NDP_LIB_DEFS_H_
#define NDP_LIB_DEFS_H_


#include "../common/defs.h"
#include "../common/logger.h"
//#include "../common/ring.h"
#include "config.h"

#ifdef NDP_BUILDING_FOR_XEN
	#include <xengnttab.h>
#endif


/* TODO this many pkb indices must fit in the rx_ring, and don't forget that one element
in the ring_buf is unusable; also this should depend on the max possible BDP */
#if   NDP_MTU == 9000
	#define NDP_TX_PACKET_WINDOW				100

	//TODO there should be at least this many rx buffer in each socket; because of this,
	//the initial rx window size should be a run time parameter
	#define NDP_INITIAL_RX_WINDOW_SIZE 			100
#elif NDP_MTU == 1500
	//why was 100 apparently too low here? (200 is overkill most likely though)
	#define NDP_TX_PACKET_WINDOW				200
	#define NDP_INITIAL_RX_WINDOW_SIZE 			200
#endif


//each socket has an associated descriptor
struct lib_socket_descriptor
{
	struct ndp_packet_headers ack_template;

	//tx and rx flags are separated because there can be two threads doing tx and rx stuff at the same time
	//I'll clear this up somehow after the design becomes mature enough
	struct
	{
		//general flags
		//TODO when FIN is implemented, we'll probably have to have can_send and can_recv
		int can_send_recv:1,
			is_from_connect:1,
			is_listening:1,
			got_first_ack:1;
	};

	struct
	{
		//tx flags
		int syn_sent:1;
	};

	struct
	{
		//rx flags
		int any_packet_received:1,
			syn_received:1;
	};

	struct ndp_socket_info socket_info;

	//TODO this field is a bit awkward (is it?)
	ndp_lock_t recv_lock;
	ndp_lock_t send_lock;

	//TODO kinda unfortunate name; probably should change it to something more fitting
	struct RING_BUF(ndp_packet_buffer_index_t) *rx_wnd_ring;

	ring_size_t rx_consumer_shadow;

	//used while waiting for the SYN packet to arrive
	ndp_sequence_number_t recv_isn;
	ndp_sequence_number_t recv_high;
	ndp_sequence_number_t recv_low;

	struct RING_BUF(ndp_packet_buffer_index_t) *pending_to_be_acked;

	//TODO the sequence number of the packet recv currently wants to read from
	ndp_sequence_number_t recv_pkt_seq;
	//recv offset into the current packet we're reading data from
	ndp_sequence_number_t recv_pkt_off;

	//this is the first new sequence number that will NOT be accepted by the receiver
	ndp_sequence_number_t remote_rwnd_edge;

	ndp_sequence_number_t largest_advertised_rwnd_edge;

	struct RING_BUF(ndp_packet_buffer_index_t) *tx_av_pkb_ring;
	struct RING_BUF(char) *tx_wnd_ring;

	ndp_packet_buffer_index_t tx_pkb_in_use;
	ndp_sequence_number_t tx_next_seq_number;
	ndp_sequence_number_t tx_first_not_ack;

	ndp_sequence_number_t send_isn;

	ring_size_t listen_consumer_shadow;

	//tempzzzzzz
	//ndp_sequence_number_t asdf_prev;

	ndp_sequence_number_t zmost_recent_ack;

	//TODO stopgap to make send check for timeouts more frequently?!
	ndp_cycle_count_t last_send_check_acknto;
	//ndp_sequence_number_t maxxx_difff;

};


#ifdef NDP_BUILDING_FOR_XEN
	struct alloc_for_grant_result
	{
	  int retval;
	  //xengntshr_handle *h;
	  uint32_t num_pages;
	  void *mem;
	  void *ref_chain;
	  uint32_t meta_ref;
	};
#endif


#define NDP_LIB_MAX_SOCKETS					1000

struct ndp_lib_data
{
	#ifdef NDP_BUILDING_FOR_XEN
		xengntshr_handle *gh;
		struct alloc_for_grant_result grant_result;
	#else
		char shm_name[NDP_LIB_MAX_SHM_NAME_BUF_SIZE];
		struct global_communication_ring *global_comm_ring;
	#endif

	struct ndp_instance_info instance_info;

	ndp_net_addr_t local_addr;
	ndp_cycle_count_t cycles_per_timeout;

	//TODO maybe also free the av_sock_ring memory
	struct lib_socket_descriptor socket_descriptors[NDP_LIB_MAX_SOCKETS];
	struct RING_BUF(ndp_socket_index_t) *av_sock_ring;

	pthread_t timeout_thread;
	volatile int timeout_thread_may_run;

	#if NDP_LIB_RECORD_SW_BUF_SIZE
		struct two_headed_buffer record_sw_buf;
	#endif

	ndp_lock_t ndp_close_lock;
};


//**********

struct lib_stupid_counters
{
	uint64_t in_w_already;
	uint64_t ack_old_data;
	uint64_t ack_no_tx_pkb;
	uint64_t nack_no_tx_pkb;
	uint64_t to_rtx;
	uint64_t ack_holes;
	uint64_t to_pkbs;
	uint64_t old_acks;
	uint64_t old_nacks;
	//
	uint64_t data_totrx;
	uint64_t acks_totrx;
	uint64_t rwnd_acks;
};

extern struct lib_stupid_counters lctrs;

//**********

extern struct ndp_lib_data lib;




#endif /* NDP_LIB_DEFS_H_ */
