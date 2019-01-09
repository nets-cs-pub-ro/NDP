#include "../../common/defs.h"
#include "../../common/helpers.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <xengnttab.h>
#include <xenstore.h>


static volatile int keep_running = 1;

static struct global_communication_ring *global_comm_ring = NULL;



static void ctrlc_handler(int x __attribute__((unused)))
{
	keep_running = 0;
}


void look_for_grants(xengnttab_handle *gh)
{
	struct xs_handle *sh = xs_open(0);

	if (!sh)
	{
		printf("xs_open error\n");
		exit(1);
	}

	char path[1024];
	xs_transaction_t st;
	unsigned int num_doms;

	st = xs_transaction_start(sh);
	char **doms = xs_directory(sh, st, "/local/domain", &num_doms);
	xs_transaction_end(sh, st, 0);

	if (!doms)
	{
		printf("!doms\n");
		exit(1);
	}

	for (int i = 0; i < num_doms; i++)
	{
		uint32_t domid = atoi(doms[i]);

		if (domid == 0)
			continue;

		//printf("domid %u\n", domid);

		sprintf(path, "/local/domain/%u/data/ndp_grants", domid);
		unsigned int num_grants;

		st = xs_transaction_start(sh);
		char **grants = xs_directory(sh, st, path, &num_grants);
		xs_transaction_end(sh, st, 0);

		if (!grants)
			continue;

		for (int j = 0; j < num_grants; j++)
		{
			uint32_t main_ref = atol(grants[j]);

			unsigned int value_len;
			sprintf(path, "/local/domain/%u/data/ndp_grants/%u", domid,	main_ref);

			st = xs_transaction_start(sh);
			char *value = xs_read(sh, st, path, &value_len);
			xs_transaction_end(sh, st, 0);

			if (!value)
			{
				printf("error reading num_pages for grant\n");
				continue;
			}

			size_t num_pages = atoll(value);

			printf("domid %u main_ref %u num_pages %lu\n", domid, main_ref, num_pages);

			st = xs_transaction_start(sh);
			if (!xs_rm(sh, st, path))
				printf("error removing grant path from store\n");
			xs_transaction_end(sh, st, 0);

			/*
			struct map_grant_result r = map_grant(gh, domid, main_ref, num_pages);

			if (r.retval)
				printf("r retval = %d\n", r.retval);
			else {
				printf("map grant ok\n");

				int num_bytes = ZZZ_PAGE_SIZE * r.num_pages;
				printf("num_bytes %u sum %u\n", num_bytes, checksum(r.mem, num_bytes));

				sleep(1);
				unmap_grant(&r);
			}
			*/

			struct ndp_instance_setup_request req;
			struct ndp_xen_grant_params *xgp = &req.grant_params;

			req.magic_number = NDP_XEN_SETUP_MAGIC_NUMBER;

			xgp->domid = domid;
			xgp->main_ref = main_ref;
			xgp->num_pages = num_pages;

			while(1)
			{
				//PLS_DONT_OPTIMIZE_AWAY_MY_READS();

				//MY_LOCK_ACQUIRE(lib.global_comm_ring->p_sync);

				ring_size_t available = PRODUCER_AVAILABLE_ACQUIRE(&global_comm_ring->inner);

				if(available)
					PRODUCE_RELEASE(&global_comm_ring->inner, req);

				//MY_LOCK_RELEASE(lib.global_comm_ring->p_sync);

				if(available)
					break;

				SPIN_BODY();
			}
		}

		free(grants);
	}

	free(doms);

	xs_close(sh);
}



int main(int argc, char ** argv)
{
	if(signal(SIGINT, ctrlc_handler) == SIG_ERR)
	{
		printf("could not register ctrl-c handler\n");
		exit(1);
	}

	xengnttab_handle *gh = xengnttab_open(NULL, 0);
	if(!gh)
	{
		printf("could not open xengnttab_handle\n");
		exit(1);
	}

	int shm_fd = shm_open(GLOBAL_COMM_RING_SHM_NAME, O_RDWR, 0);
	if(shm_fd < 0)
	{
		printf("shm_open error\n");
		exit(1);
	}

	global_comm_ring = mmap(NULL, global_comm_ring_size(NUM_GLOBAL_COMM_RING_ELEMENTS),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	if(global_comm_ring == (void*)-1)
	{
		printf("mmap_error\n");
		exit(1);
	}


	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = 400 * 1000 * 1000;

	while(keep_running)
	{
		look_for_grants(gh);

		nanosleep(&tim , NULL);
	}


	xengnttab_close(gh);
}
