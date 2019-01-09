#include "defs.h"
#include "helpers.h"
#include "hoover_ndp.h"
#include <pthread.h>
#include <sys/mman.h>


int ndp_terminate(void)
{
	lib.timeout_thread_may_run = 0;

	int ret = pthread_join(lib.timeout_thread, NULL);

	if(ret)
		LOG_FMSG(LOGLVL10, "%s -> pthread_join error %d", __func__, ret);

	if(!comm_ring_write(NDP_INSTANCE_OP_TERMINATE, 0))
		LOG_FMSG_DO(LOGLVL1, return -1, "%s -> !comm_ring_write(NDP_INSTANCE_OP_TERMINATE, 0)", __func__);


	volatile struct ndp_instance_flags *f = &lib.instance_info.shm_begins->flags;

	//TODO another busy waiting (although most likely a short wait, this code is not really performance critical or anything
	while(!f->core_terminate_acknowledged)
		SPIN_BODY();

	ACQUIRE_FENCE();

	#ifdef NDP_BUILDING_FOR_XEN
		if(lib.grant_result.retval == 0)
			free_alloc_for_grant(lib.gh, &lib.grant_result);

		if(lib.gh)
		{
			if(xengntshr_close(lib.gh))
			    printf("xengntshr_close err\n");
		}
	#else
		if(munmap(lib.global_comm_ring, global_comm_ring_size(NUM_GLOBAL_COMM_RING_ELEMENTS)))
			LOG_FMSG(LOGLVL10, "%s -> error unmapping global comm ring", __func__);

		if(munmap(lib.instance_info.shm_begins, shm_size(&lib.instance_info.shm_params)))
			LOG_FMSG(LOGLVL10, "%s -> error unmapping shm area", __func__);
	#endif

	return 0;
}
