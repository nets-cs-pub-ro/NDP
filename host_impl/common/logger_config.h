#ifndef NDP_COMMON_LOGGER_CONFIG_H_
#define NDP_COMMON_LOGGER_CONFIG_H_


#define LOGLVL1					1
#define LOGLVL2					1
#define LOGLVL3					1
#define LOGLVL4					1
#define LOGLVL5					1
#define LOGLVL6					1
#define LOGLVL7					1
#define LOGLVL8					1
#define LOGLVL9					1
#define LOGLVL10				1

#define LOGLVL99				1
#define LOGLVL100				1
#define LOGLVL101				1
#define LOGLVL102				1

/*
#define LOGLVL200				1			//core; arrival & departure of DATA, ACK & PULL packets
#define LOGLVL201				1			//lib; arrival & departure of DATA & ACK packets
#define LOGLVL202				1			//core; arrival & departure of NACK packets
#define LOGLVL203				1			//lib; arrival of NACK packets
#define LOGLVL204				1			//lib; timeout detected
*/

//core; arrival & departure of DATA, ACK & PULL packets
#define LOG200_MSG				0
#define LOG200_REC				0

//core; arrival of chopped DATA packets
#define LOG205_MSG				0
#define LOG205_REC				0

//lib; arrival & departure of DATA & ACK packets
#define LOG201_MSG				0
#define LOG201_REC				0

//core; arrival & departure of NACK packets
#define LOG202_MSG				0
#define LOG202_REC				0

//lib; arrival of NACK packets
#define LOG203_MSG				0
#define LOG203_REC				0

//lib; timeout detected
#define LOG204_MSG				1
#define LOG204_REC				0


#endif /* NDP_COMMON_LOGGER_CONFIG_H_ */
