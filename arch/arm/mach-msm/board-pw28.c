/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2011, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2011, The CyanogenMod Project 
 * Copyright (c) 2012, Geeksphone
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/power_supply.h>
#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>
#ifdef CONFIG_CACHE_L2X0
	#include <asm/hardware/cache-l2x0.h>
#endif

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/pmic.h>
#include <mach/msm_iomap.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_serial_hs.h>
#include <mach/memory.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/msm_tsif.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/android_pmem.h>
#include <mach/camera.h>

#include "devices.h"
#include "socinfo.h"
#include "clock.h"
#include "msm-keypad-devices.h"
#include "pm.h"
#ifdef CONFIG_ARCH_MSM7X27
#include <linux/msm_kgsl.h>
#endif
#include <linux/synaptics_i2c_rmi.h>
#include <linux/ssd2531.h>
#include <linux/cm3623.h>

#ifdef CONFIG_USB_ANDROID
	#include <linux/usb/android_composite.h>
#endif

#include "smd_private.h"
#define ID_SMD_UUID 12

#ifdef CONFIG_ARCH_MSM7X27
	#define MSM_PMEM_MDP_SIZE	0xDBB000
	#define MSM_PMEM_ADSP_SIZE	0x986000
	#define MSM_PMEM_AUDIO_SIZE	0x5B000
	#define MSM_FB_SIZE		0x177000
	#define MSM_GPU_PHYS_SIZE	SZ_2M
	#define PMEM_KERNEL_EBI1_SIZE	0x1C000
	
	/* Using lower 1MB of OEMSBL memory for GPU_PHYS */
	#define MSM_GPU_PHYS_START_ADDR	 0xD600000ul

	/* Using upper 1/2MB of Apps Bootloader memory*/
	#define MSM_PMEM_AUDIO_START_ADDR	0x1C000ul
#endif

#define MANU_NAME			"SIMCOM"
#define MASS_STORAGE_NAME	"SIMCOM"
#define PRUD_NAME			"PW28"
#define VID					0x05C6
#define PID					0x9018
#define ADBFN				0x1A

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = MASS_STORAGE_NAME,
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
	.platform_data  = &usb_mass_storage_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID
	static char *usb_functions_default[] = {
	#ifdef CONFIG_USB_ANDROID_DIAG
		"diag",
	#endif
	#ifdef CONFIG_USB_F_SERIAL
		   "modem",
		   "nmea",
	#endif
	#ifdef CONFIG_USB_ANDROID_RMNET
		   "rmnet",
	#endif
		   "usb_mass_storage",
	};

	static char *usb_functions_default_adb[] = {
	#ifdef CONFIG_USB_ANDROID_DIAG
		"diag",
	#endif
		   "adb",
	#ifdef CONFIG_USB_F_SERIAL
		   "modem",
		   "nmea",
	#endif
	#ifdef CONFIG_USB_ANDROID_RMNET
		   "rmnet",
	#endif
		   "usb_mass_storage",
	};

	static char *usb_functions_rndis[] = {
	#ifdef CONFIG_USB_ANDROID_RNDIS
		   "rndis",
	#endif
	};

	static char *usb_functions_rndis_adb[] = {
	#ifdef CONFIG_USB_ANDROID_RNDIS
		   "rndis",
	#endif
		   "adb",
	};

	static char *usb_functions_all[] = {
	#ifdef CONFIG_USB_ANDROID_RNDIS
		"rndis",
	#endif
	#ifdef CONFIG_USB_ANDROID_DIAG
		"diag",
	#endif
		"adb",
	#ifdef CONFIG_USB_F_SERIAL
		"modem",
		"nmea",
	#endif
	#ifdef CONFIG_USB_ANDROID_RMNET
		"rmnet",
	#endif
		"usb_mass_storage",
	#ifdef CONFIG_USB_ANDROID_ACM
		"acm",
	#endif
	};

	static struct android_usb_product usb_products[] = {
		{
		    .product_id     = 0x9026,
		    .num_functions  = ARRAY_SIZE(usb_functions_default),
		    .functions      = usb_functions_default,
		},
		{
			.product_id	= PID,
			.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
			.functions	= usb_functions_default_adb,
		},
		{
			.product_id	= 0xf00e,
			.num_functions	= ARRAY_SIZE(usb_functions_rndis),
			.functions	= usb_functions_rndis,
		},
		{
			.product_id	= 0x9024,
			.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
			.functions	= usb_functions_rndis_adb,
		},
	};

	static struct usb_mass_storage_platform_data mass_storage_pdata = {
		.nluns		= 1,
		.vendor		= "Qualcomm Incorporated",
		.product  	= "Mass storage",
		.release	= 0x0100,
	};

	static struct platform_device usb_mass_storage_device = {
		.name	= "usb_mass_storage",
		.id	= -1,
		.dev	= {
		.platform_data = &mass_storage_pdata,
		},
	};

	static struct usb_ether_platform_data rndis_pdata = {
		/* ethaddr is filled by board_serialno_setup */
		.vendorID   = 0x05C6,
		.vendorDescr    = "Qualcomm Incorporated",
	};

	static struct platform_device rndis_device = {
		.name	= "rndis",
		.id	= -1,
		.dev	= {
		.platform_data = &rndis_pdata,
		},
	};

	static struct android_usb_platform_data android_usb_pdata = {
		.vendor_id	= VID,
		.product_id	= 0x9026,
		.version	= 0x0100,
		.product_name		= PRUD_NAME,
		.manufacturer_name	= MANU_NAME,
		.num_products = ARRAY_SIZE(usb_products),
		.products = usb_products,
		.num_functions = ARRAY_SIZE(usb_functions_all),
		.functions = usb_functions_all,
		.serial_number = "1234567890ABCDEF",
	};

	static struct platform_device android_usb_device = {
		.name	= "android_usb",
		.id		= -1,
		.dev	= {
		.platform_data = &android_usb_pdata,
		},
	};

	static int __init board_serialno_setup(char *serialno)
	{
		int i;
		char *src = serialno;

		/* create a fake MAC address from our serial number.
		 * first byte is 0x02 to signify locally administered.
		 */
		rndis_pdata.ethaddr[0] = 0x02;
		for (i = 0; *src; i++) {
			/* XOR the USB serial across the remaining bytes */
			rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
		}

		android_usb_pdata.serial_number = serialno;
		return 1;
	}
