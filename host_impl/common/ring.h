#ifndef NDP_COMMON_RING_H_
#define NDP_COMMON_RING_H_


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define ZZZ_CACHE_LINE_SIZE		64


typedef uint32_t ring_size_t;


#define HELP_INVOKE(WHAT, ...)	WHAT(__VA_ARGS__)

#define TOKENIZE2(A, B)			A##B
#define TOKENIZE3(A, B, C)		A##B##C

#define GET_MACRO5(_1, _2, _3, _4, _5, NAME, ...) NAME



#define RING_BUF1(A)					ring_buf__##A
#define RING_BUF2(A,B)					HELP_INVOKE(TOKENIZE3, RING_BUF1(A), __, B)
#define RING_BUF3(A,B,C)				HELP_INVOKE(TOKENIZE3, RING_BUF2(A,B), __, C)
#define RING_BUF4(A,B,C,D)				HELP_INVOKE(TOKENIZE3, RING_BUF3(A,B,C), __, D)
#define RING_BUF5(A,B,C,D,E)			HELP_INVOKE(TOKENIZE3, RING_BUF4(A,B,C,D), __, E)

#define RING_BUF(...)					GET_MACRO5(__VA_ARGS__,RING_BUF5,RING_BUF4,RING_BUF3,RING_BUF2,RING_BUF1)(__VA_ARGS__)

#define ROUND_RING_BUF_AUX(RNAME)		round_##RNAME
#define ROUND_RING_BUF(...)				HELP_INVOKE(ROUND_RING_BUF_AUX, RING_BUF(__VA_ARGS__))


#define RING_BUF_BODY(T) \
{ \
	ring_size_t	cconsumer_index; \
	ring_size_t	num_elements; \
	char padding[56]; \
	ring_size_t pproducer_index; \
	T buf[]; \
} __attribute__((aligned));

#define ROUND_RING_BUF_BODY(T) \
{ \
	ring_size_t cconsumer_index; \
	ring_size_t num_elements_mask; \
	char padding[56]; \
	ring_size_t pproducer_index; \
	T buf[]; \
} __attribute__((aligned));


#define DEF_RING_BUF(T, ...)			struct RING_BUF(__VA_ARGS__) RING_BUF_BODY(T)
#define DEF_ROUND_RING_BUF(T, ...)  	struct ROUND_RING_BUF(__VA_ARGS__) ROUND_RING_BUF_BODY(T)


#define GET_PRODUCER_INDEX(R)			((R)->pproducer_index)
#define GET_CONSUMER_INDEX(R)			((R)->cconsumer_index)


#define SET_PRODUCER_INDEX(R, V)		(GET_PRODUCER_INDEX(R) = (V))
#define SET_CONSUMER_INDEX(R, V)		(GET_CONSUMER_INDEX(R) = (V))


//WARNING: these do not automatically wrap the index
#define GET_ELEMENT_AT(R, IDX)			((R)->buf[IDX])
#define SET_ELEMENT_AT(R, IDX, V)		((R)->buf[IDX] = (V))

//these do
#define GET_ELEMENT_AT_WRAPPED(R, IDX)		({ typeof(R) _R = (R); GET_ELEMENT_AT(_R, RING_WRAP_INDEX(_R, (IDX))); })
#define SET_ELEMENT_AT_WRAPPED(R, IDX, V)	{ typeof(R) _R = (R); SET_ELEMENT_AT(_R, RING_WRAP_INDEX(_R, (IDX)), (V)); }



#define PRODUCE_AUX(R, V, RELEASE)	\
{	\
	typeof(R) _R = (R);	\
	ring_size_t _IDX = GET_PRODUCER_INDEX(_R); \
	SET_ELEMENT_AT(_R, _IDX, V); \
	RELEASE ;	\
	SET_PRODUCER_INDEX(_R, RING_WRAP_INDEX(_R, _IDX + 1));	\
}

#define PRODUCE(R, V)			PRODUCE_AUX(R, V, )
#define PRODUCE_RELEASE(R, V)	PRODUCE_AUX(R, V, RELEASE_FENCE())


#define CONSUME_AUX(R, FENCE)	\
({	\
	typeof(R) _R = (R);	\
	ring_size_t _IDX = GET_CONSUMER_INDEX(_R); \
	typeof(_R->buf[0]) _V = GET_ELEMENT_AT(_R, _IDX);	\
	FENCE;	\
	SET_CONSUMER_INDEX(_R, RING_WRAP_INDEX(_R, _IDX + 1));	\
	_V ;\
})

