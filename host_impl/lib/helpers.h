#ifndef NDP_LIB_HELPERS_H_
#define NDP_LIB_HELPERS_H_

#include "../common/helpers.h"
#include "../common/tsc.h"
#include "defs.h"
#include <errno.h>
#include <time.h>

int init_lib_socket_descriptor(struct lib_socket_descriptor *d);
void reset_lib_socket_descriptor(struct lib_socket_descriptor *d);
int set_mac_addrs(struct lib_socket_descriptor *d);

void prepare_headers(struct lib_socket_descriptor *d);
int prepare_connect_socket(struct lib_socket_descriptor *d);
int prepare_accept_socket(struct lib_socket_descriptor *d);

void reset_tx_packet_buffer(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx);
void reset_rx_packet_buffer(struct lib_socket_descriptor *d, ndp_packet_buffer_index_t idx);

//void reset_ndp_socket(struct ndp_socket_info *socket_info);

struct lib_socket_descriptor* get_available_socket();

ring_size_t comm_ring_write(ndp_instance_op_t type, ndp_socket_index_t socket_index);

//ndp_timestamp_ns_t lib_get_ts();

void check_acks(struct lib_socket_descriptor *d, int from_main_thread);

//int get_mac_address(ndp_net_addr_t ip, void *buf);


void lib_output_file_name(char *buf, size_t len, const char *middle);


static inline void __attribute__((always_inline)) lib_record_event(log_record_type_t type, ndp_instance_id_t instance_id, ndp_socket_index_t socket_index,
		ndp_sequence_number_t seq, ndp_cycle_count_t ts, int use_front)
{
	if(use_front)
		write_generic_record_front(&lib.record_sw_buf, type, instance_id, socket_index, seq, ts);
	else
		write_generic_record_back(&lib.record_sw_buf, type, instance_id, socket_index, seq, ts);

}


static inline ndp_cycle_count_t lib_read_tsc()
{
	return ndp_rdtsc();
}


#ifdef NDP_BUILDING_FOR_XEN

	void free_alloc_for_grant(xengntshr_handle *gh, const struct alloc_for_grant_result *r);

#endif



#endif /* NDP_LIB_HELPERS_H_ */
