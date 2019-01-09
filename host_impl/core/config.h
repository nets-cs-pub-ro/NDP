#ifndef NDP_CORE_CONFIG_H_
#define NDP_CORE_CONFIG_H_



#define NDP_CORE_SEPARATE_LCORE_FOR_PULLQ				1

#define NDP_CORE_PRIOPULLQ_4COSTIN						0


#define NDP_CORE_RECORD_SW_BUF_SIZE						(200 * 1000 * 1000)


//one in this many data packets is artificially chopped before being sent
#define NDP_CORE_SEND_CHOPPED_RATIO 					0
//#define NDP_CORE_SEND_CHOPPED_RATIO 					1000




//************* RECORD RELATED STUFF *************


//#define NDP_CORE_RECORD_SW_TX_MAX			(10 * 1000 * 1000)
//#define NDP_CORE_RECORD_SW_TX_MAX			0
//#define NDP_CORE_RECORD_SW_RX_MAX			(10 * 1000 * 1000)
//#define NDP_CORE_RECORD_SW_RX_MAX			0

//#define NDP_CORE_RECORD_SW_ANY				(NDP_CORE_RECORD_SW_TX_MAX || NDP_CORE_RECORD_SW_RX_MAX)

//************* END OF RECORD RELATED STUFF *************



#include "../common/config.h"


#endif /* NDP_CORE_CONFIG_H_ */
