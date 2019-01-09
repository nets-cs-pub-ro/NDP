#ifndef NDP_COMMON_TSC_H_
#define NDP_COMMON_TSC_H_


#include "defs.h"
#include <inttypes.h>
#include <stdint.h>
#include <time.h>


double ndp_estimate_tsc_hz(uint64_t sleep_time_ns);

union rdtsc_result
{
	uint64_t value;
	struct
	{
		uint32_t lo;
		uint32_t hi;
	};
};

static inline ndp_cycle_count_t ndp_rdtsc(void)
{
	union rdtsc_result result;

	asm volatile("rdtsc" : "=a"(result.lo), "=d"(result.hi) :: "memory");

	return result.value;
}


static inline ndp_cycle_count_t ndp_read_start_cycles(void)
{
	union rdtsc_result result;

	asm volatile ("CPUID\n\t"
	          	  "RDTSC\n\t"
	          	  	: "=a"(result.lo), "=d"(result.hi)
					:: "%rbx", "%rcx", "memory");

	return result.value;
}


static inline ndp_cycle_count_t ndp_read_end_cycles(void)
{
	union rdtsc_result result;

	asm volatile("RDTSCP\n\t"
	             "mov %%edx, %0\n\t"
	             "mov %%eax, %1\n\t"
				 "CPUID\n\t"
			     : "=r" (result.hi), "=r" (result.lo)
				 :: "%rax", "%rbx", "%rcx", "%rdx", "memory");

	return result.value;
}



#endif /* NDP_COMMON_TSC_H_ */
