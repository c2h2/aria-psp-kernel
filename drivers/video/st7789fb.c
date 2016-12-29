/*
 * st7789fb.c - Copyright 2013, evilwombat
 * Based on the HGAFB implementation by Ferenc Bakonyi
 * (fero@drama.obuda.kando.hu) and on the ST7735 framebuffer driver
 * by Kamal Mostafa <kamal@whence.com> et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#define VRAM_LEN	(240*204*2)
#define TXBUF_LEN	(240*204*2*2)	/* SPI transactions require 16 bits */

#define SPI_BLOCK_SIZE 32768

static bool invert_color = false;
module_param(invert_color, bool, 0);
MODULE_PARM_DESC(invert_color, "Invert screen color.");

static struct fb_var_screeninfo st7789_var = {
	.height		= 204,
	.width		= 240,
	.activate	= FB_ACTIVATE_TEST,
	.vmode		= FB_VMODE_NONINTERLACED,
	.xres 		= 240,
	.yres 		= 204,
	.bits_per_pixel = 16,
	.red		= {11, 5},
	.green		= {5, 6},
	.blue		= {0, 5},
	.transp		= {0, 0},
};

static struct fb_fix_screeninfo st7789_fix = {
	.id		= "st7789",
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_PSEUDOCOLOR,
	.accel		= FB_ACCEL_NONE,
	.line_length 	= 240*2,
};

static void st7789fb_schedule_refresh(struct fb_info *info, const struct fb_fillrect *rect);

static int st7789_open(struct fb_info *info, int init)
{
	return 0;
}

static int st7789_release(struct fb_info *info, int init)
{
	return 0;
}

/* setcolreg implementation from 'simplefb', upstream */
static int st7789_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			      u_int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 cr = red >> (16 - info->var.red.length);
	u32 cg = green >> (16 - info->var.green.length);
	u32 cb = blue >> (16 - info->var.blue.length);
	u32 value;

	if (regno >= 16)
		return -EINVAL;

	value = (cr << info->var.red.offset) |
		(cg << info->var.green.offset) |
		(cb << info->var.blue.offset);

	if (info->var.transp.length > 0) {
		u32 mask = (1 << info->var.transp.length) - 1;
		mask <<= info->var.transp.offset;
		value |= mask;
	}

	pal[regno] = value;
	return 0;
}

static int st7789_pan_display(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	st7789fb_schedule_refresh(info, NULL);
	return 0;
}

static int st7789_blank(int blank_mode, struct fb_info *info)
{
	/* TODO: Write to LCD blanking control register */
	return 0;
}

static void st7789_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	sys_fillrect(info, rect);
	st7789fb_schedule_refresh(info, rect);
}

static void st7789_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	sys_copyarea(info, area);
	st7789fb_schedule_refresh(info, NULL);
}

static void st7789_imageblit(struct fb_info *info, const struct fb_image *image)
{
	sys_imageblit(info, image);
	st7789fb_schedule_refresh(info, NULL);
}

static ssize_t st7789_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t ret = fb_sys_write(info, buf, count, ppos);
	st7789fb_schedule_refresh(info, NULL);
	return ret;
}

static int st7789_check_var(struct fb_var_screeninfo *var,
			    struct fb_info *info)
{
	var->bits_per_pixel = 16;
	var->red.offset = 11;
	var->red.length = 5;
	var->red.msb_right = 0;

	var->green.offset = 5;
	var->green.length = 6;
	var->green.msb_right = 0;

	var->blue.offset = 0;
	var->blue.length = 5;
	var->blue.msb_right = 0;

	var->transp.offset = 0;
	var->transp.length = 0;
	var->transp.msb_right = 0;

	var->xres_virtual = 240;
	var->yres_virtual = 204;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->xres = 240;
	var->yres = 204;

	return 0;
}

static struct fb_ops st7789fb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= st7789_open,
	.fb_write	= st7789_write,
	.fb_release	= st7789_release,
	.fb_setcolreg	= st7789_setcolreg,
	.fb_pan_display	= st7789_pan_display,
	.fb_blank	= st7789_blank,
	.fb_fillrect	= st7789_fillrect,
	.fb_copyarea	= st7789_copyarea,
	.fb_imageblit	= st7789_imageblit,
	.fb_check_var	= st7789_check_var,
};

/*
 * rvmalloc implementation from VFB (drivers/video/vfb.c
 */