#endif

#ifdef CONFIG_USB_FUNCTION
	static struct usb_function_map usb_functions_map[] = {
	#ifdef CONFIG_USB_ANDROID_DIAG
		{"diag", 0},
	#endif
		{"adb", 1},
	#ifdef CONFIG_USB_F_SERIAL
		{"modem", 2},
		{"nmea", 3},
	#endif
		{"mass_storage", 4},
		{"ethernet", 5},
	#ifdef CONFIG_USB_ANDROID_RMNET
		{"rmnet", 6},
	#endif
	};

	/* dynamic composition */
	static struct usb_composition usb_func_composition[] = {
		{
			.product_id         = 0x9012,
			.functions	    = 0x5, /* 0101 */
		},

		{
			.product_id         = 0x9013,
			.functions	    = 0x15, /* 10101 */
		},

		{
			.product_id         = 0x9014,
			.functions	    = 0x30, /* 110000 */
		},

		{
			.product_id         = 0x9016,
			.functions	    = 0xD, /* 01101 */
		},

		{
			.product_id         = 0x9017,
			.functions	    = 0x1D, /* 11101 */
		},

		{
			.product_id         = 0xF000,
			.functions	    = 0x10, /* 10000 */
		},

		{
			.product_id         = 0xF009,
			.functions	    = 0x20, /* 100000 */
		},

		{
			.product_id         = 0x9018,
			.functions	    = 0x1F, /* 011111 */
		},
	#ifdef CONFIG_USB_FUNCTION_RMNET
		{
			.product_id         = 0x9021,
			/* DIAG + RMNET */
			.functions	    = 0x41,
		},
		{
			.product_id         = 0x9022,
			/* DIAG + ADB + RMNET */
			.functions	    = 0x43,
		},
	#endif
	};

	static struct msm_hsusb_platform_data msm_hsusb_pdata = {
		.version	= 0x0100,
		.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
		.vendor_id          = VID,
		.product_name       = "Qualcomm HSUSB Device",
		.serial_number      = "1234567890ABCDEF",
		.manufacturer_name  = "Qualcomm Incorporated",
		.compositions	= usb_func_composition,
		.num_compositions = ARRAY_SIZE(usb_func_composition),
		.function_map   = usb_functions_map,
		.num_functions	= ARRAY_SIZE(usb_functions_map),
		.config_gpio    = NULL,
	};
#endif

#ifdef CONFIG_USB_EHCI_MSM
	static void msm_hsusb_vbus_power(unsigned phy_info, int on)
	{
		if (on)
			msm_hsusb_vbus_powerup();
		else
			msm_hsusb_vbus_shutdown();
	}

	static struct msm_usb_host_platform_data msm_usb_host_pdata = {
		.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
	};

	static void __init msm7x2x_init_host(void)
	{
		if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa())
			return;

		msm_add_host(0, &msm_usb_host_pdata);
	}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	struct vreg *vreg_3p3;
	static int msm_hsusb_ldo_init(int init)
	{
		   if (init) {
		           vreg_3p3 = vreg_get(NULL, "usb");
		           if (IS_ERR(vreg_3p3))
		                   return PTR_ERR(vreg_3p3);
		           vreg_set_level(vreg_3p3, 3300);
		   } else
		           vreg_put(vreg_3p3);

		   return 0;
	}

	static int msm_hsusb_ldo_enable(int enable)
	{
		   static int ldo_status;

		   if (!vreg_3p3 || IS_ERR(vreg_3p3))
		           return -ENODEV;

		   if (ldo_status == enable)
		           return 0;

		   ldo_status = enable;

		   pr_info("%s: %d", __func__, enable);

		   if (enable)
		           return vreg_enable(vreg_3p3);

		   return vreg_disable(vreg_3p3);
	}

	static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
	{
		   int ret;

		   if (init) {
		           ret = msm_pm_app_rpc_init(callback);
		   } else {
		           msm_pm_app_rpc_deinit(callback);
		           ret = 0;
		   }
		   return ret;
	}
	static struct msm_otg_platform_data msm_otg_pdata = {
		   .pmic_notif_init         = msm_hsusb_pmic_notif_init,
		   .chg_vbus_draw           = hsusb_chg_vbus_draw,
		   .chg_connected           = hsusb_chg_connected,
		   .chg_init                = hsusb_chg_init,
	#ifdef CONFIG_USB_EHCI_MSM
		   .vbus_power = msm_hsusb_vbus_power,
	#endif
		   .ldo_init               = msm_hsusb_ldo_init,
		   .ldo_enable             = msm_hsusb_ldo_enable,
	};

	#ifdef CONFIG_USB_GADGET
		static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;
	#endif
