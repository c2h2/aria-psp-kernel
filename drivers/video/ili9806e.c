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
#include <linux/delay.h>
#include <linux/spi/spi.h>

struct ili9806e_data {
	spinlock_t		lock;
	struct spi_device	*spi;
};


static void ili9806e_send_cmd(struct ili9806e_data *drvdata, unsigned short v)
{
	v &= 0xff;
	spi_write(drvdata->spi, (u8 *) &v, 2);
}

static void ili9806e_send_data(struct ili9806e_data *drvdata, unsigned short v)
{
	v &= 0xff;
	v |= 0x100;
	spi_write(drvdata->spi, (u8 *) &v, 2);
}

static void ili9806e_init_lcd(struct ili9806e_data *dd)
{
	struct spi_device *spi = dd->spi;
	
	dev_info(&spi->dev, "Initializing LCD panel\n");

	ili9806e_send_cmd(dd, 0xFF);
	ili9806e_send_data(dd, 0xFF); 
	ili9806e_send_data(dd, 0x98); 
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_data(dd, 0x01); 
 
	ili9806e_send_cmd(dd, 0x08);
	ili9806e_send_data(dd, 0x10); 

	ili9806e_send_cmd(dd, 0x20);
	ili9806e_send_data(dd, 0x00); 	

	ili9806e_send_cmd(dd, 0x21);
	ili9806e_send_data(dd, 0x01); 
 
	ili9806e_send_cmd(dd, 0x30);
	ili9806e_send_data(dd, 0x02); 
 
	ili9806e_send_cmd(dd, 0x31); 
	ili9806e_send_data(dd, 0x02); 
 
	ili9806e_send_cmd(dd, 0x3A);      
	ili9806e_send_data(dd, 0x70);
 
	ili9806e_send_cmd(dd, 0x60);       
	ili9806e_send_data(dd, 0x07); 
 
	ili9806e_send_cmd(dd, 0x61);        
	ili9806e_send_data(dd, 0x06); 
 
	ili9806e_send_cmd(dd, 0x62);      
	ili9806e_send_data(dd, 0x06); 
 
	ili9806e_send_cmd(dd, 0x63);      
	ili9806e_send_data(dd, 0x04); 

	ili9806e_send_cmd(dd, 0x40);
	ili9806e_send_data(dd, 0x18);
 
	ili9806e_send_cmd(dd, 0x41);
	ili9806e_send_data(dd, 0x33);
 
	ili9806e_send_cmd(dd, 0x42);
	ili9806e_send_data(dd, 0x11); 
 
	ili9806e_send_cmd(dd, 0x43);
	ili9806e_send_data(dd, 0x09); 
 
	ili9806e_send_cmd(dd, 0x44);       
	ili9806e_send_data(dd, 0x0C); 

	ili9806e_send_cmd(dd, 0x46);
	ili9806e_send_data(dd, 0x55);
 
	ili9806e_send_cmd(dd, 0x47);  
	ili9806e_send_data(dd, 0x55); 

	ili9806e_send_cmd(dd, 0x45);       
	ili9806e_send_data(dd, 0x14); 
 
	ili9806e_send_cmd(dd, 0x50);
	ili9806e_send_data(dd, 0x50); 
 
	ili9806e_send_cmd(dd, 0x51); 
	ili9806e_send_data(dd, 0x50); 
 
	ili9806e_send_cmd(dd, 0x52);
	ili9806e_send_data(dd, 0x00); 
 
	ili9806e_send_cmd(dd, 0x53);
	ili9806e_send_data(dd, 0x38);

	ili9806e_send_cmd(dd, 0xA0);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0xA1);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xA2);
	ili9806e_send_data(dd, 0x0C); 
	ili9806e_send_cmd(dd, 0xA3);
	ili9806e_send_data(dd, 0x0F); 
	ili9806e_send_cmd(dd, 0xA4);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xA5);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xA6);
	ili9806e_send_data(dd, 0x07); 
	ili9806e_send_cmd(dd, 0xA7);
	ili9806e_send_data(dd, 0x16); 
	ili9806e_send_cmd(dd, 0xA8);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xA9);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xAA);
	ili9806e_send_data(dd, 0x11); 
	ili9806e_send_cmd(dd, 0xAB);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xAC);
	ili9806e_send_data(dd, 0x0E); 
	ili9806e_send_cmd(dd, 0xAD);
	ili9806e_send_data(dd, 0x19); 
	ili9806e_send_cmd(dd, 0xAE);
	ili9806e_send_data(dd, 0x0E); 
	ili9806e_send_cmd(dd, 0xAF);
	ili9806e_send_data(dd, 0x00); 
 
	ili9806e_send_cmd(dd, 0xC0);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0xC1);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xC2);
	ili9806e_send_data(dd, 0x0C); 
	ili9806e_send_cmd(dd, 0xC3);
	ili9806e_send_data(dd, 0x0F); 
	ili9806e_send_cmd(dd, 0xC4);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xC5);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xC6);
	ili9806e_send_data(dd, 0x07); 
	ili9806e_send_cmd(dd, 0xC7);
	ili9806e_send_data(dd, 0x16); 
	ili9806e_send_cmd(dd, 0xC8);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xC9);
	ili9806e_send_data(dd, 0x09); 
	ili9806e_send_cmd(dd, 0xCA);
	ili9806e_send_data(dd, 0x11); 
	ili9806e_send_cmd(dd, 0xCB);
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_cmd(dd, 0xCC);
	ili9806e_send_data(dd, 0x0E); 
	ili9806e_send_cmd(dd, 0xCD);
	ili9806e_send_data(dd, 0x19); 
	ili9806e_send_cmd(dd, 0xCE);
	ili9806e_send_data(dd, 0x0E); 
	ili9806e_send_cmd(dd, 0xCF);
	ili9806e_send_data(dd, 0x00); 
 

	ili9806e_send_cmd(dd, 0xFF);
	ili9806e_send_data(dd, 0xFF); 
	ili9806e_send_data(dd, 0x98); 
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_data(dd, 0x06); 

	ili9806e_send_cmd(dd, 0x00);
	ili9806e_send_data(dd, 0xA0); 
	ili9806e_send_cmd(dd, 0x01);
	ili9806e_send_data(dd, 0x05); 
	ili9806e_send_cmd(dd, 0x02);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x03);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x04);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x05);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x06); 
	ili9806e_send_data(dd, 0x88); 
	ili9806e_send_cmd(dd, 0x07);
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_cmd(dd, 0x08);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x09);
	ili9806e_send_data(dd, 0x90);
	ili9806e_send_cmd(dd, 0x0A);
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_cmd(dd, 0x0B);
	ili9806e_send_data(dd, 0x01);
	ili9806e_send_cmd(dd, 0x0C);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x0D);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x0E);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x0F);
	ili9806e_send_data(dd, 0x00);

	ili9806e_send_cmd(dd, 0x10);
	ili9806e_send_data(dd, 0x55); 
	ili9806e_send_cmd(dd, 0x11);
	ili9806e_send_data(dd, 0x50); 
	ili9806e_send_cmd(dd, 0x12);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x13);
	ili9806e_send_data(dd, 0x85); 
	ili9806e_send_cmd(dd, 0x14);
	ili9806e_send_data(dd, 0x85); 
	ili9806e_send_cmd(dd, 0x15);
	ili9806e_send_data(dd, 0xC0); 
	ili9806e_send_cmd(dd, 0x16);
	ili9806e_send_data(dd, 0x0B); 
	ili9806e_send_cmd(dd, 0x17);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x18);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x19);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x1A);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x1B);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x1C);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x1D);
	ili9806e_send_data(dd, 0x00); 

	ili9806e_send_cmd(dd, 0x20);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x21);
	ili9806e_send_data(dd, 0x23); 
	ili9806e_send_cmd(dd, 0x22);
	ili9806e_send_data(dd, 0x45); 
	ili9806e_send_cmd(dd, 0x23);
	ili9806e_send_data(dd, 0x67); 
	ili9806e_send_cmd(dd, 0x24);
	ili9806e_send_data(dd, 0x01); 
	ili9806e_send_cmd(dd, 0x25);
	ili9806e_send_data(dd, 0x23); 
	ili9806e_send_cmd(dd, 0x26);
	ili9806e_send_data(dd, 0x45); 
	ili9806e_send_cmd(dd, 0x27);
	ili9806e_send_data(dd, 0x67); 

	ili9806e_send_cmd(dd, 0x30);
	ili9806e_send_data(dd, 0x02); 
	ili9806e_send_cmd(dd, 0x31);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x32);
	ili9806e_send_data(dd, 0x11); 
	ili9806e_send_cmd(dd, 0x33);
	ili9806e_send_data(dd, 0xAA); 
	ili9806e_send_cmd(dd, 0x34);
	ili9806e_send_data(dd, 0xBB); 
	ili9806e_send_cmd(dd, 0x35);
	ili9806e_send_data(dd, 0x66); 
	ili9806e_send_cmd(dd, 0x36);
	ili9806e_send_data(dd, 0x00); 
	ili9806e_send_cmd(dd, 0x37);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x38);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x39);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3A);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3B);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3C);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3D);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3E);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x3F);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x40);
	ili9806e_send_data(dd, 0x22); 
	ili9806e_send_cmd(dd, 0x52);
	ili9806e_send_data(dd, 0x12); 
	ili9806e_send_cmd(dd, 0x53);
	ili9806e_send_data(dd, 0x12); 

	ili9806e_send_cmd(dd, 0xFF);
	ili9806e_send_data(dd, 0xFF); 
	ili9806e_send_data(dd, 0x98); 
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_data(dd, 0x07); 
 
	ili9806e_send_cmd(dd, 0x17);
	ili9806e_send_data(dd, 0x32); 

	ili9806e_send_cmd(dd, 0x02);
	ili9806e_send_data(dd, 0x17); 

	ili9806e_send_cmd(dd, 0x18);
	ili9806e_send_data(dd, 0x1D); 

	ili9806e_send_cmd(dd, 0xE1);
	ili9806e_send_data(dd, 0x79); 

	ili9806e_send_cmd(dd, 0xFF);
	ili9806e_send_data(dd, 0xFF); 
	ili9806e_send_data(dd, 0x98); 
	ili9806e_send_data(dd, 0x06); 
	ili9806e_send_data(dd, 0x04); 
	ili9806e_send_data(dd, 0x00); 


	ili9806e_send_cmd(dd, 0x11);

	msleep(120);

	ili9806e_send_cmd(dd, 0x29);

	msleep(25);

	ili9806e_send_cmd(dd, 0x2C);
}

