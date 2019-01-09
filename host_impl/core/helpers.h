#ifndef NDP_CORE_HELPERS_H_
#define NDP_CORE_HELPERS_H_


#include "../common/helpers.h"
#include "../common/logger.h"
#include "../common/ring.h"
#include "../common/tsc.h"
#include "defs.h"
#include <rte_ethdev.h>
#include <rte_ip.h>


int init_pre_dpdk(int argc, char ** argv);		//init.c
int init_dpdk(void);							//init_dpdk.c

void check_global_comm_ring(void);				//global_comm.c
void check_comm_rings(void); 					//comm.c
void check_tx(void);							//tx.c
void check_rx(void);							//rx.c
void check_pullq_in_core_loop(void);			//pullq.c

//the following are defined in helpers.c

void core_output_file_name(char *buf, size_t len, const char *middle);

struct core_instance_descriptor* get_available_instance_descriptor(void);

//TODO this function should be renamed (and redesigned for core_socket_descriptors preallocated by lib inside shm)
struct core_socket_descriptor* get_socket_descriptor(struct ndp_instance_info *instance_info, ndp_socket_index_t socket_index);
struct core_socket_descriptor* get_existing_socket_descriptor(struct ndp_instance_info *instance_info, ndp_socket_index_t socket_index);

void prepare_socket_descriptor(struct ndp_instance_info *instance_info, char is_active, struct core_socket_descriptor *d);



static inline ndp_cycle_count_t core_read_tsc(void)
{
	return ndp_rdtsc();
}

static inline void core_record_event(log_record_type_t type, ndp_instance_id_t instance_id, ndp_socket_index_t socket_index,
		ndp_sequence_number_t seq, ndp_cycle_count_t tsc)
{
	if(type == LOG_RECORD_CORE_PULL_SENT)
		write_generic_record_back(&core.record_sw_buf, type, instance_id, socket_index, seq, tsc);
	else
		write_generic_record_front(&core.record_sw_buf, type, instance_id, socket_index, seq, tsc);
}

static inline int send_one_buffer(struct rte_mbuf *p, uint16_t queue_id)
{
	p->l2_len = sizeof(struct ether_hdr);
	p->l3_len = sizeof(struct ipv4_hdr);
	p->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;


	#if NDP_CORE_RECORD_HW_TS_TX_MAX

		if(rte_pktmbuf_mtod(p, struct ndp_packet_headers*)->ndp.flags & NDP_CORE_RECORD_HW_TS_POI)
		{
			if(++core.rec_hw_ts_num_poi_skipped == NDP_CORE_RECORD_HW_TS_ONE_IN)
			{
				p->ol_flags |= PKT_TX_IEEE1588_TMST;
				core.rec_hw_ts_num_poi_sent_to_nic++;
				core.rec_hw_ts_num_poi_skipped = 0;
			}
		}

	#endif

	//TODO I should be more careful with this; currently indices are consumed in various calling contexts even if send_one_packet fails

	if(UNLIKELY(!rte_eth_tx_burst(core.dpdk_port_id, queue_id, &p, 1)))
		LOG_FMSG_DO(LOGLVL3, return -3, "%s -> ! rte_eth_tx_burst", __func__);

	//LOG_NDP_PACKET_HDRS(LOGLVL101, buf);

	return 0;
}

//TODO sending bursts is probably better, but this will do for now
static inline int send_one_packet(void *buf, size_t size, uint16_t queue_id)
{
	struct rte_mbuf *p = rte_pktmbuf_alloc(core.mbuf_pool);

	if(UNLIKELY(!p))
		LOG_FMSG_DO(LOGLVL3, return -1, "%s -> ! rte_pktmbuf_alloc", __func__);

	//should never happen :-s
	if(UNLIKELY(!rte_pktmbuf_append(p, size)))
		LOG_FMSG_DO(LOGLVL3, return -2, "%s -> ! rte_pktmbuf_append", __func__);

	memcpy(rte_pktmbuf_mtod(p, void*), buf, size);

	return send_one_buffer(p, queue_id);
}

void log_mbuf_dump(struct rte_mbuf *b, int bytes_per_line);


#if NDP_CORE_SEND_CHOPPED_RATIO
	int send_one_chopped_packet(void *buf);
#endif


#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
	int pullq_thread_func(void *param);// __attribute__ ((noreturn));
#endif


//the following are situational

//#if NDP_CORE_RECORD_HW_TS_ANY || NDP_CORE_RECORD_SW_ANY
	//record.c
	void record_write_results(void);
//#endif

#if NDP_CORE_RECORD_HW_TS_ANY
	int record_hw_ts_thread_func(void *param) __attribute__ ((noreturn)); //record.c
#endif


//******

#include <stdio.h>

static inline void print_core_stupid_counters(void)
{
	printf("rcvd_chop %lu nacks4holes %lu no_pkb4nack %lu no_socket_found %lu not_lrts %lu no_av_pkbuf %lu "
			"big_hole %lu \n",
			cctrs.rcvd_chop, cctrs.nacks4holes, cctrs.no_pkb4nack, cctrs.no_socket_found, cctrs.not_lrts, cctrs.no_av_pkbuf,
			cctrs.big_hole);
}


#endif /* NDP_CORE_HELPERS_H_ */
