#ifndef NDP_CORE_DEFS_H_
#define NDP_CORE_DEFS_H_


#include "../common/defs.h"
#include "../common/ring.h"
#include "config.h"
#include <netinet/in.h>
#include <rte_mempool.h>

#ifdef NDP_BUILDING_FOR_XEN
	#include <xengnttab.h>
#endif



#define NDP_CORE_MBUF_POOL_NAME				"ndp_core_mbuf_pool"
#define NDP_CORE_NUM_MBUFS					20000	//TODO these values should be tuned at some point
#define NDP_CORE_MBUF_CACHE_SIZE			250

#define NDP_CORE_RX_RING_SIZE				128
#define NDP_CORE_TX_RING_SIZE				1024

#define NDP_CORE_MAX_LIBS				1000
#define NDP_CORE_MAX_SOCK_DESC			1000


//TODO take the linkspeed define into account for some of these values maybe ? currently it's for 10Gbps
//the ndp tx window should also be defined differently based on mtu, in lib/defs.h
#if   NDP_MTU == 9000
	/* Ok, so when dealing with jumbo frames (or any sufficiently large frames for that matter), dpdk will hold one packet
	in multiple mbufs, which are chained together. I want mbufs to be large enough so one jumbo frame fits entirely within
	one mbuf. However, it turns out that dpdk also has some weird head/tail room areas inside mbufs, so I don't really know
	precisely how large to define the data room size. I pretty much just slap a value that appears to be working here, and
	maybe I'll actually understand what happens at some point in the future */
	#define NDP_CORE_MBUF_DATA_ROOM_SIZE		(9000 + 128 + 14 + 202)
	#define NDP_CORE_INITIAL_TX_ALLOWANCE		10
	#define NDP_CORE_PULLQ_INTERVAL_NS			7300

#elif NDP_MTU == 1500

	#define NDP_CORE_MBUF_DATA_ROOM_SIZE		RTE_MBUF_DEFAULT_BUF_SIZE

	#if NDP_HARDCODED_SAME_NEXT_HOP
		#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		50
		//#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		55
		//#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		100
	#else
		#if NDP_CURRENT_TEST_ENVIRONMENT == ENV_CAM
			#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		33
		#else
			#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		35
		#endif
		//#define NDP_CORE_INITIAL_TX_ALLOWANCE	 		40
	#endif

	//#define NDP_CORE_PULLQ_INTERVAL_NS			1210
	#define NDP_CORE_PULLQ_INTERVAL_NS			1230
	//#define NDP_CORE_PULLQ_INTERVAL_NS				1400
	//#define NDP_CORE_PULLQ_INTERVAL_NS			1100

#endif


#define NDP_CORE_PULLQ_NUM_ELEMENTS				4096


#ifdef NDP_BUILDING_FOR_XEN
	struct map_grant_result
	{
		int retval;
		void *mem;
		size_t num_pages;
	};
#endif


struct core_instance_descriptor
{
	char shm_name[NDP_LIB_MAX_SHM_NAME_BUF_SIZE];
	int shm_fd;
	struct ndp_instance_info instance_info;

	#ifdef NDP_BUILDING_FOR_XEN
		struct map_grant_result map_grant_res;
	#endif
};


struct ndp_core_data
{
	//struct in_addr local_in_addr;
	ndp_net_addr_t local_addr;

	/*the core will read as single consumer, so it only hold a reference to the inner ring of the
	global_comm_ring structure, which is actually a struct multi_ring_buf*/
	struct RING_BUF_OF_CHOICE(struct, ndp_instance_setup_request) *global_comm_ring;

	//*** dpdk stuff begins ***

	uint8_t dpdk_port_id;
	uint16_t dpdk_num_rx_queues;
	uint16_t dpdk_num_tx_queues;
	uint16_t dpdk_default_queue_id;
	uint16_t dpdk_pull_tx_queue_id;

	/*TODO this is currently used for packets of any size; maybe one for jumbo and one for headers
	would be more appropriate; then again, optimizations come later :-s */
	struct rte_mempool *mbuf_pool;

	//*** dpdk stuff ends ***

	ndp_port_number_t next_available_port;

	//TODO use ring if the max number of descriptors remains a fixed value; or something else otherwise
	char instance_descriptor_available[NDP_CORE_MAX_LIBS];
	struct core_instance_descriptor instance_descriptors[NDP_CORE_MAX_LIBS];

	//TODO stopgap; core_socket_descriptors should be allocated per socket by the libs in the shm area
	char socket_descriptor_available[NDP_CORE_MAX_SOCK_DESC];
	struct core_socket_descriptor socket_descriptors[NDP_CORE_MAX_SOCK_DESC];

	int num_lib_instances;
	int num_active_sockets;
	int num_passive_sockets;

	//TODO WARNING: when socket close/reuse is implemented, some pull requests may refer to recycled sockets
	struct RING_BUF_OF_CHOICE(struct, core_socket_descriptor, ptr) *pullq;

	#if	NDP_CORE_PRIOPULLQ_4COSTIN
		struct RING_BUF_OF_CHOICE(struct, core_socket_descriptor, ptr) *pullq2;
	#endif

	double tsc_hz;
	ndp_cycle_count_t cycles_per_pull;

	ndp_cycle_count_t next_pull_cycles;
	int pullq_was_empty;

	//ndp_cycle_count_t pseudo_ts;


	#if NDP_CORE_SEND_CHOPPED_RATIO
		size_t send_chopped_counter;
	#endif

	#if NDP_CORE_RECORD_SW_BUF_SIZE
		struct two_headed_buffer record_sw_buf;
	#endif

	#if NDP_CORE_RECORD_HW_TS_TX_MAX
		ndp_timestamp_ns_t rec_hw_ts_tx[NDP_CORE_RECORD_HW_TS_TX_MAX];
		size_t rec_hw_ts_tx_idx;
	#endif

	#if NDP_CORE_RECORD_HW_TS_RX_MAX
		ndp_timestamp_ns_t rec_hw_ts_rx[NDP_CORE_RECORD_HW_TS_RX_MAX];
		size_t rec_hw_ts_rx_idx;
	#endif

	#if NDP_CORE_RECORD_HW_TS_ANY
		size_t rec_hw_ts_num_poi_sent_to_nic;
		size_t rec_hw_ts_num_poi_skipped;
	#endif

	#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
		volatile int keep_running_pullq_core;
		volatile int pullq_core_finished;
	#endif

	#ifdef NDP_BUILDING_FOR_XEN
		xengnttab_handle *gh;
	#endif

	ndp_cycle_count_t maxxx;
};


//******************

struct core_stupid_counters
{
	uint64_t rcvd_chop;
	uint64_t nacks4holes;
	uint64_t no_pkb4nack;
	uint64_t no_socket_found;
	uint64_t not_lrts;
	uint64_t no_av_pkbuf;
	uint64_t big_hole;
};

extern struct core_stupid_counters cctrs;

//******************


extern struct ndp_core_data core;


#endif /* NDP_CORE_DEFS_H_ */
