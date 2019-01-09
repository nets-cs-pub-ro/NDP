SUBDIRS = lib core
CLEAN_SUFFIX  = _clean
CLEAN_SUBDIRS = $(addsuffix $(CLEAN_SUFFIX), $(SUBDIRS))

.PHONY: subdirs clean $(SUBDIRS) $(CLEAN_SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(CLEAN_SUBDIRS)

$(CLEAN_SUBDIRS): %$(CLEAN_SUFFIX):
	$(MAKE) -C $* clean
