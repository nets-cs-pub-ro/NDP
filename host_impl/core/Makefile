ifndef RTE_SDK
  $(error RTE_SDK is not set)
endif

ifndef RTE_TARGET
  $(error RTE_TARGET is not set)
endif

ifndef NDP_IMPL_DIR
  $(error NDP_IMPL_DIR is not set)
endif

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP := core

# all source are stored in SRCS-y
SRCS-y := $(wildcard $(addsuffix /core/*.c, $(NDP_IMPL_DIR)))
SRCS-y += $(wildcard $(addsuffix /common/*.c, $(NDP_IMPL_DIR)))
#SRCS-y := init.c main.c

CFLAGS += $(WERROR_FLAGS)

# workaround for a gcc bug with noreturn attribute
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12603
ifeq ($(CONFIG_RTE_TOOLCHAIN_GCC),y)
CFLAGS_main.o += -Wno-return-type
endif

EXTRA_CFLAGS += -O2 -Wfatal-errors -Winline -Wundef -D BUILDING_NDP_CORE -D BUILDING_WITH_DPDK -D NDP_EXTRA_DEBUG_CONFIG=\"../core/debug_config.h\"

include $(RTE_SDK)/mk/rte.extapp.mk
