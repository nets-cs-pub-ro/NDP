#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>


#include "../common/helpers.h"
#include "../common/logger.h"
#include "../common/ndp_header.h"
#include "defs.h"
#include <errno.h>
#include <fcntl.h>
#include "helpers.h"
#include "hoover_ndp.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "timeout.h"
#include <unistd.h>


#ifdef NDP_BUILDING_FOR_XEN
	#include <xenstore.h>
#endif



static void init_shm_parameters(ring_size_t num_comm_ring_elements,
								ndp_socket_index_t num_sockets,
								ring_size_t num_tx_ring_elements,
								ring_size_t num_rtx_ring_elements,
								ring_size_t num_ack_tx_ring_elements,
								ring_size_t num_rx_ring_elements,
								ring_size_t num_ack_rx_ring_elements,
								ndp_packet_buffer_index_t num_tx_packet_buffers,
								ndp_packet_buffer_index_t num_rx_packet_buffers,
								ndp_packet_buffer_size_t packet_buffer_mtu,
								struct shm_parameters *shm_params)
{

	//TODO figure exactly what this function expects from its parameters, and what it does with them
	/*
	shm_params->num_comm_ring_elements = 	RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(struct, ndp_instance_op_msg),
			num_comm_ring_elements,	__alignof__(struct RING_BUF_OF_CHOICE(char)));

	shm_params->num_sockets = num_sockets;

	shm_params->num_tx_ring_elements = 		RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			num_tx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));

	shm_params->num_rtx_ring_elements =		RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			num_rtx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));

	//same number of elements as for rtx
	shm_params->num_to_rtx_ring_elements =	RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			num_rtx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));

	shm_params->num_ack_tx_ring_elements =	RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(struct, ndp_packet_headers),
			num_ack_tx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));

	shm_params->num_rx_ring_elements = 		RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(ndp_packet_buffer_index_t),
			num_rx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));

	shm_params->num_ack_rx_ring_elements =	RING_ALIGNED_NUM_ELEMENTS(RING_BUF_OF_CHOICE(struct, ndp_header),
			num_ack_rx_ring_elements, __alignof__(struct RING_BUF_OF_CHOICE(char)));
	*/

	shm_params->num_comm_ring_elements = num_comm_ring_elements;
	shm_params->num_sockets = num_sockets;
	shm_params->num_tx_ring_elements = num_tx_ring_elements;
	shm_params->num_rtx_ring_elements =	num_rtx_ring_elements;

	//same number of elements as for rtx
	shm_params->num_to_rtx_ring_elements = num_rtx_ring_elements;

	shm_params->num_ack_tx_ring_elements = num_ack_tx_ring_elements;
	shm_params->num_rx_ring_elements = num_rx_ring_elements;
	shm_params->num_ack_rx_ring_elements = num_ack_rx_ring_elements;


	shm_params->num_tx_packet_buffers = num_tx_packet_buffers;
	shm_params->num_rx_packet_buffers = num_rx_packet_buffers;
	shm_params->packet_buffer_mtu = packet_buffer_mtu;
}


static int init_shm()
{
	struct ndp_instance_info *instance_info = &lib.instance_info;

	//instance_info->shm_begins->flags.lib_test = 121;

	//printf("sfsg3\n");
	//init_ring(instance_info->shm_params.num_comm_ring_elements, instance_info->comm_ring);
	RING_INIT(instance_info->comm_ring, instance_info->shm_params.num_comm_ring_elements);

	//printf("sfsg2\n");

	for(ndp_socket_index_t i = 0; i < instance_info->shm_params.num_sockets; i++)
	{
		//const struct shm_parameters *shm_params = &instance_info->shm_params;
		struct lib_socket_descriptor *d = &lib.socket_descriptors[i];
		struct ndp_socket_info *socket_info = &d->socket_info;

		init_ndp_socket_info(instance_info, i, socket_info);

		if(init_lib_socket_descriptor(d))
			LOG_FMSG_DO(LOGLVL1, return -1, "%s -> init_lib_socket_descriptor error; probably from malloc", __func__);
	}

	return 0;
}


#ifdef NDP_BUILDING_FOR_XEN

