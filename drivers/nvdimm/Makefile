obj-$(CONFIG_LIBNVDIMM) += libnvdimm.o
obj-$(CONFIG_BLK_DEV_PMEM) += nd_pmem.o
obj-$(CONFIG_ND_BTT) += nd_btt.o
obj-$(CONFIG_ND_BLK) += nd_blk.o

nd_pmem-y := pmem.o

nd_btt-y := btt.o

nd_blk-y := blk.o

libnvdimm-y := core.o
libnvdimm-y += bus.o
libnvdimm-y += dimm_devs.o
libnvdimm-y += dimm.o
libnvdimm-y += region_devs.o
libnvdimm-y += region.o
libnvdimm-y += namespace_devs.o
libnvdimm-y += label.o
libnvdimm-$(CONFIG_BTT) += btt_devs.o