static void *rvmalloc(unsigned long size)
{
	void *mem;
	unsigned long adr;

	size = PAGE_ALIGN(size);
	mem = vmalloc_32(size);
	if (!mem)
		return NULL;

	memset(mem, 0, size); /* Clear the ram out, no junk to the user */
	adr = (unsigned long) mem;
	while (size > 0) {
		SetPageReserved(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return mem;
}

struct st7789_data {
	spinlock_t		lock;
	struct spi_device	*spi;
	struct fb_info 		*info;
	unsigned char		*vram;
	unsigned short		txbuf[TXBUF_LEN];
};

static void st7789_send_cmd(struct st7789_data *drvdata, unsigned short v)
{
	v &= 0xff;
	spi_write(drvdata->spi, (u8 *) &v, 2);
}

static void st7789_send_data(struct st7789_data *drvdata, unsigned short v)
{
	v &= 0xff;
	v |= 0x100;
	spi_write(drvdata->spi, (u8 *) &v, 2);
}

static void st7789fb_deferred_io(struct fb_info *info,
				 struct list_head *pagelist)
{
	unsigned int i, size, r;
	unsigned char *vram = info->screen_base;
	struct st7789_data *dd = info->par;
	
	/* Convert framebuffer contents into a SPI transmit buffer */

#if 0
	for (i = 0; i < 204*240; i++)
	{
		r = 204*240 - 1 - i;
		dd->txbuf[i*2] = 0x100 | (vram[r*3] & 0xF8) |
			((vram[r*3+1] & 0xE0)>>5);

		dd->txbuf[i*2+1] = 0x100 | 
			((vram[r*3+1] & 0x1C) << 3) |
			((vram[r*3+2] & 0xFF)>>3);
	}
#endif
	for (i = 0; i < 204*240; i++)
	{
		r = 204*240 - 1 - i;
		dd->txbuf[i*2] = 0x100 | vram[r*2+1];

		dd->txbuf[i*2+1] = 0x100 | vram[r*2];
	}	

	st7789_send_cmd(dd, 0x2a);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0xEF);

	st7789_send_cmd(dd, 0x2b);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0xCB);

	st7789_send_cmd(dd, 0x2c);

	for(i=0;i<204*240*2*2;i+=SPI_BLOCK_SIZE)
	{
		if(204*240*2*2 - i >= SPI_BLOCK_SIZE)
		{
			size = SPI_BLOCK_SIZE;
		}
		else
		{
			size = 204*240*2*2 - i;
		}
		spi_write(dd->spi, (u8 *)dd->txbuf+i, size);
	}
}

static struct fb_deferred_io st7789fb_defio = {
	.delay		= HZ / 30,
	.deferred_io	= st7789fb_deferred_io,
};

