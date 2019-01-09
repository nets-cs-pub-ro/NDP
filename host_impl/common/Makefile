COMMON_SRC_DIRS  := $(shell find ./common -type d -print)
CORE_SRC_DIRS  := $(shell find ./core -type d -print)
LIB_SRC_DIRS  := $(shell find ./lib -type d -print)
TEST_SRC_DIRS := $(shell find ./test -type d -print)

COMMON_SRC_FILES := $(wildcard $(addsuffix /*.c, $(COMMON_SRC_DIRS)))
CORE_SRC_FILES := $(wildcard $(addsuffix /*.c, $(CORE_SRC_DIRS)))
LIB_SRC_FILES := $(wildcard $(addsuffix /*.c, $(LIB_SRC_DIRS)))
TEST_SRC_FILES := $(wildcard $(addsuffix /*.c, $(TEST_SRC_DIRS)))

COMMON_OBJ_FILES := $(COMMON_SRC_FILES:.c=.o)
CORE_OBJ_FILES := $(CORE_SRC_FILES:.c=.o)
LIB_OBJ_FILES := $(LIB_SRC_FILES:.c=.o)
TEST_OBJ_FILES := $(TEST_SRC_FILES:.c=.o)

ALL_OBJ_FILES := $(COMMON_OBJ_FILES) $(CORE_OBJ_FILES) $(LIB_OBJ_FILES) $(TEST_OBJ_FILES)
DEP_FILES := $(ALL_OBJ_FILES:.o=.d)

CXX := gcc
CXXFLAGS := -Wall -Wundef -Werror -O2

CORE_OUT_NAME := ./core/core
LIB_OUT_NAME := ./lib/lib
RING_TEST_OUT_NAME := ./test/ring_test

all : $(CORE_OUT_NAME) $(LIB_OUT_NAME) $(RING_TEST_OUT_NAME)

$(CORE_OUT_NAME): $(COMMON_OBJ_FILES) $(CORE_OBJ_FILES)	
	$(CXX) $(CXXFLAGS) -o $(CORE_OUT_NAME) $(COMMON_OBJ_FILES) $(CORE_OBJ_FILES) -lrt
	
$(LIB_OUT_NAME): $(COMMON_OBJ_FILES) $(LIB_OBJ_FILES)	
	$(CXX) $(CXXFLAGS) -o $(LIB_OUT_NAME) $(COMMON_OBJ_FILES) $(LIB_OBJ_FILES) -lrt
	
$(RING_TEST_OUT_NAME): $(COMMON_OBJ_FILES) ./test/ring_test.o	
	$(CXX) $(CXXFLAGS) -o $(RING_TEST_OUT_NAME) $(COMMON_OBJ_FILES) ./test/ring_test.o -lrt
	

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
	rm -f $(CORE_OUT_NAME) $(LIB_OUT_NAME) $(RING_TEST_OUT_NAME) $(DEP_FILES) $(ALL_OBJ_FILES)