#define CONSUME(R)				CONSUME_AUX(R, )
#define CONSUME_RELEASE(R)		CONSUME_AUX(R, RELEASE_FENCE())




#define RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME)				TOKENIZE2(RNAME, __num_elements)
#define PRODUCER_AVAILABLE_FUNCTION(RNAME)					TOKENIZE2(RNAME, __producer_available)
#define PRODUCER_AVAILABLE_ACQUIRE_FUNCTION(RNAME)			TOKENIZE2(RNAME, __producer_available_acquire)
#define CONSUMER_AVAILABLE_FUNCTION(RNAME)					TOKENIZE2(RNAME, __consumer_available)
#define CONSUMER_AVAILABLE_ACQUIRE_FUNCTION(RNAME)			TOKENIZE2(RNAME, __consumer_available_acquire)
#define RING_BUF_WRAP_INDEX_FUNCTION(RNAME)					TOKENIZE2(RNAME, __wrap_index)
#define PRODUCER_ADVANCE_FUNCTION(RNAME)					TOKENIZE2(RNAME, __producer_advance)
#define CONSUMER_ADVANCE_FUNCTION(RNAME)					TOKENIZE2(RNAME, __consumer_advance)
#define RING_BUF_READ_FUNCTION(RNAME)						TOKENIZE2(RNAME, __read)
#define RING_BUF_WRITE_FUNCTION(RNAME)						TOKENIZE2(RNAME, __write)
#define RING_BUF_RESET_FUNCTION(RNAME)						TOKENIZE2(RNAME, __reset)
#define RING_BUF_INIT_FUNCTION(RNAME)						TOKENIZE2(RNAME, __init)
#define RING_BUF_SIZE_FUNCTION(RNAME)						TOKENIZE2(RNAME, __size)
#define RING_BUF_ALIGNED_SIZE_FUNCTION(RNAME)				TOKENIZE2(RNAME, __aligned_size)
#define RING_BUF_ALIGNED_NUM_ELEMENTS_FUNCTION(RNAME)		TOKENIZE2(RNAME, __aligned_num_elements)




#define OP_DISPATCH_RING(OP, ...)	\
struct RING_BUF(__VA_ARGS__)* :			OP(RING_BUF(__VA_ARGS__))


#define OP_DISPATCH_ROUND_RING(OP, ...)	\
struct ROUND_RING_BUF(__VA_ARGS__)* :	OP(ROUND_RING_BUF(__VA_ARGS__))


#define RING_META_COMMON(F, A)							  \
F(A, char)												, \
F(A, void, ptr)											, \
F(A, ndp_socket_index_t)								, \
F(A, ndp_packet_buffer_index_t)							, \
F(A, struct, ndp_header)								, \
F(A, struct, ndp_instance_setup_request)				, \
F(A, struct, ndp_packet_headers)						, \
F(A, struct, ndp_instance_op_msg)

/*
#define RING_META_LIB(F, A)								  \
F(A, struct, ndp_instance_op_msg)
*/

#define RING_META_CORE(F, A)							  \
F(A, struct, core_socket_descriptor, ptr)


#if defined(BUILDING_NDP_CORE)
  #define OP_DISPATCH(R, OP)		_Generic(*((R*)0), 	  \
  RING_META_COMMON(OP_DISPATCH_RING, OP)				, \
  RING_META_COMMON(OP_DISPATCH_ROUND_RING, OP)			, \
  RING_META_CORE(OP_DISPATCH_RING, OP)					, \
  RING_META_CORE(OP_DISPATCH_ROUND_RING, OP) )
/*
#elif defined(BUILDING_NDP_LIB)
  #define OP_DISPATCH(R, OP)		_Generic(*((R*)0), 	  \
  RING_META_COMMON(OP_DISPATCH_RING, OP)				, \
  RING_META_COMMON(OP_DISPATCH_ROUND_RING, OP)			, \
  RING_META_LIB(OP_DISPATCH_RING, OP)					, \
  RING_META_LIB(OP_DISPATCH_ROUND_RING, OP)	)
*/
#else
  #define OP_DISPATCH(R, OP)		_Generic(*((R*)0), 	  \
  RING_META_COMMON(OP_DISPATCH_RING, OP)				, \
  RING_META_COMMON(OP_DISPATCH_ROUND_RING, OP) )
#endif