static int ili9806e_probe(struct spi_device *spi)
{
	struct ili9806e_data *drvdata;
	int ret;

	drvdata = devm_kzalloc(&spi->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata) {
		dev_err(&spi->dev, "Error allocating drvdata struct\n");
		return -ENOMEM;
	}

	drvdata->spi = spi;
	spin_lock_init(&drvdata->lock);
	spi_set_drvdata(spi, drvdata);

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 9;
	spi->max_speed_hz = 100000;
	ret = spi_setup(spi);

	if (ret) {
		dev_err(&spi->dev, "Error configuring SPI controller\n");
		return ret;
	}

	ili9806e_init_lcd(drvdata);

	return 0;
}

static int ili9806e_remove(struct spi_device *spi)
{
	struct ili9806e_data *drvdata = spi_get_drvdata(spi);

	spin_lock_irq(&drvdata->lock);
	drvdata->spi = NULL;
	spi_set_drvdata(spi, NULL);

	spin_unlock_irq(&drvdata->lock);

	return 0;
}

static struct spi_driver ili9806e_spi_driver = {
	.driver = {
		.name =		"ili9806e",
		.owner =	THIS_MODULE,
	},
	.probe =	ili9806e_probe,
	.remove =	ili9806e_remove,
};

static int __init ili9806e_init(void)
{
	return spi_register_driver(&ili9806e_spi_driver);
}

static void __exit ili9806e_exit(void)
{
	spi_unregister_driver(&ili9806e_spi_driver);
}

module_init(ili9806e_init);
module_exit(ili9806e_exit);

MODULE_AUTHOR("supercatexpert");
MODULE_DESCRIPTION("ILI9806E MIPI SPI Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ili9806e");


