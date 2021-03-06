

struct map_desc RD_78XX0_H3C_MEM_TABLE[] __initdata = {
 { PCI0_MEM0_BASE,  	__phys_to_pfn(PCI0_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
/*
 { PCI1_MEM0_BASE,  	__phys_to_pfn(PCI1_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
 { PCI2_MEM0_BASE,  	__phys_to_pfn(PCI2_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
 { PCI3_MEM0_BASE,  	__phys_to_pfn(PCI3_MEM0_BASE),  PCIx_MEM0_SIZE, MT_DEVICE},
*/
 { INTER_REGS_BASE, 	__phys_to_pfn(INTER_REGS_BASE), _1M,  	      MT_DEVICE },
 { PCI0_IO_BASE,   	__phys_to_pfn(PCI0_IO_BASE),   	PCIx_IO_SIZE,  	MT_DEVICE},
 { PCI1_IO_BASE,   	__phys_to_pfn(PCI1_IO_BASE),   	PCIx_IO_SIZE, 	MT_DEVICE},
 { PCI2_IO_BASE,   	__phys_to_pfn(PCI2_IO_BASE),   	PCIx_IO_SIZE,  	MT_DEVICE},
 { PCI3_IO_BASE,   	__phys_to_pfn(PCI3_IO_BASE),   	PCIx_IO_SIZE, 	MT_DEVICE},
 { DEVICE_CS2_BASE, __phys_to_pfn(DEVICE_CS2_BASE), DEVICE_CS2_SIZE, MT_DEVICE }, 
 { DEVICE_CS0_BASE, __phys_to_pfn(DEVICE_CS0_BASE), DEVICE_CS0_SIZE, MT_DEVICE },
 { DEVICE_CS1_BASE, __phys_to_pfn(DEVICE_CS1_BASE), DEVICE_CS1_SIZE, MT_DEVICE },
 /*{ DEVICE_CS3_BASE, __phys_to_pfn(DEVICE_CS3_BASE), DEVICE_CS3_SIZE, MT_DEVICE },*/
 { CRYPTO_ENG_BASE,		__phys_to_pfn(CRYPTO_ENG_BASE), 	CRYPTO_SIZE, 	MT_DEVICE  },					
 { BOOTDEV_CS_BASE, __phys_to_pfn(BOOTDEV_CS_BASE), BOOTDEV_CS_SIZE, MT_DEVICE}
};

