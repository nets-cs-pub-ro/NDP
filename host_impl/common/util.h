#ifndef NDP_COMMON_UTIL_H_
#define NDP_COMMON_UTIL_H_

#include <stdatomic.h>
#include <stddef.h>


#define NS_PER_SECOND	   1000000000ul


#define IS_POW2(x)			({typeof(x) _x = (x) ; _x && !(_x & (_x - 1));})


//#define PLS_DONT_OPTIMIZE_AWAY_READS() 		asm volatile("" ::: "memory")
#define PLS_DONT_OPTIMIZE_AWAY_MY_READS() 		atomic_signal_fence(memory_order_relaxed)


#define ACQUIRE_FENCE()			 atomic_thread_fence(memory_order_acquire)
#define RELEASE_FENCE()			 atomic_thread_fence(memory_order_release)


#define DUMMY_SPIN()

//x86 bb
//TODO at some point I clobbered memory here; probably just to prevent reads from being optimized away; was there any other reason?
#define CPU_PAUSE()				asm volatile("pause" :::)

#define SPIN_BODY()				CPU_PAUSE()



#define MY_LOCK_ACQUIRE(x)								\
{														\
	while(!__sync_bool_compare_and_swap(&(x), 0, 1)) 	\
	{													\
		SPIN_BODY();									\
	}													\
	ACQUIRE_FENCE();									\
}


//TODO I'm not so sure about this definition, but it will do for now
#define MY_LOCK_ACQUIRE_ONE_TRY(x)								\
({																\
	int result = __sync_bool_compare_and_swap(&(x), 0, 1);		\
	ACQUIRE_FENCE();											\
	result;														\
})


#define MY_LOCK_RELEASE(x)			{ RELEASE_FENCE(); (x) = 0; }


#define LIKELY(x)       		__builtin_expect(!!(x), 1)

#define UNLIKELY(x)     		__builtin_expect(!!(x), 0)



static inline size_t aligned_size(size_t at_least, unsigned int alignment)
{
	if(at_least % alignment)
		at_least = (at_least / alignment + 1) * alignment;

	return at_least;
}



#endif /* UTIL_H_ */