#define RING_NUM_ELEMENTS(R)						OP_DISPATCH(typeof(R), RING_BUF_NUM_ELEMENTS_FUNCTION) (R)
#define PRODUCER_AVAILABLE(R)						OP_DISPATCH(typeof(R), PRODUCER_AVAILABLE_FUNCTION) (R)
#define PRODUCER_AVAILABLE_ACQUIRE(R)				OP_DISPATCH(typeof(R), PRODUCER_AVAILABLE_ACQUIRE_FUNCTION) (R)
#define CONSUMER_AVAILABLE(R)						OP_DISPATCH(typeof(R), CONSUMER_AVAILABLE_FUNCTION) (R)
#define CONSUMER_AVAILABLE_ACQUIRE(R)				OP_DISPATCH(typeof(R), CONSUMER_AVAILABLE_ACQUIRE_FUNCTION) (R)
#define RING_WRAP_INDEX(R, ...)						OP_DISPATCH(typeof(R), RING_BUF_WRAP_INDEX_FUNCTION) (R, __VA_ARGS__)
#define PRODUCER_ADVANCE(R, ...)					OP_DISPATCH(typeof(R), PRODUCER_ADVANCE_FUNCTION) (R, __VA_ARGS__)
#define CONSUMER_ADVANCE(R, ...)					OP_DISPATCH(typeof(R), CONSUMER_ADVANCE_FUNCTION) (R, __VA_ARGS__)
#define RING_READ(R, ...)							OP_DISPATCH(typeof(R), RING_BUF_READ_FUNCTION) (R, __VA_ARGS__)
#define RING_WRITE(R, ...)							OP_DISPATCH(typeof(R), RING_BUF_WRITE_FUNCTION) (R, __VA_ARGS__)
#define RING_RESET(R)								OP_DISPATCH(typeof(R), RING_BUF_RESET_FUNCTION) (R)
#define RING_INIT(R, ...)							OP_DISPATCH(typeof(R), RING_BUF_INIT_FUNCTION) (R, __VA_ARGS__)

//TODO make the SIZE related functions use a ring* parameter; also see if it helps with the annoying declaration issues
#define RING_SIZE(RNAME, ...)						OP_DISPATCH(struct RNAME*, RING_BUF_SIZE_FUNCTION) (__VA_ARGS__)

#define RING_ALIGNED_SIZE(RNAME, ...)				OP_DISPATCH(struct RNAME*, RING_BUF_ALIGNED_SIZE_FUNCTION) (__VA_ARGS__)
#define RING_ALIGNED_NUM_ELEMENTS(RNAME, ...)		OP_DISPATCH(struct RNAME*, RING_BUF_ALIGNED_NUM_ELEMENTS_FUNCTION) (__VA_ARGS__)



#define AUX_NUM_ELEMENTS(R)				((R)->num_elements)
#define AUX_ROUND_NUM_ELEMENTS(R)		((R)->num_elements_mask + 1)

#define DEF_RING_BUF_NUM_ELEMENTS(RNAME, HOW_NUM_ELEMENTS)	\
static inline ring_size_t RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME) (const struct RNAME *r) \
{ 	\
	return HOW_NUM_ELEMENTS(r);	\
}


#define PRODUCER_AVAILABLE_BODY(RNAME, FENCE)	\
{ \
	ring_size_t pi = GET_PRODUCER_INDEX(r); \
	ring_size_t ci = GET_CONSUMER_INDEX(r); \
	FENCE; \
	return ci - pi - 1 + (ci <= pi) * RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME)(r);	\
}

#define DEF_PRODUCER_AVAILABLE(RNAME)	\
static inline ring_size_t PRODUCER_AVAILABLE_FUNCTION(RNAME)	(const struct RNAME *r)	\
PRODUCER_AVAILABLE_BODY(RNAME, )

#define DEF_PRODUCER_AVAILABLE_ACQUIRE(RNAME)	\
static inline ring_size_t PRODUCER_AVAILABLE_ACQUIRE_FUNCTION(RNAME)	(const struct RNAME *r)	\
PRODUCER_AVAILABLE_BODY(RNAME, ACQUIRE_FENCE())


#define CONSUMER_AVAILABLE_BODY(RNAME, FENCE)	\
{ \
	ring_size_t pi = GET_PRODUCER_INDEX(r); \
	ring_size_t ci = GET_CONSUMER_INDEX(r); \
	FENCE; \
	return pi - ci + (pi < ci) * RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME)(r); \
}

#define DEF_CONSUMER_AVAILABLE(RNAME)	\
static inline ring_size_t CONSUMER_AVAILABLE_FUNCTION(RNAME)	(const struct RNAME *r)	\
CONSUMER_AVAILABLE_BODY(RNAME, )

