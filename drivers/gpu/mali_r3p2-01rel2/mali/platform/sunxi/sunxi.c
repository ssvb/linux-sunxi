/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a sunxi platform
 */

#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <asm/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"

#include <linux/module.h>
#include <linux/clk.h>
#include <mach/irqs.h>
#include <mach/clock.h>
#include <plat/sys_config.h>
#include <plat/memory.h>
#include <plat/system.h>

#ifdef CONFIG_FB_SUNXI_RESERVED_MEM
extern unsigned long fb_start;
extern unsigned long fb_size;
#endif

int mali_clk_div = 3;
module_param(mali_clk_div, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(mali_clk_div, "Clock divisor for mali");

struct clk *h_ahb_mali, *h_mali_clk, *h_ve_pll;
int mali_clk_flag=0;


_mali_osk_errcode_t mali_platform_init(void)
{
	unsigned long rate;
	int clk_div;
	int mali_used = 0;

	//get mali ahb clock
	h_ahb_mali = clk_get(NULL, "ahb_mali");
	if(!h_ahb_mali){
		MALI_PRINT(("try to get ahb mali clock failed!\n"));
	}
	//get mali clk
	h_mali_clk = clk_get(NULL, "mali");
	if(!h_mali_clk){
		MALI_PRINT(("try to get mali clock failed!\n"));
	}

	h_ve_pll = clk_get(NULL, "ve_pll");
	if(!h_ve_pll){
		MALI_PRINT(("try to get ve pll clock failed!\n"));
	}

	//set mali parent clock
	if(clk_set_parent(h_mali_clk, h_ve_pll)){
		MALI_PRINT(("try to set mali clock source failed!\n"));
	}

	//set mali clock
	rate = clk_get_rate(h_ve_pll);

	if(!script_parser_fetch("mali_para", "mali_used", &mali_used, 1)) {
		if (mali_used == 1) {
			if (!script_parser_fetch("mali_para", "mali_clkdiv", &clk_div, 1)) {
				if (clk_div > 0) {
					pr_info("mali: use config clk_div %d\n", clk_div);
					mali_clk_div = clk_div;
				}
			}
		}
	}

	pr_info("mali: clk_div %d\n", mali_clk_div);
	rate /= mali_clk_div;

	if(clk_set_rate(h_mali_clk, rate)){
		MALI_PRINT(("try to set mali clock failed!\n"));
	}

	if(clk_reset(h_mali_clk,0)){
		MALI_PRINT(("try to reset release failed!\n"));
	}

	MALI_PRINT(("mali clock set completed, clock is  %d Hz\n", rate));


	/*enable mali axi/apb clock*/
	if(mali_clk_flag == 0)
	{
		//printk(KERN_WARNING "enable mali clock\n");
		//MALI_PRINT(("enable mali clock\n"));
		mali_clk_flag = 1;
	       if(clk_enable(h_ahb_mali))
	       {
		     MALI_PRINT(("try to enable mali ahb failed!\n"));
	       }
	       if(clk_enable(h_mali_clk))
	       {
		       MALI_PRINT(("try to enable mali clock failed!\n"));
	        }
	}


    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	/*close mali axi/apb clock*/
	if(mali_clk_flag == 1)
	{
		//MALI_PRINT(("disable mali clock\n"));
		mali_clk_flag = 0;
	       clk_disable(h_mali_clk);
	       clk_disable(h_ahb_mali);
	}

    MALI_SUCCESS;
}

static void mali_platform_device_release(struct device *device);

static struct resource mali_gpu_resources_m400_mp1[] =
{
	MALI_GPU_RESOURCES_MALI400_MP1_PMU(
		0x01C40000,
		SW_INT_IRQNO_GPU_GP,  SW_INT_IRQNO_GPU_GPMMU,
		SW_INT_IRQNO_GPU_PP0, SW_INT_IRQNO_GPU_PPMMU0)
};

static struct resource mali_gpu_resources_m400_mp2[] =
{
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(
		0x01C40000,
		SW_INT_IRQNO_GPU_GP,  SW_INT_IRQNO_GPU_GPMMU,
		SW_INT_IRQNO_GPU_PP0, SW_INT_IRQNO_GPU_PPMMU0,
		SW_INT_IRQNO_GPU_PP1, SW_INT_IRQNO_GPU_PPMMU1)
};

static struct platform_device mali_gpu_device =
{
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
};

static struct mali_gpu_device_data mali_gpu_data =
{
	.shared_mem_size = 256 * 1024 * 1024, /* 256MB */
};

int mali_platform_device_register(void)
{
	int err = -1;

	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

#ifdef CONFIG_FB_SUNXI_RESERVED_MEM
	mali_gpu_data.fb_start = fb_start;
	mali_gpu_data.fb_size = fb_size;
#endif

	mali_platform_init();

	if (sunxi_is_a20()) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP2 device\n"));
		err = platform_device_add_resources(
				&mali_gpu_device,
				mali_gpu_resources_m400_mp2,
				ARRAY_SIZE(mali_gpu_resources_m400_mp2));
	} else {
		MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP1 device\n"));
		err = platform_device_add_resources(
				&mali_gpu_device,
				mali_gpu_resources_m400_mp1,
				ARRAY_SIZE(mali_gpu_resources_m400_mp1));
	}

	if (0 == err)
	{
		err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
		if (0 == err)
		{
			/* Register the platform device */
			err = platform_device_register(&mali_gpu_device);
			if (0 == err)
			{
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif

				return 0;
			}
		}

		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

	platform_device_unregister(&mali_gpu_device);
	mali_platform_deinit();
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}
