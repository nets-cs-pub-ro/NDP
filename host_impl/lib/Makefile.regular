SRC_DIRS := $(shell find . -type d -print)
SRC_DIRS += $(shell find ../common -type d -print)
SRC_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
OBJ_FILES := $(SRC_FILES:.c=.o)
DEP_FILES := $(OBJ_FILES:.o=.d)

CXX := gcc
CXXFLAGS := -O2 -Wall -Wundef -Werror -Winline -D BUILDING_NDP_LIB -D NDP_EXTRA_DEBUG_CONFIG=\"../lib/debug_config.h\"

OUT_NAME := lib

.PHONY: clean

$(OUT_NAME): $(OBJ_FILES) 	
	$(CXX) $(CXXFLAGS) -o $(OUT_NAME) $(OBJ_FILES) -lpthread -lrt
	

-include $(DEP_FILES)

%.o: %.c
	@echo "$*.c ..."
	@$(CXX) -c $(CXXFLAGS) $*.c -o $*.o
	@$(CXX) -MM $(CXXFLAGS) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

clean:
	rm -f $(OUT_NAME) $(DEP_FILES) $(OBJ_FILES)