#endif

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(LOOPBACK_HANDSET, 26),
	SND(LOOPBACK_HEADSET, 27),
	SND(LOOPBACK_SPEAKER, 28),
	SND(LINEIN_HANDSET, 29),
	SND(LINEIN_HEADSET, 30),
	SND(LINEIN_SPEAKER, 31),
	SND(HEADSET_AND_SPEAKER, 32),
	SND(VOC_HEADSET, 33),
	SND(VOC_SPEAKER, 34),
	SND(CURRENT, 36),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
	.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	0, 0, 0, 0,

	/* Concurrency 7 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
    DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
    DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
    DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
	.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	 * the only valid choice at this time. The board structure is
	 * set to all zeros by the C runtime initialization and that is now
	 * the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	 * include/linux/android_pmem.h.
	 */
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#define LCDC_CONFIG_PROC          21
#define LCDC_UN_CONFIG_PROC       22
#define LCDC_API_PROG             0x30000066
#define LCDC_API_VERS             0x00010001

#define GPIO_OUT_132    132
#define GPIO_OUT_131    131
#define GPIO_OUT_103    103
#define GPIO_OUT_102    102
#define GPIO_OUT_88     88
#define GPIO_OUT_29     29

static struct resource msm_bl_resources[] = {
	{
		.name	= "ctrl",
		.start	= 29,
		.end	= 29,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "lvs",
		.start	= 20,
		.end	= 20,
		.flags	= IORESOURCE_IO,
	},
};

static struct platform_device msm_bl_device = {
	.name   = "tps61045",
	.num_resources	= ARRAY_SIZE(msm_bl_resources),
	.resource	= msm_bl_resources,
};

static struct resource msm_charge_pump_resources[] = {
	{
		.name	= "ctrl",
		.start	= 29,
		.end	= 29,
		.flags	= IORESOURCE_IO,
	},
};

static struct platform_device msm_charge_pump_device = {
	.name   = "charge_pump",
	.num_resources	= ARRAY_SIZE(msm_charge_pump_resources),
	.resource	= msm_charge_pump_resources,
};

static struct msm_rpc_endpoint *lcdc_ep;

