#include "../common/logger.h"
#include <assert.h>
#include "helpers.h"
#include <rte_atomic.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>


#define CORE_REQS_SUM		(1 + NDP_CORE_SEPARATE_LCORE_FOR_PULLQ + NDP_CORE_RECORD_HW_TS_ANY)

//leave core 0 alone; I hope this helps with availability when isolcpus is not used
#define CORE_MASK_VALUE		(((1 << CORE_REQS_SUM) - 1) << 1)



//the arguments fed to the dpdk initialization
static char dpdk_arg0[] = "zzz";	//this should actually be the program name I guess, but ah well
static char dpdk_arg1[] = "-c";
static char dpdk_arg2[30];

#ifdef NDP_BUILDING_FOR_XEN
	static char dpdk_arg3[] = "--xen-dom0";
	static char *dpdk_argv[] = { dpdk_arg0, dpdk_arg1, dpdk_arg2, dpdk_arg3 };
#else
	static char *dpdk_argv[] = { dpdk_arg0, dpdk_arg1, dpdk_arg2 };
#endif

/*
static void enable_strict_prio_for_tc(uint8_t port, uint8_t tc)
{
	struct rte_eth_dev_info dev_info;
	rte_eth_dev_info_get(port, &dev_info);

	char *start = (char*)dev_info.pci_dev->mem_resource[0].addr;

	//RTTPT2C[tc]
	uint32_t offset = 0x0CD20 + 4 * tc;

	volatile uint32_t *reg = (volatile uint32_t*)(start + offset);

	*reg |= (1 << 31);
}
*/


int init_dpdk(void)
{
	static_assert(CORE_MASK_VALUE < 64, "CORE_MASK_VALUE too large");
	sprintf(dpdk_arg2, "%d", CORE_MASK_VALUE);

	core.dpdk_port_id = 0;

	//I think 128 are required to use the port in DCB mode
	core.dpdk_num_rx_queues = 128;
	core.dpdk_num_tx_queues = 128;

	core.dpdk_default_queue_id = 0;
	core.dpdk_pull_tx_queue_id = 64;

	int dpdk_argc = sizeof(dpdk_argv) / sizeof(char*);

	int ret = rte_eal_init(dpdk_argc, dpdk_argv);
	if(ret < 0)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eal_init error; ret = %d", __func__, ret);


	unsigned int needed_lcores = 1;		//one for the core loop


	#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
	  needed_lcores++;
	#endif

	#if NDP_CORE_RECORD_HW_TS_ANY
	  needed_lcores++;
	#endif

/*
#if NDP_CORE_RECORD_SW_RX_MAX
	needed_lcores++;
#endif
*/

	unsigned int lcore_count = rte_lcore_count();
	if(lcore_count != needed_lcores)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s-> rte_lcore_count(%u) != needed_lcores(%u)", __func__, lcore_count, needed_lcores);

	LOG_FMSG(LOGLVL10, "rte_lcore_count() = %u", lcore_count);

	core.mbuf_pool = rte_pktmbuf_pool_create(NDP_CORE_MBUF_POOL_NAME,
												  NDP_CORE_NUM_MBUFS,
												  NDP_CORE_MBUF_CACHE_SIZE,
												  0,
												  NDP_CORE_MBUF_DATA_ROOM_SIZE,
												  rte_socket_id());
	if (!core.mbuf_pool)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> NULL rte_pktmbuf_pool_create", __func__);

	//configure the port (port 0)

	struct rte_eth_conf port_conf =
	{
	#if   NDP_MTU == 9000
	    .rxmode = { .mq_mode = ETH_MQ_RX_DCB, .max_rx_pkt_len = 9100, .jumbo_frame = 1 },
	#elif NDP_MTU == 1500
	    .rxmode = { .mq_mode = ETH_MQ_RX_DCB, .max_rx_pkt_len = ETHER_MAX_LEN },
    #endif
	    .rx_adv_conf = { .dcb_rx_conf = { .nb_tcs = ETH_4_TCS} },
	    .txmode = { .mq_mode = ETH_MQ_TX_DCB },
		.tx_adv_conf = { .dcb_tx_conf = { .nb_tcs = ETH_4_TCS} }
	};

	LOG_FMSG(LOGLVL10, "rte_eth_dev_count() = %u", rte_eth_dev_count());

	LOG_FMSG(LOGLVL10, "using %d rx queue(s) and %d tx queue(s)", core.dpdk_num_rx_queues, core.dpdk_num_tx_queues);

	ret = rte_eth_dev_configure(core.dpdk_port_id, core.dpdk_num_rx_queues, core.dpdk_num_tx_queues, &port_conf);
	if(ret < 0)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eth_dev_configure error; ret = %d", __func__, ret);

	for(uint16_t q = 0; q < core.dpdk_num_rx_queues; q++)
	{
		ret = rte_eth_rx_queue_setup(core.dpdk_port_id, q, NDP_CORE_RX_RING_SIZE,
						rte_eth_dev_socket_id(core.dpdk_port_id), NULL, core.mbuf_pool);
		if(ret < 0)
			LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eth_rx_queue_setup error ; q = %d ; ret = %d", __func__, q, ret);
	}

	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf *txconf;
	rte_eth_dev_info_get(core.dpdk_default_queue_id, &dev_info);
	txconf = &dev_info.default_txconf;
	LOG_FMSG(LOGLVL10, "original default txconf txq_flags = %08x", txconf->txq_flags);

	//TODO why don't I use the default flags? is it just because something didn't work with hardware timestamping?
	//TODO maybe at some point these assumptions aren't valid anymore
	txconf->txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS | ETH_TXQ_FLAGS_NOMULTMEMP;

	for(uint16_t q = 0; q < core.dpdk_num_tx_queues; q++)
	{
		ret = rte_eth_tx_queue_setup(core.dpdk_port_id, q, NDP_CORE_TX_RING_SIZE,
						rte_eth_dev_socket_id(core.dpdk_port_id), txconf);
		if(ret < 0)
			LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eth_tx_queue_setup error ; q = %d ; ret = %d", __func__, q, ret);
	}

	ret = rte_eth_dev_start(core.dpdk_port_id);
	if(ret < 0)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eth_dev_start error; ret = %d", __func__, ret);

#if NDP_CORE_RECORD_HW_TS_ANY
	ret = rte_eth_timesync_enable(core.dpdk_port_id);
	if(ret < 0)
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> rte_eth_timesync_enable error; ret = %d", __func__, ret)
	else
		LOG_FMSG(LOGLVL10, "%s -> rte_eth_timesync_enable (%d)", __func__, ret);

#endif


	/*

	LOG_MSG(LOGLVL1, "rate limiting queues to 9900Mbps");

	if(rte_eth_set_queue_rate_limit(core.dpdk_port_id, core.dpdk_default_queue_id, 9900))
		rte_exit(1, "could not set queue rate limit\n");

	if(rte_eth_set_queue_rate_limit(core.dpdk_port_id, core.dpdk_pull_tx_queue_id, 9900))
		rte_exit(1, "could not set queue rate limit\n");

	*/

	//hmmmmm
	//enable_strict_prio_for_tc(core.dpdk_port_id, 1);

	return 0;
}