static void populate_ref_chain(const uint32_t *refs_end, size_t num_pages, char *ref_data_end)
{
  size_t x = num_pages_for_bytes(num_pages * sizeof(uint32_t));

  uint32_t *data_start = (uint32_t*)(ref_data_end - x * ZZZ_PAGE_SIZE);

  for(int i = 0; i < num_pages; i++)
    data_start[i] = (refs_end - num_pages)[i];

  if(x > 1)
    populate_ref_chain(refs_end - num_pages, x, (char*)data_start);
}


static struct alloc_for_grant_result alloc_for_grant(xengntshr_handle *gh, uint32_t domid, size_t num_bytes)
{
	struct alloc_for_grant_result result;
	memset(&result, 0, sizeof(result));

	size_t pages = num_pages_for_bytes(num_bytes);
	size_t meta_pages = num_meta_pages(pages);

	result.num_pages = pages;

	uint32_t *meta_refs = malloc((pages + meta_pages) * sizeof(uint32_t));
	if(!meta_refs)
	{
		//printf("alloc_and_grant !refs\n");
		result.retval = -2;
		goto return_result;
	}

	uint32_t *refs = meta_refs + meta_pages;

	result.mem = xengntshr_share_pages(gh, domid, pages, refs, 1);

	if(!result.mem)
	{
		//printf("alloc_and_grant !result\n");
		result.retval = -4;
		goto cleanup_refs;
	}

	result.ref_chain = xengntshr_share_pages(gh, domid, meta_pages, meta_refs, 0);
	if(!result.ref_chain)
	{
		result.retval = -5;
		goto cleanup_result_mem;
	}

	populate_ref_chain(refs + pages, pages, (char*)result.ref_chain + ZZZ_PAGE_SIZE * meta_pages);

	result.meta_ref = meta_refs[0];

	goto cleanup_refs;

  //cleanup_result_ref_chain:
	//if(xengntshr_unshare(gh, result.ref_chain, meta_pages))
	//printf("alloc unshare ref_chain err\n");

  cleanup_result_mem:
	if(xengntshr_unshare(gh, result.mem, pages))
	printf("alloc unshare mem err\n");

  cleanup_refs:
	free(meta_refs);

  return_result:
	return result;
}

static int write_grant_to_store(uint32_t meta_ref, uint32_t num_pages)
{
	//write to store

	struct xs_handle *sh = xs_open(0);

	if(!sh)
		return -1;

	xs_transaction_t st;
	char path[1024];
	char path_data[256];

	sprintf(path, "data/ndp_grants/%u", meta_ref);
	sprintf(path_data, "%u", num_pages);

	st = xs_transaction_start(sh);
	int temp1 = xs_mkdir(sh, st, path);
	int temp2 = xs_write(sh, st, path, path_data, strlen(path_data));
	xs_transaction_end(sh, st, 0);

	xs_close(sh);

	if(!temp1 || !temp2)
		return -1;

	return 0;
}

#endif


