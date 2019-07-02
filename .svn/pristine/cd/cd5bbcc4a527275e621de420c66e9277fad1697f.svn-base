TARGET			= ssdsim
SPDK_ROOT_DIR	?= $(FQA_ROOT_DIR)/tools/spdk
SPDK_LIBS		= -L$(SPDK_ROOT_DIR)/build/lib -lspdk -ldl -luuid
SPDK_INCS		= -I$(SPDK_ROOT_DIR)/include

.pre-setup:
	$(Q)$(CP) $(FQA_ROOT_DIR)/tools/env_ssdsim/mmio.h $(SPDK_ROOT_DIR)/include/spdk/
	$(Q)cd $(SPDK_ROOT_DIR) && ./configure --disable-tests --without-dpdk --without-fio --without-vhost --without-virtio --without-pmdk --without-vpp --without-rbd --without-rdma --without-iscsi-initiator --without-vtune --with-env=$(FQA_ROOT_DIR)/tools/env_ssdsim
	$(Q)echo "SKIP_DPDK_BUILD?=1" >> $(SPDK_ROOT_DIR)/CONFIG.local
	$(Q)rm -f $(FQA_ROOT_DIR)/setup.sh
	$(Q)sed -e "s/env_dpdk//" -i $(SPDK_ROOT_DIR)/lib/Makefile
	$(Q)sed -e "s/spdk_env_dpdk/spdk_env_ssdsim/" -e "s/-lrte_.* //" -i $(FQA_ROOT_DIR)/tools/nvme-cli/env.spdk.mk

.post-setup:

