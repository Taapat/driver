#ifndef _PLATFORM_7200_H_
#define _PLATFORM_7200_H_

static struct resource tkdma_resource_7200[] = {
        [0] = { .start = 0xFDB68000,
                .end   = 0xFDB6FFFF,
                .flags = IORESOURCE_MEM,     },
        [1] = { .start = ILC_IRQ(28),
                .end   = ILC_IRQ(28),
                .flags = IORESOURCE_IRQ, },
};

struct platform_device tkdma_device_7200 = {
        .name = "tkdma",
        .id = -1,
        .num_resources = ARRAY_SIZE(tkdma_resource_7200),
        .resource = tkdma_resource_7200,
        .dev = { .platform_data = &tkdma_core }
};

struct platform_device h264pp_device_7200 = {
	.name          = "h264pp",
	.id            = -1,
	.num_resources =  4,
	.resource      = (struct resource[]) {
		[0] = { .start = 0xFD900000,
			.end   = 0xFD900FFF,
			.flags = IORESOURCE_MEM, },
		[1] = { .start = ILC_IRQ(44),
			.end   = ILC_IRQ(44),
			.flags = IORESOURCE_IRQ, },		
		[2] = { .start = 0xFD920000,
			.end   = 0xFD920FFF,
			.flags = IORESOURCE_MEM, },
		[3] = { .start = ILC_IRQ(48),
			.end   = ILC_IRQ(48),
			.flags = IORESOURCE_IRQ, },
	},
};

struct platform_device dvp_device_7200 = {
    .name          = "dvp",
    .id            = -1,
    .num_resources =  2,
    .resource      = (struct resource[]) {
        [0] = { .start = 0xFDA40000,
                .end   = 0xFDA40FFF,
                .flags = IORESOURCE_MEM,},
        [1] = { .start = ILC_IRQ(46), 
                .end   = ILC_IRQ(46),
                .flags = IORESOURCE_IRQ },
    },
};

static struct platform_device *platform_7200[] __initdata = {
	&h264pp_device_7200,
        &dvp_device_7200,
	&tkdma_device_7200
};

static __init int platform_init_7200(void)
{
    static int *syscfg40;
    static int *syscfg7;
        
    // Configure DVP
    syscfg40 = ioremap(0xfd7041a0,4);
    *syscfg40 = 1 << 16;

    syscfg7 = ioremap(0xfd70411c,4);
    *syscfg7 = *syscfg7 | (1 << 29);

#ifdef BOARD_SPECIFIC_CONFIG
    register_board_drivers();
#endif

    return platform_add_devices(platform_7200,sizeof(platform_7200)/sizeof(struct platform_device*));

}

module_init(platform_init_7200);
#endif
