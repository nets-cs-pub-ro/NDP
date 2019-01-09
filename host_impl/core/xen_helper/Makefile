all:	core_xen_helper

DEPS_C := ../../common/helpers.c ../../common/logger.c ../../common/mac_db.c
DEPS_H := ../../common/defs.h ../../common/helpers.h ../../common/ring.h ../../common/logger.h ../../common/mac_db.h

core_xen_helper:	main.c $(DEPS_C) $(DEPS_H)
	gcc -o core_xen_helper -O2 -Wall -Werror -Winline -D NDP_BUILDING_FOR_XEN main.c $(DEPS_C) -lrt -lxenstore -lxengnttab 