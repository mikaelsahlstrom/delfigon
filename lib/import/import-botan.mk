BOTAN_PORT_DIR := $(call select_from_ports,botan)

ifeq ($(filter-out $(SPECS),arm),)
	REP_INC_DIR += include/botan/32bit
	INC_DIR += $(BOTAN_PORT_DIR)/include/botan-generic
endif

ifeq ($(filter-out $(SPECS),x86_32),)
	REP_INC_DIR += include/botan/32bit
	INC_DIR += $(BOTAN_PORT_DIR)/include/botan-i386
endif

ifeq ($(filter-out $(SPECS),x86_64),)
	REP_INC_DIR += include/botan/64bit
	INC_DIR += $(BOTAN_PORT_DIR)/include/botan-amd64
endif