#define DEF_CONSUMER_AVAILABLE_ACQUIRE(RNAME)	\
static inline ring_size_t CONSUMER_AVAILABLE_ACQUIRE_FUNCTION(RNAME)	(const struct RNAME *r)	\
CONSUMER_AVAILABLE_BODY(RNAME, ACQUIRE_FENCE())



#define DEF_RING_BUF_WRAP_INDEX(RNAME, WRAP_AROUND_OP)	\
static inline ring_size_t RING_BUF_WRAP_INDEX_FUNCTION(RNAME) (const struct RNAME *r, ring_size_t index)	\
{	\
	return index WRAP_AROUND_OP; \
}


#define DEF_PRODUCER_ADVANCE(RNAME)	\
static inline void PRODUCER_ADVANCE_FUNCTION(RNAME)	(struct RNAME *r, ring_size_t num_pos) \
{ \
	SET_PRODUCER_INDEX(r, RING_BUF_WRAP_INDEX_FUNCTION(RNAME)(r, GET_PRODUCER_INDEX(r) + num_pos)); \
}


#define DEF_CONSUMER_ADVANCE(RNAME)	\
static inline void CONSUMER_ADVANCE_FUNCTION(RNAME)	(struct RNAME *r, ring_size_t num_pos) \
{ \
	SET_CONSUMER_INDEX(r, RING_BUF_WRAP_INDEX_FUNCTION(RNAME)(r, GET_CONSUMER_INDEX(r) + num_pos)); \
}


#define DEF_RING_BUF_READ(RNAME)	\
static inline void RING_BUF_READ_FUNCTION(RNAME) (const struct RNAME *r, void *dst, ring_size_t start_index, ring_size_t num_elements) \
{ \
	ring_size_t bytes_left = (RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME)(r) - start_index) * sizeof(r->buf[0]); \
	ring_size_t count_bytes	= num_elements * sizeof(r->buf[0]);	\
																\
	if(bytes_left >= count_bytes) \
		memcpy(dst, r->buf + start_index, count_bytes); \
	else \
	{ \
		memcpy(dst, r->buf + start_index, bytes_left); \
		memcpy((char*)dst + bytes_left, r->buf, count_bytes - bytes_left); \
	} \
}


#define DEF_RING_BUF_WRITE(RNAME)	\
static inline void RING_BUF_WRITE_FUNCTION(RNAME) (struct RNAME *r, const void *src, ring_size_t start_index, ring_size_t num_elements) \
{ \
	ring_size_t bytes_left = (RING_BUF_NUM_ELEMENTS_FUNCTION(RNAME)(r) - start_index) * sizeof(r->buf[0]); \
	ring_size_t count_bytes	= num_elements * sizeof(r->buf[0]);	\
																\
	if(bytes_left >= count_bytes) \
		memcpy(r->buf + start_index, src, count_bytes); \
	else \
	{ \
		memcpy(r->buf + start_index, src, bytes_left); \
		memcpy(r->buf, (const char*)src + bytes_left, count_bytes - bytes_left); \
	} \
}


#define DEF_RING_BUF_RESET(RNAME)		\
static inline void RING_BUF_RESET_FUNCTION(RNAME) (struct RNAME *r) \
{ \
	SET_PRODUCER_INDEX(r, 0); \
	SET_CONSUMER_INDEX(r, 0); \
}


#define DEF_RING_BUF_INIT(...)		\
static inline void RING_BUF_INIT_FUNCTION(RING_BUF(__VA_ARGS__)) (struct RING_BUF(__VA_ARGS__) *r, ring_size_t num_elements) \
{ \
	r->num_elements = num_elements; \
	RING_BUF_RESET_FUNCTION(RING_BUF(__VA_ARGS__))(r); \
}


#define DEF_ROUND_RING_BUF_INIT(...)		\
static inline void RING_BUF_INIT_FUNCTION(ROUND_RING_BUF(__VA_ARGS__)) (struct ROUND_RING_BUF(__VA_ARGS__) *r, ring_size_t num_elements) \
{ \
	if (!IS_POW2(num_elements))	\
	{	\
		printf("%s: round condition not satisfied\n", __func__); \
		exit(1); \
	}	\
	r->num_elements_mask = num_elements - 1; \
	RING_BUF_RESET_FUNCTION(ROUND_RING_BUF(__VA_ARGS__))(r); \
}


#define DEF_RING_BUF_SIZE(T, ...)	\
static inline size_t RING_BUF_SIZE_FUNCTION(RING_BUF(__VA_ARGS__))	(ring_size_t num_elements)	\
{	\
	return offsetof(struct RING_BUF(__VA_ARGS__), buf) + num_elements * sizeof(T);	\
}


