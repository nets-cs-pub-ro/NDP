#include "../common/logger.h"
#include "../common/tsc.h"
#include <arpa/inet.h>
#include "defs.h"
#include <errno.h>
#include <fcntl.h>
#include "helpers.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


int init_pre_dpdk(int argc, char ** argv)
{
	if (argc != 2)
	{
		printf("ip ?\n");
		goto exit_error;
	}

	memset(&core, 0, sizeof(core));

	#if NDP_CORE_SEPARATE_LCORE_FOR_PULLQ
		core.pullq_core_finished = 0;
		RELEASE_FENCE();
		core.keep_running_pullq_core = 1;
	#endif


	#ifdef NDP_BUILDING_FOR_XEN
		core.gh = xengnttab_open(NULL, 0);
		if(!core.gh)
		{
			printf("cloud not open xengnttab_handle\n");
			exit(1);
		}
	#endif


	for(int i = 0; i < NDP_CORE_MAX_LIBS; i++)
	{
		core.instance_descriptor_available[i] = 1;
		core.instance_descriptors[i].instance_info.id = i;
	}

	for(int i = 0; i < NDP_CORE_MAX_SOCK_DESC; i++)
	{
		core.socket_descriptor_available[i] = 1;
		core.socket_descriptors[i].index = i;
	}

	if(inet_pton(AF_INET, argv[1], &core.local_addr) != 1)
		LOG_MSG_DO(LOGLVL1, goto exit_error, "!inet_pton");

	if(!shm_unlink(GLOBAL_COMM_RING_SHM_NAME))
		LOG_MSG(LOGLVL100, "removed previous global_comm_ring shm file");

	//setup global comm ring
	int shm_fd = shm_open(GLOBAL_COMM_RING_SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (shm_fd < 0)
		LOG_FMSG_DO(LOGLVL1, goto exit_error, "shm_open error ; %s", strerror(errno));

	size_t global_comm_ring_sz = global_comm_ring_size(NUM_GLOBAL_COMM_RING_ELEMENTS);

	if(ftruncate(shm_fd, global_comm_ring_sz) < 0)
		LOG_FMSG_DO(LOGLVL1, goto unlink, "ftruncate error ; %s", strerror(errno));

	void *mmap_result = mmap(NULL, global_comm_ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(mmap_result == (void*)-1)
		LOG_FMSG_DO(1, goto unlink, "mmap error ; %s", strerror(errno));

	struct global_communication_ring *aux = (struct global_communication_ring*)mmap_result;

	//init_multi_ring(GLOBAL_COMM_RING_BUF_SIZE, (struct multi_ring_buf*)mmap_result);
	aux->p_sync = 0;
	aux->c_sync = 0;
	core.global_comm_ring = &aux->inner;

	RING_INIT(core.global_comm_ring, NUM_GLOBAL_COMM_RING_ELEMENTS);

	//core_data.global_comm_ring = &((struct multi_ring_buf*)mmap_result)->inner;


	ring_size_t pq_num_elements = NDP_CORE_PULLQ_NUM_ELEMENTS; //* sizeof(struct core_socket_descriptor*);
	core.pullq = malloc(RING_SIZE(RING_BUF_OF_CHOICE(struct, core_socket_descriptor, ptr), pq_num_elements));

	if(!core.pullq)
		LOG_FMSG_DO(1, goto unlink, "!core_data.pullq ; %s", strerror(errno));

	RING_INIT(core.pullq, pq_num_elements);


	#if NDP_CORE_PRIOPULLQ_4COSTIN
		core.pullq2 = malloc(RING_SIZE(RING_BUF_OF_CHOICE(struct, core_socket_descriptor, ptr), pq_num_elements));

		if(!core.pullq2)
			LOG_FMSG_DO(1, goto unlink, "!core_data.pullq2 ; %s", strerror(errno));

		RING_INIT(core.pullq2, pq_num_elements);
	#endif


	LOG_MSG(LOGLVL10, "estimating tsc_hz... ");
	core.tsc_hz = ndp_estimate_tsc_hz(NDP_TSC_HZ_ESTIMATION_SLEEP_NS);
	LOG_FMSG(LOGLVL10, "tsc_hz = %lf", core.tsc_hz);

	//aux = core.tsc_hz / (1000 * 1000 * 1000.0) * NDP_CORE_PULLQ_INTERVAL_NS;
	core.cycles_per_pull = (core.tsc_hz * NDP_CORE_PULLQ_INTERVAL_NS) / NS_PER_SECOND;
	LOG_FMSG(LOGLVL10, "cycles_per_pull = %"PRIu64, core.cycles_per_pull);



	/*
	#if NDP_CORE_RECORD_HW_TS_TX_MAX
		memset(core_data.rec_hw_ts_tx, 0, sizeof(core_data.rec_hw_ts_tx));
		core_data.rec_hw_ts_tx_idx = 0;
	#endif

	#if NDP_CORE_RECORD_HW_TS_RX_MAX
		memset(core_data.rec_hw_ts_rx, 0, sizeof(core_data.rec_hw_ts_rx));
		core_data.rec_hw_ts_rx_idx = 0;
	#endif
	*/

	#if NDP_CORE_RECORD_HW_TS_ANY
		//core_data.rec_hw_ts_num_poi_sent_to_nic = 0;

		//catch first packet, no matter the sampling rate
		core.rec_hw_ts_num_poi_skipped = NDP_CORE_RECORD_HW_TS_ONE_IN - 1;
	#endif

	/*
	#if NDP_CORE_RECORD_SW_TX_MAX
		memset(core_data.rec_sw_data_tx, 0, sizeof(core_data.rec_sw_data_tx));
		core_data.rec_sw_data_tx_idx = 0;
	#endif

	#if NDP_CORE_RECORD_SW_RX_MAX
		memset(core_data.rec_sw_data_rx, 0, sizeof(core_data.rec_sw_data_rx));
		core_data.rec_sw_data_rx_idx = 0;
	#endif
	*/

	#if	NDP_CORE_RECORD_SW_BUF_SIZE
		if(two_headed_buffer_init(&core.record_sw_buf, NDP_CORE_RECORD_SW_BUF_SIZE))
			LOG_FMSG_DO(LOGLVL1, goto unlink, "%s -> !two_headed_buffer_init(&core.record_sw_buf)", __func__);
	#endif

	return 0;

unlink:
	if(shm_unlink(GLOBAL_COMM_RING_SHM_NAME) < 0)
		LOG_MSG(LOGLVL1, "core global comm ring unlink error");

exit_error:
	return -1;
}