static void init_lcd(struct st7789_data *dd)
{
	unsigned int i, size;
	struct spi_device *spi = dd->spi;
	
	st7789_send_cmd(dd, 0x11);
	//msleep(120);
	
	dev_info(&spi->dev, "Initializing LCD panel\n");
	if(invert_color)
	{
		st7789_send_cmd(dd, 0x36);
		st7789_send_data(dd,0x00);
	
		st7789_send_cmd(dd, 0x35);
		st7789_send_data(dd,0x00);//40

		//st7789_send_cmd(dd, 0x39);//idle on

		st7789_send_cmd(dd, 0x2a);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0xef);
	
		st7789_send_cmd(dd, 0x2b);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0xef);
	
		st7789_send_cmd(dd, 0x3A);
		st7789_send_data(dd,0x55);

		st7789_send_cmd(dd, 0x55);
		st7789_send_data(dd,0x90);
	
		st7789_send_cmd(dd, 0xb2);
		st7789_send_data(dd,0x46);//7f  20131022
		st7789_send_data(dd,0x4a);//7f
		st7789_send_data(dd,0x01);//01
		st7789_send_data(dd,0xde);//de
		st7789_send_data(dd,0x33);//33
	
		st7789_send_cmd(dd, 0xb3);
		st7789_send_data(dd,0x10);
		st7789_send_data(dd,0x05);
		st7789_send_data(dd,0x0f);
	
		st7789_send_cmd(dd, 0xb4);
		st7789_send_data(dd,0x0b);
	
		st7789_send_cmd(dd, 0xb7);
		st7789_send_data(dd,0x35);
	
		st7789_send_cmd(dd, 0xbb);
		st7789_send_data(dd,0x28);
	
		st7789_send_cmd(dd, 0xbc);
		st7789_send_data(dd,0xec);
	
		st7789_send_cmd(dd, 0xc0);
		st7789_send_data(dd,0x2c);
	
		st7789_send_cmd(dd, 0xc2);
		st7789_send_data(dd,0x01);
	
		st7789_send_cmd(dd, 0xc3);
		st7789_send_data(dd,0x1e);
	
		st7789_send_cmd(dd, 0xc4);
		st7789_send_data(dd,0x20);
	
		st7789_send_cmd(dd, 0xc6);
		st7789_send_data(dd,0x0c);//14 20131022 07
	
		st7789_send_cmd(dd, 0xd0);
		st7789_send_data(dd,0xa4);
		st7789_send_data(dd,0xa1);
	
		st7789_send_cmd(dd, 0xe0);
		st7789_send_data(dd,0xd0);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x08);
		st7789_send_data(dd,0x07);
		st7789_send_data(dd,0x05);
		st7789_send_data(dd,0x29);
		st7789_send_data(dd,0x54);
		st7789_send_data(dd,0x41);
		st7789_send_data(dd,0x3c);
		st7789_send_data(dd,0x17);
		st7789_send_data(dd,0x15);
		st7789_send_data(dd,0x1a);
		st7789_send_data(dd,0x20);
	
		st7789_send_cmd(dd, 0xe1);
		st7789_send_data(dd,0xd0);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x00);
		st7789_send_data(dd,0x08);
		st7789_send_data(dd,0x07);
		st7789_send_data(dd,0x04);
		st7789_send_data(dd,0x29);
		st7789_send_data(dd,0x44);
		st7789_send_data(dd,0x42);
		st7789_send_data(dd,0x3b);
		st7789_send_data(dd,0x16);
		st7789_send_data(dd,0x15);
		st7789_send_data(dd,0x1b);
		st7789_send_data(dd,0x1f);
	}
	else
	{
		st7789_send_cmd(dd, 0x36);     
		st7789_send_data(dd, 0x00);  
		st7789_send_cmd(dd, 0x3a);   
		st7789_send_data(dd, 0x05); 
		st7789_send_cmd(dd, 0x21);   
		                       
		st7789_send_cmd(dd, 0x2a);   
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0xef);  
		st7789_send_cmd(dd, 0x2b);   
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0xCB);  
		                       
		st7789_send_cmd(dd, 0xb2);   
		st7789_send_data(dd, 0x0c);  
		st7789_send_data(dd, 0x0c);  
		st7789_send_data(dd, 0x00);  
		st7789_send_data(dd, 0x33);  
		st7789_send_data(dd, 0x33);  
		st7789_send_cmd(dd, 0xb7);   
		st7789_send_data(dd, 0x35);  
		                       
		st7789_send_cmd(dd, 0xbb);   
		st7789_send_data(dd, 0x1f);  
		st7789_send_cmd(dd, 0xc0);   
		st7789_send_data(dd, 0x2c);  
		st7789_send_cmd(dd, 0xc2);   
		st7789_send_data(dd, 0x01);  
		st7789_send_cmd(dd, 0xc3);   
		st7789_send_data(dd, 0x12);  
		st7789_send_cmd(dd, 0xc4);   
		st7789_send_data(dd, 0x20);  
		st7789_send_cmd(dd, 0xc6);   
		st7789_send_data(dd, 0x0f);  
		st7789_send_cmd(dd, 0xd0);   
		st7789_send_data(dd, 0xa4);  
		st7789_send_data(dd, 0xa1);  
		                       
		st7789_send_cmd(dd, 0xe0);   
		st7789_send_data(dd, 0xd0);  
		st7789_send_data(dd, 0x08);  
		st7789_send_data(dd, 0x11);  
		st7789_send_data(dd, 0x08);  
		st7789_send_data(dd, 0x0c);  
		st7789_send_data(dd, 0x15);  
		st7789_send_data(dd, 0x39);  
		st7789_send_data(dd, 0x33);  
		st7789_send_data(dd, 0x50);  
		st7789_send_data(dd, 0x36);  
		st7789_send_data(dd, 0x13);  
		st7789_send_data(dd, 0x14);  
		st7789_send_data(dd, 0x29);  
		st7789_send_data(dd, 0x2d);  
		st7789_send_cmd(dd, 0xe1);   
		st7789_send_data(dd, 0xd0);  
		st7789_send_data(dd, 0x08);  
		st7789_send_data(dd, 0x10);  
		st7789_send_data(dd, 0x08);  
		st7789_send_data(dd, 0x06);  
		st7789_send_data(dd, 0x06);  
		st7789_send_data(dd, 0x39);  
		st7789_send_data(dd, 0x44);  
		st7789_send_data(dd, 0x51);  
		st7789_send_data(dd, 0x0b);  
		st7789_send_data(dd, 0x16);  
		st7789_send_data(dd, 0x14);  
		st7789_send_data(dd, 0x2f);  
		st7789_send_data(dd, 0x31);  
}	
	//Delayms(120);
	st7789_send_cmd(dd, 0x29);   //Display ON 
	//Delayms(120);  

	for (i = 0; i < 240*204; i++)
	{
		dd->txbuf[i*2] = 0x100;
		dd->txbuf[i*2+1] = 0x100;
	}

	st7789_send_cmd(dd, 0x2a);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0xEF);

	st7789_send_cmd(dd, 0x2b);
	st7789_send_data(dd, 0);
	st7789_send_data(dd, 0x0);
	st7789_send_data(dd, 0x1);
	st7789_send_data(dd, 0x3F);

	st7789_send_cmd(dd, 0x2c);

	for(i=0;i<240*204*2*2;i+=16384)
	{
		if(240*204*2*2 - i >= 16384)
		{
			size = 16384;
		}
		else
		{
			size = 240*204*2*2 - i;
		}
		spi_write(dd->spi, (u8 *) dd->txbuf+ i , size);
	}
}