#define DEF_ROUND_RING_BUF_SIZE(T, ...)	\
static inline size_t RING_BUF_SIZE_FUNCTION(ROUND_RING_BUF(__VA_ARGS__))	(ring_size_t num_elements)	\
{	\
	if (!IS_POW2(num_elements))	\
	{	\
		printf("%s: round condition not satisfied\n", __func__); \
		exit(1); \
	}	\
	return offsetof(struct ROUND_RING_BUF(__VA_ARGS__), buf) + num_elements * sizeof(T);	\
}


#define DEF_RING_BUF_ALIGNED_SIZE(RNAME)	\
static inline size_t RING_BUF_ALIGNED_SIZE_FUNCTION(RNAME)	(ring_size_t num_elements, unsigned int alignment)	\
{	\
	return aligned_size(RING_BUF_SIZE_FUNCTION(RNAME)(num_elements), alignment);	\
}


#define DEF_RING_BUF_ALIGNED_NUM_ELEMENTS(T, RNAME)	\
static inline ring_size_t RING_BUF_ALIGNED_NUM_ELEMENTS_FUNCTION(RNAME) (ring_size_t at_least, unsigned int alignment)	\
{	\
	return (RING_BUF_ALIGNED_SIZE_FUNCTION(RNAME)(at_least, alignment) -		\
			offsetof(struct RNAME, buf)) / sizeof(T);	\
}





#define DEF_RING_BUF_OPS(T, ...) \
DEF_RING_BUF_NUM_ELEMENTS(RING_BUF(__VA_ARGS__), AUX_NUM_ELEMENTS) \
DEF_PRODUCER_AVAILABLE(RING_BUF(__VA_ARGS__)) \
DEF_PRODUCER_AVAILABLE_ACQUIRE(RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_AVAILABLE(RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_AVAILABLE_ACQUIRE(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_WRAP_INDEX(RING_BUF(__VA_ARGS__), % r->num_elements) \
DEF_PRODUCER_ADVANCE(RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_ADVANCE(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_READ(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_WRITE(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_RESET(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_INIT(__VA_ARGS__) \
DEF_RING_BUF_SIZE(T, __VA_ARGS__) \
DEF_RING_BUF_ALIGNED_SIZE(RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_ALIGNED_NUM_ELEMENTS(T, RING_BUF(__VA_ARGS__)) \


#define DEF_ROUND_RING_BUF_OPS(T, ...) \
DEF_RING_BUF_NUM_ELEMENTS(ROUND_RING_BUF(__VA_ARGS__), AUX_ROUND_NUM_ELEMENTS) \
DEF_PRODUCER_AVAILABLE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_PRODUCER_AVAILABLE_ACQUIRE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_AVAILABLE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_AVAILABLE_ACQUIRE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_WRAP_INDEX(ROUND_RING_BUF(__VA_ARGS__), & r->num_elements_mask) \
DEF_PRODUCER_ADVANCE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_CONSUMER_ADVANCE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_READ(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_WRITE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_RESET(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_ROUND_RING_BUF_INIT(__VA_ARGS__) \
DEF_ROUND_RING_BUF_SIZE(T, __VA_ARGS__) \
DEF_RING_BUF_ALIGNED_SIZE(ROUND_RING_BUF(__VA_ARGS__)) \
DEF_RING_BUF_ALIGNED_NUM_ELEMENTS(T, ROUND_RING_BUF(__VA_ARGS__)) \



#define DEF_RINGS(T, ...)	\
DEF_RING_BUF(T, __VA_ARGS__)	\
DEF_RING_BUF_OPS(T, __VA_ARGS__)	\
DEF_ROUND_RING_BUF(T, __VA_ARGS__)	\
DEF_ROUND_RING_BUF_OPS(T, __VA_ARGS__)	\


DEF_RINGS(char, char)
DEF_RINGS(void*, void, ptr)

//ROUND_RING_META(DEF_ROUND_RING_BUF, , )
//ROUND_RING_META(DEF_ROUND_RING_BUF_OPS, , )

//#define RING_BUF_SIZE(T, NUM_ELEMENTS)	( offsetof(struct RING_BUF(char), buf) + (NUM_ELEMENTS) * sizeof(T) )
//#define ROUND_RING_BUF_SIZE(T, NUM_ELEMENTS_MASK)	( offsetof(struct ROUND_RING_BUF(char), buf) + ((NUM_ELEMENTS_MASK) + 1) * sizeof(T) )



#endif /* NDP_COMMON_RING_H_ */
