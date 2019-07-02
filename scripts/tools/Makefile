FQA_ROOT_DIR=$(abspath $(CURDIR)/..)
-include $(FQA_ROOT_DIR)/.config.mk

ifeq ($(TARGET),ssdsim)
DIRS 	:= env_ssdsim
endif
DIRS	+= spdk nvme-cli nbio prim_proc powerctl nvme-cli-1.6 ftl_checker

all: $(DIRS)
	$(Q)ln -s ../nvme-cli/nvme bin/nvme
	$(Q)ln -s ../nbio/nbio bin/nbio
	$(Q)ln -s ../powerctl/powerctl bin/powerctl
	$(Q)ln -s ../nvme-cli-1.6/nvme bin/nvme-lkd
	$(Q)ln -s ../ftl_checker/ftl_checker bin/ftl_checker

clean: $(DIRS)

clean-all:
	$(Q)$(MAKE) clean
	$(Q)rm -rf spdk
	$(Q)rm -rf nvme-cli

$(DIRS):
	$(Q)$(MAKE) -C $@ $(MAKECMDGOALS)
	$(Q)rm -f bin/nvme bin/nbio bin/powerctl bin/nvme-lkd bin/ftl_checker

.PHONY: all clean $(DIRS)
