ifeq ($V, 1)
	Q = 
else
	Q = @
	MAKE += --no-print-directory
endif
export Q

FQA_ROOT_DIR	:= $(abspath $(CURDIR))
SPDK_ROOT_DIR	:= $(FQA_ROOT_DIR)/tools/spdk
CP 				:= cp
DIRS 			:= tools
MODS			:= tools/nvme-cli/Makefile tools/nvme-cli/env.spdk.mk
DOLLAR			:= $$

-include .config.mk

MODS			+= $(SPDK_ROOT_DIR)/Makefile $(SPDK_ROOT_DIR)/lib/Makefile $(SPDK_ROOT_DIR)/examples/Makefile $(SPDK_ROOT_DIR)/include/spdk/mmio.h

ifeq ($(TARGET),)
all:
	@echo "usage: make [ssdsim] or [native]"
else
all: target $(DIRS)
endif

target:
	@echo "* Build target is" $(TARGET)

$(DIRS):
	$(Q)$(MAKE) -C $@ $(MAKECMDGOALS)

setup: source.csh source.bash
	$(Q)rm -rf $(SPDK_ROOT_DIR)
	$(Q)tar xf tools/archive/spdk.tar.xz -C tools
	$(Q)tar xf tools/archive/nvme-cli.tar.xz -C tools
	$(Q)tar xzf tools/archive/nvme-cli-1.6.tar.gz -C tools
	$(Q)$(MAKE) .pre-setup 
	$(Q)sed -e "s/json//" -e "s/cunit//" -e "s/net//" -e "s/ioat//" -e "s/rpc//" -e "s/virtio//" -e "s/nvmf//" -e "s/iscsi//" -e "s/scsi//" -e "s/bdev//" -e "s/blobfs//" -e "s/blob//" -e "s/event//" -e "s/atoi//" -e "s/lvol//" -e "s/copy//" -e "s/nbd//" -i $(SPDK_ROOT_DIR)/lib/Makefile
	$(Q)sed -e "s/bdev//" -e "s/blob//" -e "s/ioat//" -e "s/nbd//" -i $(SPDK_ROOT_DIR)/examples/Makefile
	$(Q)sed -e "s/app//" -i $(SPDK_ROOT_DIR)/Makefile
	$(Q)$(MAKE) -C $(SPDK_ROOT_DIR) mk/cc.mk config.h
	-$(Q)unlink tools/nvme-cli/spdk
	$(Q)ln -s $(SPDK_ROOT_DIR) tools/nvme-cli/spdk
	$(Q)sed -e "s/intel-nvme.o //" -e "s/lnvm-nvme.o //" -e "s/memblaze-nvme.o //" -e "s/wdc-nvme.o //" -e "s/huawei-nvme.o //" -i tools/nvme-cli/Makefile
	$(Q)$(MAKE)
	$(Q)$(MAKE) .post-setup
	$(Q)echo " * type 'source source.csh' or 'source source.bash' depending on your shell"

ssdsim:
	$(Q)$(CP) ssdsim.mk .config.mk
	$(Q)$(MAKE) setup

native:
	$(Q)$(CP) native.mk .config.mk
	$(Q)$(MAKE) setup

clean: $(DIRS)

clean-all: $(DIRS)
	$(Q)rm -f source.bash
	$(Q)rm -f source.csh
	$(Q)rm -f .config.mk
	$(Q)rm -f setup.sh

run: $(DIRS)

source.csh:
	$(Q)echo "setenv FQA_TARGET $(TARGET)" > $@
ifeq ($(TARGET), native)
	$(Q)echo "setenv FQA_DEVICE `lspci | grep 6150 | awk '{printf \"0000:%s\", $(DOLLAR)1}'`" >> $@
	$(Q)echo "if (! $(DOLLAR)""?FQA_DEVICE"" ) then" >> $@
	$(Q)echo "  echo \"DUT not found. set FQA_DEVICE manually.\"" >> $@
	$(Q)echo "endif" >> $@
	$(Q)echo "setenv FQA_SPDK_MEM_SIZE 4096" >> $@
else
	$(Q)echo "setenv FQA_DEVICE 0000:00:00.0" >> $@
endif
	$(Q)echo "setenv FQA_SPDK 1" >> $@
	$(Q)echo "setenv FQA_SPDK_SHM_ID 0" >> $@
	$(Q)echo "if (! $(DOLLAR)""?LD_LIBRARY_PATH"" ) then" >> $@
	$(Q)echo "  setenv LD_LIBRARY_PATH $(FQA_ROOT_DIR)/tools/spdk/build/lib" >> $@
	$(Q)echo "else" >> $@
	$(Q)echo "  setenv LD_LIBRARY_PATH ""$(DOLLAR)""{LD_LIBRARY_PATH}:$(FQA_ROOT_DIR)/tools/spdk/build/lib" >> $@
	$(Q)echo "endif" >> $@
	$(Q)echo "set path = ( $(FQA_ROOT_DIR)/tools/bin ""$(DOLLAR)""path )" >> $@

source.bash:
	$(Q)echo "export FQA_TARGET=$(TARGET)" > $@
ifeq ($(TARGET), native)
	$(Q)echo "export FQA_DEVICE=`lspci | grep 6150 | awk '{printf \"0000:%s\", $(DOLLAR)1}'`" >> $@
	$(Q)echo "if [[ -z $(DOLLAR)""FQA_DEVICE"" ]]; then" >> $@
	$(Q)echo "  echo \"DUT not found. set FQA_DEVICE manually.\"" >> $@
	$(Q)echo "fi" >> $@
	$(Q)echo "export FQA_SPDK_MEM_SIZE=4096" >> $@
else
	$(Q)echo "export FQA_DEVICE=0000:00:00.0" >> $@
endif
	$(Q)echo "export FQA_SPDK=1" >> $@
	$(Q)echo "export FQA_SPDK_SHM_ID=0" >> $@
	$(Q)echo "export LD_LIBRARY_PATH=""$(DOLLAR)""LD_LIBRARY_PATH:$(FQA_ROOT_DIR)/tools/spdk/build/lib" >> $@
	$(Q)echo "export PATH=$(FQA_ROOT_DIR)/tools/bin:""$(DOLLAR)""PATH" >> $@

.PHONY: all sim native target prepare clean clean-all run setup source.csh source.bash $(DIRS)