int ndp_init()
{
	int exit_status = 1;

	srandom(ndp_rdtsc());

	struct shm_parameters* shm_params;
	void *shm_begins;

	#ifdef NDP_BUILDING_FOR_XEN
		//setup_msg.magic_number = NDP_XEN_SETUP_MAGIC_NUMBER;

		struct shm_parameters aux_shm_params;
		shm_params = &aux_shm_params;
	#else
		struct ndp_instance_setup_request setup_msg;

		pid_t pid = getpid();

		setup_msg.magic_number = NDP_INSTANCE_SETUP_MAGIC_NUMBER;
		setup_msg.pid = pid;

		shm_params = &setup_msg.shm_params;
	#endif

	ndp_socket_index_t num_sockets = 100;

	//TODO maybe move this to some centralized, MTU-related, option/define set
	ring_size_t num_ring_elements = 128;
	ndp_packet_buffer_index_t num_packet_buffers = 100;

	if(NDP_MTU < 9000)
	{
		num_ring_elements = 512;
		//num_ring_elements = 1024;

		num_packet_buffers = 300;
		//num_packet_buffers = 1000;
	}

	init_shm_parameters(128,						//num_comm_ring_elements
						num_sockets,				//num_sockets
						num_ring_elements,			//num_tx_ring_elements
						num_ring_elements,			//num_rtx_ring_elements
						num_ring_elements,			//num_ack_tx_ring_elements
						num_ring_elements,			//num_rx_ring_elements
						num_ring_elements,			//num_ack_rx_ring_elements
						num_packet_buffers,			//num_tx_packet_buffers
						num_packet_buffers,			//num_rx_packet_buffers
						NDP_MTU,					//packet_buffer_mtu
						shm_params);

	/*printf("%lu\n%lu\n%lu\n",	\
			aligned_ndp_tx_packet_buffer_size(&setup_msg.shm_params),	\
			aligned_ndp_socket_size(&setup_msg.shm_params),	\
			shm_size(&setup_msg.shm_params));*/

	#ifdef NDP_BUILDING_FOR_XEN

		lib.gh = xengntshr_open(NULL, 0);

		if(!lib.gh)
		{
			printf("could not open grant handle\n");
			exit_status = -111;
		    goto just_exit;
		}

		size_t sz = shm_size(shm_params) + sizeof(struct shm_parameters);

		//always dom0
		lib.grant_result = alloc_for_grant(lib.gh, 0, sz);

		if(lib.grant_result.retval)
		{
			exit_status = -112;
			goto just_exit;
		}

		shm_begins = lib.grant_result.mem;

		//hacky hacky
		char *temp = (char*)shm_begins + ZZZ_PAGE_SIZE * lib.grant_result.num_pages;
		temp -= sizeof(struct shm_parameters);

		struct shm_parameters *back_shm_params = (struct shm_parameters*)temp;
		*back_shm_params = *shm_params;

	#else
		//mmap global comm ring
		int shm_fd = shm_open(GLOBAL_COMM_RING_SHM_NAME, O_RDWR, 0);
		if(shm_fd < 0)
			LOG_FMSG_DO(LOGLVL1, goto just_exit, "shm_open error for GCR; %s", strerror(errno));

		void *mmap_result = mmap(NULL, global_comm_ring_size(NUM_GLOBAL_COMM_RING_ELEMENTS),
				PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

		if(mmap_result == (void*)-1)
			LOG_FMSG_DO(LOGLVL1, goto just_exit, "mmap error for GCR; %s", strerror(errno));

		lib.global_comm_ring = mmap_result;

		//setup shared memory

		//char name_buf[100];
		init_lib_instance_name(lib.shm_name, pid);

		shm_fd = shm_open(lib.shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
		if (shm_fd < 0)
			LOG_FMSG_DO(LOGLVL1, goto just_exit, "shm_open error ; %s", strerror(errno));

		if(ftruncate(shm_fd, shm_size(&setup_msg.shm_params)) < 0)
			LOG_FMSG_DO(LOGLVL1, goto unlink, "ftruncate error ; %s", strerror(errno));

		shm_begins = mmap(NULL, shm_size(&setup_msg.shm_params), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if(shm_begins == (void*)-1)
			LOG_FMSG_DO(LOGLVL1, goto unlink, "mmap error ; %s", strerror(errno));
	#endif

	prepare_ndp_instance_info(shm_begins, shm_params, &lib.instance_info);

	if(init_shm())
		LOG_FMSG_DO(LOGLVL1, goto unlink, "%s -> init_shm error", __func__);

	//printf("sfsg\n");


	//handshake with core process

	/*we just busy wait here for now for comm space to become available; this shouldn't take long
	during normal operation, but can also potentially hang forever
	TODO fix in "real" implementation */

	//while(!multi_produce_at_most(lib_data.global_comm_ring, &setup_msg, sizeof(setup_msg), 1))

	#ifdef NDP_BUILDING_FOR_XEN
		if(write_grant_to_store(lib.grant_result.meta_ref, lib.grant_result.num_pages))
		{
			free_alloc_for_grant(lib.gh, &lib.grant_result);
			lib.grant_result.retval = -100;

			exit_status = -119;
			goto just_exit;
		}

	#else
		while(1)
		{
			//PLS_DONT_OPTIMIZE_AWAY_MY_READS();

			MY_LOCK_ACQUIRE(lib.global_comm_ring->p_sync);

			ring_size_t available = PRODUCER_AVAILABLE_ACQUIRE(&lib.global_comm_ring->inner);

			if(available)
				PRODUCE_RELEASE(&lib.global_comm_ring->inner, setup_msg);

			MY_LOCK_RELEASE(lib.global_comm_ring->p_sync);

			if(available)
				break;

			SPIN_BODY();
		}
	#endif

	//busy wait again for status change ; TODO fix in "real" implementation

	//while(!consume_at_most(lib_data.instance_info.comm_ring, &lib_data.instance_info.id, sizeof(ndp_instance_id_t)))

	volatile struct ndp_instance_flags *flags = &lib.instance_info.shm_begins->flags;

	while(!flags->core_instance_id_is_valid)
		SPIN_BODY();

	ACQUIRE_FENCE();

	lib.instance_info.id = lib.instance_info.shm_begins->instance_id;
	lib.local_addr = lib.instance_info.shm_begins->local_addr;
	lib.cycles_per_timeout = lib.instance_info.shm_begins->tsc_hz * ((double)NDP_TIMEOUT_INTERVAL_NS / NS_PER_SECOND);

	if(lib.instance_info.id < 0)
		LOG_FMSG_DO(LOGLVL1, goto unlink, "core handshake error %" PRId64, lib.instance_info.id);

	//signal that the core can start consuming from the comm_ring
	//lib_data.instance_info.shm_begins->flags.lib_done_handshake = 1;

	//LOG_FMSG(LOGLVL9, "core handshake OK; instance id is %" PRId64, lib.instance_info.id);



	//setup rings for available sockets and descriptors
	//TODO what descriptors? also the name of the following variable can be nicer & clearer
	size_t aux_size_t = (NDP_LIB_MAX_SOCKETS + 1); // * sizeof(ndp_socket_index_t);

	if(! (lib.av_sock_ring = malloc(RING_SIZE(RING_BUF(ndp_socket_index_t), aux_size_t))) )
		LOG_MSG_DO(LOGLVL1, goto unlink, "malloc NULL for lib_data.av_sock_ring");

	RING_INIT(lib.av_sock_ring, aux_size_t);

	for(ndp_socket_index_t i = 0; i < num_sockets; i++)
	{
		//if(!produce_at_most(lib_data.av_sock_ring, &i, sizeof(ndp_socket_index_t), 1))
		//	LOG_MSG_DO(LOGLVL1, goto unlink, "!produce_at_most(lib_data.av_sock_ring, &i, sizeof(ndp_socket_index_t), 1)");

		if(!PRODUCER_AVAILABLE(lib.av_sock_ring))
			LOG_MSG_DO(LOGLVL1, goto unlink, "!PRODUCER_AVAILABLE(lib_data.av_sock_ring)");

		PRODUCE(lib.av_sock_ring, i);
	}


	#if	NDP_LIB_RECORD_SW_BUF_SIZE
		if(two_headed_buffer_init(&lib.record_sw_buf, NDP_LIB_RECORD_SW_BUF_SIZE))
			LOG_FMSG_DO(LOGLVL1, goto unlink, "%s -> !two_headed_buffer_init(&lib.record_sw_buf", __func__);
	#endif


	lib.timeout_thread_may_run = 1;

	//TODO pinning timeout thread to core 0
	cpu_set_t mask;
	pthread_attr_t to_thread_attr;

	CPU_ZERO(&mask);
	pthread_attr_init(&to_thread_attr);
	CPU_SET(0, &mask);
	pthread_attr_setaffinity_np(&to_thread_attr, sizeof(cpu_set_t), &mask);

	if(pthread_create(&lib.timeout_thread, &to_thread_attr, timeout_thread_function, NULL))
		LOG_FMSG_DO(LOGLVL1, goto unlink, "%s -> error creating timeout_thread", __func__);

	lib.ndp_close_lock = 0;

	exit_status = 0;


	/*(the following might not be entirely true: unlink is called regardless of exit_status
	because the shared memory zone is reference-counted (or smt like that), and won't be actually
	freed until everyone unmaps it; so if everything goes well, we just unlink in advance from
	this side; otherwise we unlink anyway*/

  unlink:
	#ifndef NDP_BUILDING_FOR_XEN
		if(shm_unlink(lib.shm_name) < 0)
			LOG_MSG(LOGLVL10, "lib unlink error");

	#endif

  just_exit:
	return(exit_status);
}
