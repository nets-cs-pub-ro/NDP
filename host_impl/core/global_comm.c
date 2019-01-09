#include "../common/logger.h"
#include <errno.h>
#include <fcntl.h>
#include "helpers.h"
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/stat.h>


#ifdef NDP_BUILDING_FOR_XEN

static size_t refs_at_level_aux(size_t n, size_t level, size_t *result)
{
  size_t x = num_pages_for_bytes(n * sizeof(uint32_t));
  size_t this_level;

  if(x == 1)
    this_level = 1;
  else
    this_level = 1 + refs_at_level_aux(x, level, result);

  if(this_level == level)
    *result = n;

  return this_level;
}


static size_t refs_at_level(size_t num_pages, size_t level)
{
  size_t result = 0;
  refs_at_level_aux(num_pages, level, &result);
  return result;
}


static struct map_grant_result map_grant(xengnttab_handle *gh, int32_t domid, uint32_t main_ref, size_t num_pages)
{
	struct map_grant_result result;
	memset(&result, 0, sizeof(result));

	result.num_pages = num_pages;

	size_t level = 0;
	size_t refs_this_level = 1;
	uint32_t *refs = &main_ref;
	uint32_t *mem;

	while(1)
	{
		int prot = PROT_READ;
		if(level && refs_this_level == num_pages)
		prot |= PROT_WRITE;

		mem = xengnttab_map_domain_grant_refs(gh, refs_this_level, domid, refs, prot);

		if(level)
		{
			int x = xengnttab_unmap(gh, refs, num_pages_for_bytes(refs_this_level * sizeof(uint32_t)));
			if(x)
				printf("xengntab_unmap err %d\n", x);
		}

		if(!mem)
		{
			result.retval = -2;
			goto return_result;
		}

		if(level && refs_this_level == num_pages)
		{
			result.mem = mem;
			break;
		}
		else
		{
			level++;
			refs_this_level = refs_at_level(num_pages, level);

			if(!refs_this_level)
			{
				printf("ERROR ERROR !refs_this_level\n");
				exit(1);
			}

			refs = mem;
		}
	}

  return_result:
	return result;
}


#endif


void check_global_comm_ring(void)
{
	struct ndp_instance_setup_request msg;

	//while(consume_at_most(core_data.global_comm_ring, &msg, sizeof(msg)))
	while(CONSUMER_AVAILABLE_ACQUIRE(core.global_comm_ring))
	{
		msg = CONSUME_RELEASE(core.global_comm_ring);

		#ifdef NDP_BUILDING_FOR_XEN
			if (UNLIKELY (msg.magic_number != NDP_XEN_SETUP_MAGIC_NUMBER))
				LOG_FMSG_DO(LOGLVL2, continue, "msg.magic_number (%" PRIu64 ") != XEN_MAGIC_NUMBER",
						msg.magic_number);
		#else
			if (UNLIKELY (msg.magic_number != NDP_INSTANCE_SETUP_MAGIC_NUMBER))
				LOG_FMSG_DO(LOGLVL2, continue, "msg.magic_number (%" PRIu64 ") != SHM_PARAMETERS_MAGIC_NUMBER",
						msg.magic_number);
		#endif

		struct core_instance_descriptor *d = get_available_instance_descriptor();
		struct shm_parameters *shm_params;
		void *mmap_result;

		if(UNLIKELY (!d))
			LOG_MSG_DO(LOGLVL2, continue, "check_global_comm_ring() -> cannot find_available_lib_instance_descriptor()");

		core.instance_descriptor_available[d->instance_info.id] = 0;

		#ifdef NDP_BUILDING_FOR_XEN

			struct map_grant_result r;
			r = map_grant(core.gh, msg.grant_params.domid, msg.grant_params.main_ref, msg.grant_params.num_pages);

			if(r.retval)
			{
				printf("map_grant r.retval = %d\n", r.retval);
				continue;
			}

			mmap_result = r.mem;

			d->map_grant_res = r;

			//hacky hacky
			char *temp = (char*)r.mem + ZZZ_PAGE_SIZE * r.num_pages;
			temp -= sizeof(struct shm_parameters);
			shm_params =  (struct shm_parameters*)temp;

			//printf("aaaaaa\n");

		#else

			init_lib_instance_name(d->shm_name, msg.pid);

			d->shm_fd = shm_open(d->shm_name, O_RDWR, 0);

			if(UNLIKELY (d->shm_fd < 0))
				LOG_FMSG_DO(LOGLVL1, continue, "check_global_comm_ring() -> shm_open error %s", strerror(errno));

			mmap_result = mmap(NULL, shm_size(&msg.shm_params), PROT_READ | PROT_WRITE, MAP_SHARED, d->shm_fd, 0);

			if(UNLIKELY (mmap_result == (void*)-1))
				LOG_FMSG_DO(LOGLVL1, continue, "check_global_comm_ring() -> mmap error %s", strerror(errno));

			shm_params = &msg.shm_params;

		#endif


		prepare_ndp_instance_info(mmap_result, shm_params, &d->instance_info);

		d->instance_info.shm_begins->instance_id = d->instance_info.id;

		LOG_FMSG(LOGLVL10, "check_global_comm_ring() -> registered instance with id %" PRIu64, d->instance_info.id);

		d->instance_info.shm_begins->local_addr = core.local_addr;
		d->instance_info.shm_begins->tsc_hz = core.tsc_hz;

		RELEASE_FENCE();

		d->instance_info.shm_begins->flags.core_instance_id_is_valid = 1;
		core.num_lib_instances++;


		/*
		//should pretty much never happen
		//TODO reclaim the descriptor (if not NULL) when it does happen
		//if(!produce_at_most(d->instance_info.comm_ring, &reply, sizeof(ndp_instance_id_t), 1))
		if(UNLIKELY (! PRODUCER_AVAILABLE(d->instance_info.comm_ring)))
			LOG_FMSG(LOGLVL2, "check_global_comm_ring() -> could not reply to setup request with pid %d", msg.pid)
		else if(d)
		{
			PRODUCE_RELEA
			core_data.num_lib_instances++;
		}
		*/
	}
}

