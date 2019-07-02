TARGET			= native
SPDK_ROOT_DIR	?= $(FQA_ROOT_DIR)/tools/spdk
SPDK_LIBS		= -L$(SPDK_ROOT_DIR)/build/lib -lspdk -L$(SPDK_ROOT_DIR)/dpdk/build/lib -ldpdk -lrte_bus_pci -lrte_bus_vdev -lrte_eal -lrte_mempool -lrte_pci -ldl -luuid -lnuma
SPDK_INCS		= -I$(SPDK_ROOT_DIR)/include

.pre-setup:
	$(Q)rm -rf $(SPDK_ROOT_DIR)/dpdk
	$(Q)tar xf tools/archive/dpdk.tar.xz -C $(SPDK_ROOT_DIR)
	$(Q)cd $(SPDK_ROOT_DIR) && ./configure --disable-tests --without-fio --without-vhost --without-virtio --without-pmdk --without-vpp --without-rbd --without-rdma --without-iscsi-initiator --without-vtune
	$(Q)sed -e "s/ -lrdmacm//" -e "s/ -libverbs//" -i $(FQA_ROOT_DIR)/tools/nvme-cli/env.spdk.mk
	$(Q)rm -f $(FQA_ROOT_DIR)/setup.sh
	$(Q)echo "#!/usr/bin/env bash" > $(FQA_ROOT_DIR)/setup.sh
	$(Q)echo "HUGEMEM=4096 $(SPDK_ROOT_DIR)/scripts/setup.sh $$""@" >> $(FQA_ROOT_DIR)/setup.sh
	$(Q)chmod +x $(FQA_ROOT_DIR)/setup.sh

.post-setup:
	$(Q)echo "* Type './setup.sh' prior to run tests"

