#ifndef NDP_COMMON_CONFIG_H_
#define NDP_COMMON_CONFIG_H_


#include "debug_config.h"


#define ENV_GAINA				1
#define ENV_GAINA_FPGA			2
#define ENV_COCOS				3
#define ENV_CAM					4


//here because the stupid core_socket_descriptor definition is at the end of "common/defs.h"
#define NDP_CORE_NACKS_FOR_HOLES						0
#define NDP_CORE_NUM_N4H_RING_ELEMENTS					512


#define NDP_MTU								1500


#define NDP_TSC_HZ_ESTIMATION_SLEEP_NS		(NS_PER_SECOND * 3)

#define NDP_LIB_MAX_SHM_NAME_BUF_SIZE			256


//#define NDP_CORE_RECORD_HW_TS_TX_MAX			2000000
#define NDP_CORE_RECORD_HW_TS_TX_MAX			0
#define NDP_CORE_RECORD_HW_TS_RX_MAX			0

//POI = packet(s) of interest
#define NDP_CORE_RECORD_HW_TS_POI				NDP_HEADER_FLAG_PULL

//sampling rate for POI; 1 = every packet; 2 = every second packet; & so on
#define NDP_CORE_RECORD_HW_TS_ONE_IN			5

#define NDP_CORE_RECORD_HW_TS_ANY			(NDP_CORE_RECORD_HW_TS_TX_MAX || NDP_CORE_RECORD_HW_TS_RX_MAX)




#endif /* NDP_COMMON_CONFIG_H_ */