static int msm_fb_lcdc_config(int on)
{
	int rc = 0;
	struct rpc_request_hdr hdr;

	if (on)
		pr_info("lcdc config\n");
	else
		pr_info("lcdc un-config\n");

	lcdc_ep = msm_rpc_connect_compatible(LCDC_API_PROG, LCDC_API_VERS, 0);
	if (IS_ERR(lcdc_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",
			__func__, PTR_ERR(lcdc_ep));
		return -EINVAL;
	}

	rc = msm_rpc_call(lcdc_ep,
				(on) ? LCDC_CONFIG_PROC : LCDC_UN_CONFIG_PROC,
				&hdr, sizeof(hdr),
				5 * HZ);
	if (rc)
		printk(KERN_ERR
			"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	msm_rpc_close(lcdc_ep);
	return rc;
}

static int gpio_array_num[] = {
				GPIO_OUT_132, /* spi_clk */
				GPIO_OUT_131, /* spi_cs  */
				GPIO_OUT_103, /* spi_sdi */
				GPIO_OUT_102, /* spi_sdoi */
				GPIO_OUT_88,
				GPIO_OUT_29,
				};

static void lcdc_gordon_gpio_init(void)
{
	if (gpio_request(GPIO_OUT_132, "spi_clk"))
		pr_err("failed to request gpio spi_clk\n");
	if (gpio_request(GPIO_OUT_131, "spi_cs"))
		pr_err("failed to request gpio spi_cs\n");
	if (gpio_request(GPIO_OUT_103, "spi_sdi"))
		pr_err("failed to request gpio spi_sdi\n");
	if (gpio_request(GPIO_OUT_102, "spi_sdoi"))
		pr_err("failed to request gpio spi_sdoi\n");
	if (gpio_request(GPIO_OUT_88, "gpio_dac"))
		pr_err("failed to request gpio_dac\n");
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_CFG(GPIO_OUT_132, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_OUT_131, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_OUT_103, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_OUT_102, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_OUT_88,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_OUT_29,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n],
			enable ? GPIO_CFG_ENABLE : GPIO_CFG_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void lcdc_gordon_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static char *msm_fb_lcdc_vreg[] = {
	"gp5"
};

static int msm_fb_lcdc_power_save(int on)
{
	struct vreg *vreg[ARRAY_SIZE(msm_fb_lcdc_vreg)];
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(msm_fb_lcdc_vreg); i++) {
		if (on) {
			vreg[i] = vreg_get(0, msm_fb_lcdc_vreg[i]);
			rc = vreg_enable(vreg[i]);
			if (rc) {
				printk(KERN_ERR "vreg_enable: %s vreg"
						"operation failed \n",
						msm_fb_lcdc_vreg[i]);
				goto bail;
			}
			gpio_tlmm_config(GPIO_CFG(GPIO_OUT_103, 0,
						GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		} else {
			int tmp;
			vreg[i] = vreg_get(0, msm_fb_lcdc_vreg[i]);
			tmp = vreg_disable(vreg[i]);
			if (tmp) {
				printk(KERN_ERR "vreg_disable: %s vreg "
						"operation failed \n",
						msm_fb_lcdc_vreg[i]);
				if (!rc)
					rc = tmp;
			}
		}
	}

	return rc;

bail:
	if (on) {
		for (; i > 0; i--)
			vreg_disable(vreg[i - 1]);
	}

	return rc;
}
static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = msm_fb_lcdc_config,
	.lcdc_power_save   = msm_fb_lcdc_power_save,
};

static struct msm_panel_common_pdata lcdc_ili9325sim_panel_data = {
	.panel_config_gpio = lcdc_gordon_config_gpios,
	.gpio_num          = gpio_array_num,
};

static struct platform_device lcdc_ili9325sim_panel_device = {
	.name   = "ili9325sim_qvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_ili9325sim_panel_data,
	}
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;

	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
		if (!strcmp(name, "ili9325sim_qvga"))
			ret = 0;
		else
			ret = -ENODEV;
	}

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
	.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
	static struct resource ram_console_resource[] = {
		{
			.flags  = IORESOURCE_MEM,
		}
	};

	static struct platform_device ram_console_device = {
		.name = "ram_console",
		.id = -1,
		.num_resources  = ARRAY_SIZE(ram_console_resource),
		.resource       = ram_console_resource,
	};
#endif

#ifdef CONFIG_BT
	static struct platform_device msm_bt_power_device = {
		.name = "bt_power",
	};

	enum {
		BT_WAKE,
		BT_RFR,
		BT_CTS,
		BT_RX,
		BT_TX,
		BT_PCM_DOUT,
		BT_PCM_DIN,
		BT_PCM_SYNC,
		BT_PCM_CLK,
		BT_HOST_WAKE,
	};

	static unsigned bt_config_power_on[] = {
		GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
		GPIO_CFG(43, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* RFR */
		GPIO_CFG(44, 2, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* CTS */
		GPIO_CFG(45, 2, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Rx */
		GPIO_CFG(46, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Tx */
		GPIO_CFG(68, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DOUT */
		GPIO_CFG(69, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DIN */
		GPIO_CFG(70, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_SYNC */
		GPIO_CFG(71, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_CLK */
		GPIO_CFG(83, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	};
	static unsigned bt_config_power_off[] = {
		GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */
		GPIO_CFG(43, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* RFR */
		GPIO_CFG(44, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CTS */
		GPIO_CFG(45, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Rx */
		GPIO_CFG(46, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Tx */
		GPIO_CFG(68, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DOUT */
		GPIO_CFG(69, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DIN */
		GPIO_CFG(70, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_SYNC */
		GPIO_CFG(71, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_CLK */
		GPIO_CFG(83, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* HOST_WAKE */
	};

	static int bluetooth_power(int on)
	{
		struct vreg *vreg_bt;
		int pin, rc;

		printk(KERN_DEBUG "%s\n", __func__);

		/* do not have vreg bt defined, gp6 is the same */
		/* vreg_get parameter 1 (struct device *) is ignored */
		vreg_bt = vreg_get(NULL, "gp6");

		if (IS_ERR(vreg_bt)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				   __func__, PTR_ERR(vreg_bt));
			return PTR_ERR(vreg_bt);
		}

		if (on) {
			for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
				rc = gpio_tlmm_config(bt_config_power_on[pin],
							  GPIO_CFG_ENABLE);
				if (rc) {
					printk(KERN_ERR
						   "%s: gpio_tlmm_config(%#x)=%d\n",
						   __func__, bt_config_power_on[pin], rc);
					return -EIO;
				}
			}

			/* units of mV, steps of 50 mV */
			rc = vreg_set_level(vreg_bt, 2600);
			if (rc) {
				printk(KERN_ERR "%s: vreg set level failed (%d)\n",
					   __func__, rc);
				return -EIO;
			}
			rc = vreg_enable(vreg_bt);
			if (rc) {
				printk(KERN_ERR "%s: vreg enable failed (%d)\n",
					   __func__, rc);
				return -EIO;
			}
			msleep(100);

			printk(KERN_ERR "BlueZ required power up * QCOM\r\n");
			gpio_direction_output(94,0);
			gpio_direction_output(20,0);
			msleep(1);
			printk(KERN_ERR "BlueZ required power up * QCOM delay 1ms\r\n");
			printk(KERN_ERR "BlueZ required power up * QCOM delay 100ms\r\n");
			gpio_direction_output(94,1);
			msleep(100);
			gpio_direction_output(20,1);
			msleep(100);

		} else {
			msleep(100);
			rc = vreg_disable(vreg_bt);
			if (rc) {
				printk(KERN_ERR "%s: vreg disable failed (%d)\n",
					   __func__, rc);
				return -EIO;
			}
			msleep(100);
			for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
				rc = gpio_tlmm_config(bt_config_power_off[pin],
							  GPIO_CFG_ENABLE);
				if (rc) {
					printk(KERN_ERR
						   "%s: gpio_tlmm_config(%#x)=%d\n",
						   __func__, bt_config_power_off[pin], rc);
					return -EIO;
				}
			}
			printk(KERN_ERR "BlueZ required power down * QCOM\r\n");
			gpio_direction_output(94,0);
			gpio_direction_output(20,0);
		}
		return 0;
	}

	static void __init bt_power_init(void)
	{
		msm_bt_power_device.dev.platform_data = &bluetooth_power;
	}
#else
	#define bt_power_init(x) do {} while (0)
#endif

int wlan_power(int flag)
{
	struct vreg *vreg_bt;
	int rc;

	printk(KERN_DEBUG "%s\n", __func__);

	/* do not have vreg bt defined, gp6 is the same */
	/* vreg_get parameter 1 (struct device *) is ignored */
	vreg_bt = vreg_get(NULL, "wlan");

	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_bt));
		return PTR_ERR(vreg_bt);
	}
	/* units of mV, steps of 50 mV */
	rc = vreg_set_level(vreg_bt, 2650);
	if (rc) {
		printk(KERN_ERR "%s: vreg set level failed (%d)\n",
				__func__, rc);
		return -EIO;
	}
	if (flag == 1){
		rc = vreg_enable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg enable failed (%d)\n",
					__func__, rc);
			return -EIO;
		}
	}else {
		rc = vreg_disable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg disable failed (%d)\n",
					__func__, rc);
			return -EIO;
		}
	}
	return 0;
}
EXPORT_SYMBOL(wlan_power);

#ifdef CONFIG_ARCH_MSM7X27
static struct resource kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "kgsl_yamato_irq",
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
	},
};

static struct kgsl_platform_data kgsl_pdata;

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(kgsl_resources),
	.resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};
#endif

static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 42,
		.end	= 42,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(83),
		.end	= MSM_GPIO_TO_INT(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static int synaptics_power(int on) {
    static struct vreg *vreg;

    if (!vreg) {
	vreg = vreg_get(NULL, "gp4");
    }
    if (vreg) {
        if (on) {
        	pr_info("synaptics power on\n");
            
        	gpio_tlmm_config(GPIO_CFG(124, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);            
    		gpio_tlmm_config(GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	        gpio_request(30, "touch power");
	        gpio_direction_output(30, 1);
            
    		gpio_tlmm_config(GPIO_CFG(122, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
    		gpio_tlmm_config(GPIO_CFG(123, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	        vreg_enable(vreg);
	        vreg_set_level(vreg, 2150);
        }
        else {
        	pr_info("synaptics power off\n");
            
	        vreg_set_level(vreg, 0);
	        vreg_disable(vreg);            
    		gpio_tlmm_config(GPIO_CFG(122, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
    		gpio_tlmm_config(GPIO_CFG(123, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);

	        gpio_direction_output(30, 0);
    		gpio_tlmm_config(GPIO_CFG(30, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
        	gpio_tlmm_config(GPIO_CFG(124,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);            
        }
    }

    return 0;
}

static struct synaptics_i2c_rmi_platform_data synaptics_ts_data[] = {
    {
        .power = synaptics_power,
    }
};

static int cm3623_power(int on) {
    static struct vreg *vreg;
    if (!vreg) {
        vreg = vreg_get(NULL, "gp1");
        if (vreg) {
        	pr_info("cm3623 power on\n");
            vreg_enable(vreg);
        }
    }

    return 0;
}

static struct cm3623_platform_data cm3623_platform_data = {
    .gpio_int = 18,
    .power = cm3623_power,
};

static struct i2c_board_info i2c_devices[] = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#if defined(CONFIG_SENSORS_MMC31XX)
	#include <linux/mmc31xx.h>
		{
			I2C_BOARD_INFO(MMC31XX_I2C_NAME, MMC31XX_I2C_ADDR),
		},
#endif
#ifdef CONFIG_BMA_ACC
	{
		I2C_BOARD_INFO("bma020", 0x38),
	},
#endif
	{
		I2C_BOARD_INFO(CM3623_NAME, 0),
		.platform_data = &cm3623_platform_data,
	},
};

static struct i2c_board_info gpio_i2c_devices[] = {
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI
	{
		I2C_BOARD_INFO(SYNAPTICS_I2C_RMI_NAME, 0x22),
		.platform_data = synaptics_ts_data,
		.irq = MSM_GPIO_TO_INT(124),
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA
	static uint32_t camera_off_gpio_table[] = {
		/* parallel CAMERA interfaces */
		GPIO_CFG(3,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_PWDN */
		GPIO_CFG(89,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_RESET */
		GPIO_CFG(90,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_POWER/CAM_PWDN_F */
		GPIO_CFG(4,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
		GPIO_CFG(5,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
		GPIO_CFG(6,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
		GPIO_CFG(7,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
		GPIO_CFG(8,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
		GPIO_CFG(9,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
		GPIO_CFG(10, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
		GPIO_CFG(11, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
		GPIO_CFG(12, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
		GPIO_CFG(13, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
		GPIO_CFG(14, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
		GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
	};

	static uint32_t camera_on_gpio_table[] = {
		/* parallel CAMERA interfaces */
		GPIO_CFG(3,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_PWDN */
		GPIO_CFG(89,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_RESET */
		GPIO_CFG(90,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CAM_POWER/CAM_PWDN_F */

		GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
		GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
		GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
		GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
		GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
		GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
		GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
		GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
		GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),/* PCLK */
		GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
		GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
		GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), /* MCLK */
		};

	static void config_gpio_table(uint32_t *table, int len)
	{
		int n, rc;
		for (n = 0; n < len; n++) {
			rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
					__func__, table[n], rc);
				break;
			}
		}
	}

	static struct vreg *vreg_gp2;
	static struct vreg *vreg_gp3;

	static struct msm_camera_sensor_flash_src msm_flash_src = {
		.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
		._fsrc.pmic_src.low_current  = 30,
		._fsrc.pmic_src.high_current = 100,
	};
	#ifdef CONFIG_MT9D112
		static void msm_camera_vreg_config(int vreg_en)
		{
			int rc;

			pr_info("msm_camera_vreg_config_mt9d112:vreg_en[%d]\n",vreg_en);
			gpio_request(3, "mt9d112");
			gpio_request(89, "mt9d112");
			gpio_request(90, "mt9d112");
			gpio_request(15, "mt9d112");
			if (vreg_gp2 == NULL) {
				vreg_gp2 = vreg_get(NULL, "gp2");
				if (IS_ERR(vreg_gp2)) {
					printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
						__func__, "gp2", PTR_ERR(vreg_gp2));
					return;
				}

				rc = vreg_set_level(vreg_gp2, 2600);
				if (rc) {
					printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
						__func__, rc);
				}
			}

			if (vreg_gp3 == NULL) {
				vreg_gp3 = vreg_get(NULL, "gp3");
				if (IS_ERR(vreg_gp3)) {
					printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
						__func__, "gp3", PTR_ERR(vreg_gp3));
					return;
				}

				rc = vreg_set_level(vreg_gp3, 2600);
				if (rc) {
					printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
						__func__, rc);
				}
			}

			if (vreg_en) {
				rc = vreg_enable(vreg_gp2);
				if (rc) {
					printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
						 __func__, rc);
				}

				rc = vreg_enable(vreg_gp3);
				if (rc) {
					printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
						__func__, rc);
				}
				gpio_direction_output(90, 1);
				mdelay(30);
				gpio_direction_output(3, 0);
			} else {
				rc = vreg_disable(vreg_gp2);
				if (rc) {
					printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
						 __func__, rc);
				}

				rc = vreg_disable(vreg_gp3);
				if (rc) {
					printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
						__func__, rc);
				}
				gpio_direction_output(90, 0);
				mdelay(10);
				gpio_direction_output(3, 0);
				gpio_direction_output(89, 0);
				gpio_direction_output(15, 0);
				mdelay(10);
			}
			gpio_free(3);
			gpio_free(89);
			gpio_free(90);
			gpio_free(15);        
		}

		static void config_camera_on_gpios(void)
		{
			int vreg_en = 1;

			msm_camera_vreg_config(vreg_en);
			config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
		}

		static void config_camera_off_gpios(void)
		{
			int vreg_en = 0;

			msm_camera_vreg_config(vreg_en);
			config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
		}

		static struct msm_camera_device_platform_data msm_camera_device_data = {
			.camera_gpio_on  = config_camera_on_gpios,
			.camera_gpio_off = config_camera_off_gpios,
			.ioext.mdcphy = MSM_MDC_PHYS,
			.ioext.mdcsz  = MSM_MDC_SIZE,
			.ioext.appphy = MSM_CLK_CTL_PHYS,
			.ioext.appsz  = MSM_CLK_CTL_SIZE,
		};

		static struct msm_camera_sensor_flash_data flash_mt9d112 = {
			.flash_type = MSM_CAMERA_FLASH_NONE,	// MSM_CAMERA_FLASH_LED
			.flash_src  = &msm_flash_src
		};

		static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
			.sensor_name    = "mt9d112",
			.sensor_reset   = 89,
			.sensor_pwd     = 3,
			.vcm_pwd        = 0,
			.vcm_enable     = 0,
			.pdata          = &msm_camera_device_data,
			.flash_data     = &flash_mt9d112
		};

		static struct platform_device msm_camera_sensor_mt9d112 = {
			.name      = "msm_camera_mt9d112",
			.dev       = {
				.platform_data = &msm_camera_sensor_mt9d112_data,
			},
		};
	#endif
#endif

//Los voltajes y modo de cálculo se delegan en el driver.
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
};

static struct platform_device msm_batt_device = {
	.name 		    	= "msm-battery",
	.id		   			= -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

struct i2c_gpio_platform_data platform_data_gpio_i2c = {
    .sda_pin = 123,
    .scl_pin = 122,
};

struct platform_device msm_device_gpio_i2c = {
	.name	= "i2c-gpio",
	.id		= 10,
	.dev    = {
	.platform_data = &platform_data_gpio_i2c,
    }
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_GPIOLIB
	&msm_gpio_devices[0],
	&msm_gpio_devices[1],
	&msm_gpio_devices[2],
	&msm_gpio_devices[3],
	&msm_gpio_devices[4],
	&msm_gpio_devices[5],
#endif
};

static struct platform_device *devices[] __initdata = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif

	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,

#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
	#ifdef CONFIG_USB_GADGET
		&msm_device_gadget_peripheral,
	#endif
#endif

#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif

#ifdef CONFIG_USB_ANDROID
    &usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
	&android_usb_device,
#endif
	&msm_device_i2c,
	&msm_device_gpio_i2c,
	&msm_device_tssc,
	&android_pmem_kernel_ebi1_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
    &msm_bl_device,	
    &msm_charge_pump_device,
	&msm_fb_device,
	&lcdc_ili9325sim_panel_device,
	&msm_device_uart_dm1,
#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
	&msm_device_pmic_leds,
	&msm_device_snd,
	&msm_device_adspdec,
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
	&msm_bluesleep_device,
#ifdef CONFIG_ARCH_MSM7X27
	&msm_device_kgsl,
#endif
	&hs_device,
	&msm_batt_device,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", 0);
	msm_fb_register_device("lcdc", &lcdc_pdata);
}

extern struct sys_timer msm_timer;

static ssize_t pw28_virtual_keys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	/* Dimensions, 80x60, y starts at 480
	center: x: home: 40, menu: 120, back: 200, search 280, y: 520 */
	return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_HOME)  ":40:520:60:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":120:520:60:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":200:520:60:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":280:520:60:60"
			"\n");
}

static struct kobj_attribute pw28_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &pw28_virtual_keys_show,
};

static struct attribute *pw28_properties_attrs[] = {
	&pw28_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group pw28_properties_attr_group = {
	.attrs = pw28_properties_attrs,
};

static void __init msm7x2x_init_irq(void)
{
	msm_init_irq();
}

static struct msm_acpu_clock_platform_data msm7x2x_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 400000,
	.vdd_switch_time_us = 62,
	.max_axi_khz = 160000,
};

void msm_serial_debug_init(unsigned int base, int irq,
			   struct device *clk_device, int signal_irq);

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT))

static unsigned long vreg_sts, gpio_sts;
static struct vreg *vreg_mmc;

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc2_clk"},
	{GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_cmd"},
	{GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_3"},
	{GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_2"},
	{GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_1"},
	{GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_0"},
};

static struct msm_gpio sdc2_sleep_cfg_data[] = {
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_clk"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_cmd"},
	{GPIO_CFG(64, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_dat_3"},
	{GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_dat_2"},
	{GPIO_CFG(66, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_dat_1"},
	{GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc2_dat_0"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
        {
                .cfg_data = sdc1_cfg_data,
                .size = ARRAY_SIZE(sdc1_cfg_data),
                .sleep_cfg_data = NULL,
        },
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
        {
                .cfg_data = sdc2_cfg_data,
                .size = ARRAY_SIZE(sdc2_cfg_data),
                .sleep_cfg_data = sdc2_sleep_cfg_data,
        },
#endif
};

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
			return;
		}
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;
	unsigned mpp_mmc = 2;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			if (machine_is_msm7x25_ffa() ||
					machine_is_msm7x27_ffa()) {
				rc = mpp_config_digital_out(mpp_mmc,
				     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     MPP_DLOGIC_OUT_CTRL_LOW));
			} else
				rc = vreg_disable(vreg_mmc);
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
			rc = mpp_config_digital_out(mpp_mmc,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));
		} else {
			rc = vreg_set_level(vreg_mmc, 2650);

			if (!rc)
				rc = vreg_enable(vreg_mmc);
		}
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	static struct mmc_platform_data msm7x2x_sdc1_data = {
		.ocr_mask	= MMC_VDD_28_29,
		.translate_vdd	= msm_sdcc_setup_power,
		.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
		.msmsdcc_fmin	= 144000,
		.msmsdcc_fmid	= 24576000,
		.msmsdcc_fmax	= 49152000,
		.nonremovable	= 0,
	#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
		.dummy52_required = 1,
	#endif
	};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	static struct mmc_platform_data msm7x2x_sdc2_data = {
		.ocr_mask	= MMC_VDD_20_21,
		.translate_vdd	= msm_sdcc_setup_power,
		.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
//		.sdiowakeup_irq = MSM_GPIO_TO_INT(66),
	#endif
		.msmsdcc_fmin   = 144000,
		.msmsdcc_fmid   = 24576000,
		.msmsdcc_fmax   = 24576000,
		.nonremovable   = 0,
	#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
		.dummy52_required = 1,
	#endif
	};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static void sdio_wakeup_gpiocfg_slot2(void)
{
       gpio_request(66, "sdio_wakeup");
       gpio_direction_output(66, 1);
       /*
        * MSM GPIO 66 will be used as both SDIO wakeup irq and
        * DATA_1 for slot 2. Hence, leave it to SDCC driver to
        * request this gpio again when it wants to use it as a
        * data line.
        */
       gpio_free(66);
}
#endif

static void __init msm7x2x_init_mmc(void)
{
	vreg_mmc = vreg_get(NULL, "mmc");
	if (!machine_is_msm7x25_ffa() && !machine_is_msm7x27_ffa()) {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_add_sdcc(1, &msm7x2x_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	sdio_wakeup_gpiocfg_slot2();
	/* Register platform device */
	msm_add_sdcc(2, &msm7x2x_sdc2_data);
#endif
}
#else
#define msm7x2x_init_mmc() do {} while (0)
#endif

static struct msm_pm_platform_data msm7x27_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 20000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 20000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 0,
};

static void
msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;
	if (iface) {
		gpio_scl = 95;
		gpio_sda = 96;
	} else {
		gpio_scl = 60;
		gpio_sda = 61;
	}
	if (config_type) {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.rmutex  = 0,
	.pri_clk = 60,
	.pri_dat = 61,
	.aux_clk = 95,
	.aux_dat = 96,
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (gpio_request(60, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(61, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");
	if (gpio_request(95, "i2c_sec_clk"))
		pr_err("failed to request gpio i2c_sec_clk\n");
	if (gpio_request(96, "i2c_sec_dat"))
		pr_err("failed to request gpio i2c_sec_dat\n");

	msm_i2c_pdata.pm_lat = msm7x27_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency;

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static void usb_mpp_init(void)
{
	unsigned rc;
	unsigned mpp_usb = 7;

	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
		rc = mpp_config_digital_out(mpp_usb,
			MPP_CFG(MPP_DLOGIC_LVL_VDD,
				MPP_DLOGIC_OUT_CTRL_HIGH));
		if (rc)
			pr_err("%s: configuring mpp pin"
				"to enable 3.3V LDO failed\n", __func__);
	}
}
#ifdef CONFIG_BOARD_PW28
	char *board_serial;
	static void generate_serial_from_uuid(void)
	{
		unsigned *uuid;
		board_serial = kzalloc(64, GFP_KERNEL);

		uuid = smem_find(ID_SMD_UUID, 4);
		sprintf(board_serial,"ZBR%u",(uuid[2]*uuid[3]));
		#ifdef CONFIG_USB_ANDROID
			board_serialno_setup(board_serial);
		#endif

		/* Ugly hack: Rewrite the command line to include the
		     * serial, since userspace wants it */
		sprintf(boot_command_line,"%s androidboot.serialno=%s",saved_command_line,board_serial);
		saved_command_line = kzalloc(strlen(boot_command_line)+1, GFP_KERNEL);
		strcpy(saved_command_line, boot_command_line);
	}
#endif

void get_sd_boot_mode(unsigned *mode)
{
    unsigned *pMode;
    unsigned int mode_len = 2*sizeof(unsigned);

    if(0 == mode)
    {
        printk(KERN_ERR "[boot] ERROR: 0 == mode\n");
        return;
    }

    pMode = smem_find(ID_SMD_UUID, mode_len);
    if (pMode != 0) {
        printk(KERN_ERR "[boot] Kernel read SMEM_WM_UUID  mode ={<0x%x>, <0x%x>} len <%d>\n", pMode[0], pMode[1], mode_len);
        mode[0] = pMode[0];
        mode[1] = pMode[1];
        
    }else{
        printk(KERN_ERR "[boot] Can't find the msg \n");
    }

}
EXPORT_SYMBOL(get_sd_boot_mode);

static void __init msm7x2x_init(void)
{
	struct kobject *properties_kobj;

	wlan_power(1);
	msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
	generate_serial_from_uuid();

	gpio_tlmm_config(GPIO_CFG(97,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	if (gpio_request(97, "wlan_ctrl") < 0)
		printk ("%s-%d,wlan gpio ctrl request err\n", __FILE__, __LINE__);
	gpio_direction_output(97,0);

	printk(KERN_ERR "bt init gpio\n");
	gpio_tlmm_config(GPIO_CFG(20, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(94, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	if (gpio_request(94, "94_ctrl") < 0)
		printk ("%s-%d,wlan gpio ctrl request err\n", __FILE__, __LINE__);
	if (gpio_request(20, "20_ctrl") < 0)
		printk ("%s-%d,wlan gpio ctrl request err\n", __FILE__, __LINE__);
	gpio_direction_output(94,0);
	gpio_direction_output(20,0);

	msm7x2x_clock_data.max_axi_khz = 200000;

	msm_acpu_clock_init(&msm7x2x_clock_data);

#ifdef CONFIG_ARCH_MSM7X27
	/* Initialize the zero page for barriers and cache ops */
	map_zero_page_strongly_ordered();
	/* This value has been set to 160000 for power savings. */
	/* OEMs may modify the value at their discretion for performance */
	/* The appropriate maximum replacement for 160000 is: */
	/* clk_get_max_axi_khz() */
	kgsl_pdata.high_axi_3d = 160000;

	/* 7x27 doesn't allow graphics clocks to be run asynchronously to */
	/* the AXI bus */
	kgsl_pdata.max_grp2d_freq = 0;
	kgsl_pdata.min_grp2d_freq = 0;
	kgsl_pdata.set_grp2d_async = NULL;
	kgsl_pdata.max_grp3d_freq = 0;
	kgsl_pdata.min_grp3d_freq = 0;
	kgsl_pdata.set_grp3d_async = NULL;
	kgsl_pdata.imem_clk_name = "imem_clk";
	kgsl_pdata.grp3d_clk_name = "grp_clk";
	kgsl_pdata.grp2d_clk_name = NULL;
#endif

	usb_mpp_init();

#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;

	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_otg_pdata.pemp_level = PRE_EMPHASIS_WITH_10_PERCENT;
	msm_otg_pdata.drv_ampl = HS_DRV_AMPLITUDE_5_PERCENT;
	msm_otg_pdata.cdr_autoreset = CDR_AUTO_RESET_DISABLE;
	msm_otg_pdata.phy_reset_sig_inverted = 1;

	#ifdef CONFIG_USB_GADGET
		msm_otg_pdata.swfi_latency =
			msm7x27_pm_data
			[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
		msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
	#endif
#endif

	platform_add_devices(devices, ARRAY_SIZE(devices));
	msm_device_i2c_init();
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	i2c_register_board_info(10, gpio_i2c_devices, ARRAY_SIZE(gpio_i2c_devices));

#ifdef CONFIG_SURF_FFA_GPIO_KEYPAD
	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa())
		platform_device_register(&keypad_device_7k_ffa);
	else
		platform_device_register(&keypad_device_surf);
#endif
	lcdc_gordon_gpio_init();
	msm_fb_add_devices();
#ifdef CONFIG_USB_EHCI_MSM
	msm7x2x_init_host();
#endif
	msm7x2x_init_mmc();

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj) {
		if (sysfs_create_group(properties_kobj,
					&pw28_properties_attr_group))
			pr_err("failed to create board_properties\n");
	} else {
		pr_err("failed to create board_properties\n");
	}

	bt_power_init();

	msm_pm_set_platform_data(msm7x27_pm_data,
							 ARRAY_SIZE(msm7x27_pm_data));
}

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static void __init pmem_mdp_size_setup(char **p)
{
	pmem_mdp_size = memparse(*p, p);
}
__early_param("pmem_mdp_size=", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static void __init msm_msm7x2x_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_mdp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for mdp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_audio_size;
	if (size > 0xE1000) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	} else if (size) {
		android_pmem_audio_pdata.start = MSM_PMEM_AUDIO_START_ADDR;
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes (at %lx physical) for audio "
			"pmem arena\n", size , MSM_PMEM_AUDIO_START_ADDR);
	}

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

#ifdef CONFIG_ANDROID_RAM_CONSOLE
	/* RAM Console can't use alloc_bootmem(), since that zeroes the
	 * region */
	size = 128 * SZ_1K;
	ram_console_resource[0].start = msm_fb_resources[0].end+1;
	ram_console_resource[0].end = ram_console_resource[0].start + size - 1;
	pr_info("allocating %lu bytes at (%lx physical) for ram console\n",
			size, (unsigned long)ram_console_resource[0].start);
	/* We still have to reserve it, though */
	reserve_bootmem(ram_console_resource[0].start,size,0);
#endif

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

#ifdef CONFIG_ARCH_MSM7X27
	size = MSM_GPU_PHYS_SIZE;
	kgsl_resources[1].start = MSM_GPU_PHYS_START_ADDR ;
	kgsl_resources[1].end = kgsl_resources[1].start + size - 1;
	pr_info("allocating %lu bytes (at %lx physical) for KGSL\n",
		size , MSM_GPU_PHYS_START_ADDR);

#endif
}

static void __init msm7x2x_map_io(void)
{
	msm_map_common_io();
	msm_msm7x2x_allocate_memory_regions();

	if (socinfo_init() < 0)
		BUG();

#ifdef CONFIG_CACHE_L2X0
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa()) {
		/* 7x27 has 256KB L2 cache:
			64Kb/Way and 4-Way Associativity;
			evmon/parity/share disabled. */
		if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) > 1)
			|| ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 1)
			&& (SOCINFO_VERSION_MINOR(socinfo_get_version()) >= 3)))
			/* R/W latency: 4 cycles; */
			l2x0_init(MSM_L2CC_BASE, 0x0006801B, 0xfe000000);
		else
			/* R/W latency: 3 cycles; */
			l2x0_init(MSM_L2CC_BASE, 0x00068012, 0xfe000000);
	}
#endif
}

MACHINE_START(MSM7X27_FFA, "PW28 board")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END
