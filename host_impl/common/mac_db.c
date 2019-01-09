#include "mac_db.h"
#include <string.h>


#define MAC6(_1, _2, _3, _4, _5, _6)		{{ 0x##_1, 0x##_2, 0x##_3, 0x##_4, 0x##_5, 0x##_6 }}


#define MAX_INTERFACES		10

struct computer_mac_info
{
	unsigned int computer_idx;
	unsigned int num_interfaces;
	struct ndp_mac_addr interface_mac[MAX_INTERFACES];
};


static const struct computer_mac_info gaina_mac_info [] =
{
		{0, 2, { MAC6(90, e2, ba, 46, f2, 64), MAC6(90, e2, ba, 46, f2, 65) } },

		{1, 4, { MAC6(90, e2, ba, 46, f2, e8), MAC6(90, e2, ba, 46, f2, e9),
				 MAC6(90, e2, ba, 46, f2, 44), MAC6(90, e2, ba, 46, f2, 45) } },

		{2, 2, { MAC6(90, e2, ba, 46, f2, 04), MAC6(90, e2, ba, 46, f2, 05) } },
		{3, 2, { MAC6(90, e2, ba, 46, f0, 38), MAC6(90, e2, ba, 46, f0, 39) } },
		{4, 2, { MAC6(90, e2, ba, 46, f2, 68), MAC6(90, e2, ba, 46, f2, 69) } },
		{5, 2, { MAC6(90, e2, ba, 46, f2, 24), MAC6(90, e2, ba, 46, f2, 25) } },
		{6, 2, { MAC6(90, e2, ba, 46, f3, 44), MAC6(90, e2, ba, 46, f3, 45) } },
		{7, 2, { MAC6(90, e2, ba, 46, f1, 20), MAC6(90, e2, ba, 46, f1, 21) } },
		{8, 2, { MAC6(90, e2, ba, 46, f0, 34), MAC6(90, e2, ba, 46, f0, 35) } },
};


static const struct computer_mac_info cocos_mac_info [] =
{
		{0 , 2, { MAC6(a0, 36, 9f, 5f, db, c0), MAC6(a0, 36, 9f, 5f, db, c1)} },
		{1 , 2, { MAC6(a0, 36, 9f, 5f, db, d0), MAC6(a0, 36, 9f, 5f, db, d1)} },
		{2 , 2, { MAC6(a0, 36, 9f, 5f, dd, 34), MAC6(a0, 36, 9f, 5f, dd, 35)} },
		{3 , 2, { MAC6(a0, 36, 9f, 5c, 5a, ac), MAC6(a0, 36, 9f, 5c, 5a, ad)} },
		{4 , 2, { MAC6(a0, 36, 9f, 5c, 52, f0), MAC6(a0, 36, 9f, 5c, 52, f1)} },
		{5 , 2, { MAC6(a0, 36, 9f, 5f, dc, c8), MAC6(a0, 36, 9f, 5f, dc, c9)} },
		{6 , 2, { MAC6(a0, 36, 9f, 5f, dc, cc), MAC6(a0, 36, 9f, 5f, dc, cd)} },
		{7 , 2, { MAC6(a0, 36, 9f, 5c, 52, f4), MAC6(a0, 36, 9f, 5c, 52, f5)} },
		{8 , 2, { MAC6(a0, 36, 9f, 5c, 53, 14), MAC6(a0, 36, 9f, 5c, 53, 15)} },
		{9 , 4, { MAC6(a0, 36, 9f, 5f, dc, f8), MAC6(a0, 36, 9f, 5f, dc, f9),
				  MAC6(90, e2, ba, 46, f0, 34), MAC6(90, e2, ba, 46, f0, 35) }},
		{10, 2, { MAC6(a0, 36, 9f, 5f, db, bc), MAC6(a0, 36, 9f, 5f, db, bd) }},
		{11, 2, { MAC6(a0, 36, 9f, 5c, 55, 30), MAC6(a0, 36, 9f, 5c, 55, 31) }},
		{12, 2, { MAC6(a0, 36, 9f, 5f, db, 30), MAC6(a0, 36, 9f, 5f, db, 31) }},
		{13, 2, { MAC6(a0, 36, 9f, 5f, f6, b8), MAC6(a0, 36, 9f, 5f, f6, b9) }},
		{14, 2, { MAC6(a0, 36, 9f, 5c, 57, 08), MAC6(a0, 36, 9f, 5c, 57, 09) }},
		{15, 2, { MAC6(a0, 36, 9f, 5c, 58, 7c), MAC6(a0, 36, 9f, 5c, 58, 7d) }},
		{16, 2, { MAC6(a0, 36, 9f, 5c, 58, 84), MAC6(a0, 36, 9f, 5c, 58, 85) }},
		{17, 2, { MAC6(a0, 36, 9f, 5c, 58, e8), MAC6(a0, 36, 9f, 5c, 58, e9) }},
		{18, 2, { MAC6(a0, 36, 9f, 5c, 57, 18), MAC6(a0, 36, 9f, 5c, 57, 19) }},
		{19, 2, { MAC6(a0, 36, 9f, 5c, 58, ec), MAC6(a0, 36, 9f, 5c, 58, ed) }}
};


static const struct computer_mac_info cam_mac_info [] =
{
		{0 , 1, { MAC6(90, e2, ba, 40, 8b, c9) } },	//server4
		{1 , 1, { MAC6(00, 1b, 21, af, 62, d9) } },	//server5
		{2 , 1, { MAC6(00, 1b, 21, af, 61, 65) } }, //server6
		{3 , 1, { MAC6(90, e2, ba, 27, fb, e1) } }, //server7
		{4 , 1, { MAC6(90, e2, ba, 27, fc, 7d) } }, //server8
		{5 , 1, { MAC6(90, e2, ba, 27, fb, 8d) } }, //server9
		{6 , 1, { MAC6(00, 1b, 21, ab, e2, 39) } }, //server10
		{7 , 1, { MAC6(90, e2, ba, 27, fb, c8) } }  //server113
};


int gaina_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst)
{
	if(computer_idx >= sizeof(gaina_mac_info) / sizeof(struct computer_mac_info))
		return -1;

	if(interface_idx >= gaina_mac_info[computer_idx].num_interfaces)
		return -2;

	memcpy(dst, &gaina_mac_info[computer_idx].interface_mac[interface_idx], sizeof(struct ndp_mac_addr));

	return 0;
}


int cocos_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst)
{
	if(computer_idx >= sizeof(cocos_mac_info) / sizeof(struct computer_mac_info))
		return -1;

	if(interface_idx >= cocos_mac_info[computer_idx].num_interfaces)
		return -2;

	memcpy(dst, &cocos_mac_info[computer_idx].interface_mac[interface_idx], sizeof(struct ndp_mac_addr));

	return 0;
}


int cam_get_mac(unsigned int computer_idx, unsigned int interface_idx, struct ndp_mac_addr *dst)
{
	if(computer_idx >= sizeof(cam_mac_info) / sizeof(struct computer_mac_info))
		return -1;

	if(interface_idx >= cam_mac_info[computer_idx].num_interfaces)
		return -2;

	memcpy(dst, &cam_mac_info[computer_idx].interface_mac[interface_idx], sizeof(struct ndp_mac_addr));

	return 0;
}