static int st7789_probe(struct spi_device *spi)
{
	struct st7789_data *drvdata;
	int ret;

	drvdata = devm_kzalloc(&spi->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata) {
		dev_err(&spi->dev, "Error allocating drvdata struct\n");
		return -ENOMEM;
	}

	drvdata->vram = rvmalloc(VRAM_LEN);
	if (!drvdata->vram) {
		dev_err(&spi->dev, "Error allocating video memory struct\n");
		return -ENOMEM;
	}

	drvdata->spi = spi;
	spin_lock_init(&drvdata->lock);
	spi_set_drvdata(spi, drvdata);

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 9;
	spi->max_speed_hz = 50000000;
	ret = spi_setup(spi);

	if (ret) {
		dev_err(&spi->dev, "Error configuring SPI controller\n");
		return ret;
	}

	init_lcd(drvdata);

	drvdata->info = framebuffer_alloc(sizeof(u32) * 16, &spi->dev);
	if (!drvdata->info) {
		return -ENOMEM;
	}

	st7789_fix.smem_start = (unsigned long)drvdata->vram;
	st7789_fix.smem_len = VRAM_LEN;

	drvdata->info->par = drvdata;
	drvdata->info->flags = FBINFO_DEFAULT | FBINFO_VIRTFB;
	drvdata->info->var = st7789_var;
	drvdata->info->fix = st7789_fix;
	drvdata->info->fbops = &st7789fb_ops;
	drvdata->info->screen_base = drvdata->vram;
	drvdata->info->fbdefio = &st7789fb_defio;
	fb_deferred_io_init(drvdata->info);
	drvdata->info->pseudo_palette = (void *)(drvdata->info + 1);

	ret = register_framebuffer(drvdata->info);

	if (ret < 0) {
		framebuffer_release(drvdata->info);
		return -EINVAL;
	}
	
        printk(KERN_INFO "fb%d: %s frame buffer device\n",
	       drvdata->info->node, drvdata->info->fix.id);

	return 0;
}

static void st7789fb_schedule_refresh(struct fb_info *info,
	const struct fb_fillrect *rect)
{
	if (!info->fbdefio)
		return;

	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
}

static int st7789_remove(struct spi_device *spi)
{
	struct st7789_data *drvdata = spi_get_drvdata(spi);

	spin_lock_irq(&drvdata->lock);
	drvdata->spi = NULL;
	spi_set_drvdata(spi, NULL);

	if (drvdata->info) {
		unregister_framebuffer(drvdata->info);
		fb_deferred_io_cleanup(drvdata->info);
		framebuffer_release(drvdata->info);
	}
	spin_unlock_irq(&drvdata->lock);
	return 0;
}

static struct spi_driver st7789_spi_driver = {
	.driver = {
		.name =		"st7789fb",
		.owner =	THIS_MODULE,
	},
	.probe =	st7789_probe,
	.remove =	st7789_remove,
};

static int __init st7789fb_init(void)
{
	return spi_register_driver(&st7789_spi_driver);
}

static void __exit st7789fb_exit(void)
{
	spi_unregister_driver(&st7789_spi_driver);
}

module_init(st7789fb_init);
module_exit(st7789fb_exit);

MODULE_AUTHOR("evilwombat");
MODULE_DESCRIPTION("Sitronix ST7789 LCD Controller Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:st7789fb");


