#ifndef NDP_COMMON_LOGGER_H_
#define NDP_COMMON_LOGGER_H_

#include "defs.h"
#include "logger_config.h"
#include <stdint.h>
#include <stdlib.h>



void log_msg_partial(const char *format, ...);
void log_msg(const char *format, ...);


#define LOG_MSG_DO(level, action, msg)					{ if(level) log_msg(msg); action; }
#define LOG_FMSG_DO(level, action, format, ...)			{ if(level) log_msg(format, __VA_ARGS__); action; }


#define LOG_MSG(level, msg)								LOG_MSG_DO(level, , msg)
#define LOG_FMSG(level, format, ...)					LOG_FMSG_DO(level, , format, __VA_ARGS__)




#define LOG_RECORD_CORE_PULL_SENT						1
#define LOG_RECORD_CORE_DATA_SENT						2
#define LOG_RECORD_CORE_DATA_RTX						3
#define LOG_RECORD_CORE_DATA_TO_RTX						4
#define LOG_RECORD_CORE_ACK_SENT						5
#define LOG_RECORD_CORE_NACK_SENT						6
#define LOG_RECORD_CORE_DATA_RCVD						7
#define LOG_RECORD_CORE_ACK_RCVD						8
#define LOG_RECORD_CORE_NACK_RCVD						9
#define LOG_RECORD_CORE_PULL_RCVD						10
#define LOG_RECORD_CORE_HDR_RCVD						11
#define LOG_RECORD_CORE_HDR_IMPL						12


#define LOG_RECORD_LIB_DATA_SENT						41
#define LOG_RECORD_LIB_ACK_SENT							42
#define LOG_RECORD_LIB_DATA_RCVD						43
#define LOG_RECORD_LIB_OLD_DATA_RCVD					44
#define LOG_RECORD_LIB_ACK_RCVD							45
#define LOG_RECORD_LIB_OLD_ACK_RCVD						46
#define LOG_RECORD_LIB_NACK_RCVD						47
#define LOG_RECORD_LIB_OLD_NACK_RCVD					48
#define LOG_RECORD_LIB_TIMEOUT							49


typedef uint8_t log_record_type_t;
typedef uint8_t log_instance_id_t;
typedef uint8_t log_socket_index_t;

struct generic_log_record
{
	log_record_type_t type;
	log_instance_id_t instance_id;
	log_socket_index_t socket_index;
	ndp_sequence_number_t seq;
	ndp_cycle_count_t ts;
} __attribute__((packed));


struct two_headed_buffer
{
	uint64_t front_index;
	uint64_t num_front_entries;

	uint64_t back_index;
	uint64_t num_back_entries;

	uint64_t size;
	char *buf;
};



int two_headed_buffer_init(struct two_headed_buffer *b, uint64_t size);
void write_two_headed_buffer_to_file(const struct two_headed_buffer *b, FILE *f);



static inline void fill_generic_record(struct generic_log_record *r, log_record_type_t type,
		ndp_instance_id_t instance_id, ndp_socket_index_t socket_index, ndp_sequence_number_t seq, ndp_cycle_count_t ts)
{
	r->type = type;
	r->instance_id = instance_id;
	r->socket_index = socket_index;
	r->seq = seq;
	r->ts = ts;
}

static inline void write_generic_record_front(struct two_headed_buffer *b, log_record_type_t type,
		ndp_instance_id_t instance_id, ndp_socket_index_t socket_index, ndp_sequence_number_t seq, ndp_cycle_count_t ts)
{
	struct generic_log_record *r = (struct generic_log_record*)&b->buf[b->front_index];
	fill_generic_record(r, type, instance_id, socket_index, seq, ts);
	b->front_index += sizeof(struct generic_log_record);
	b->num_front_entries++;
}

static inline void write_generic_record_back(struct two_headed_buffer *b, log_record_type_t type,
		ndp_instance_id_t instance_id, ndp_socket_index_t socket_index, ndp_sequence_number_t seq, ndp_cycle_count_t ts)
{
	b->back_index -= sizeof(struct generic_log_record);
	struct generic_log_record *r = (struct generic_log_record*)&b->buf[b->back_index];
	fill_generic_record(r, type, instance_id, socket_index, seq, ts);
	b->num_back_entries++;
}



#endif /* NDP_COMMON_LOGGER_H_ */
