#ifndef NDP_COMMON_MAC_DB_H_
#define NDP_COMMON_MAC_DB_H_

#include "defs.h"



int gaina_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst);

int cocos_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst);

int cam_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst);


#endif /* MAC_DB_H_ */
