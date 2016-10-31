#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "ssd2828.h"

#define LCM_DEBUG printk

#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#define Bridge_IC_ID      0x2828

#define UDELAY(n)   udelay(n)
#define MDELAY(n)   mdelay(n)

#define SET_RESET_PIN(v)    gpio_direction_output(GPIO_TO_PIN(1, 19), v)

#define SET_LSCE_LOW   gpio_direction_output(GPIO_TO_PIN(0, 12), 0)
#define SET_LSCE_HIGH  gpio_direction_output(GPIO_TO_PIN(0, 12), 1)

#define SET_LSCK_LOW   gpio_direction_output(GPIO_TO_PIN(3, 14), 0) /* SCLK */
#define SET_LSCK_HIGH  gpio_direction_output(GPIO_TO_PIN(3, 14), 1)

#define SET_LSDA_LOW   gpio_direction_output(GPIO_TO_PIN(3, 15), 0) /* DI */
#define SET_LSDA_HIGH  gpio_direction_output(GPIO_TO_PIN(3, 15), 1)

#define GET_HX_SDI     gpio_get_value(GPIO_TO_PIN(3, 16)) /* D0 */


#define HX_WR_COM       (0x70)
#define HX_WR_REGISTER  (0x72)
#define HX_RD_REGISTER  (0x73)

/* 8" show config */
#define LCD_XSIZE_TFT	(800) 
#define LCD_YSIZE_TFT	(1280)
#define HACT	LCD_XSIZE_TFT
#define VACT	LCD_YSIZE_TFT
#define LCD_VBPD	(20)
#define LCD_VFPD	(20)
#define LCD_VSPW	(2)
#define LCD_HBPD	(42)
#define LCD_HFPD	(44)
#define LCD_HSPW	(2)


#define BIT0                (0x0001)
#define BIT1                (0x0002)
#define BIT2                (0x0004)
#define BIT3                (0x0008)
#define BIT4                (0x0010)
#define BIT5                (0x0020)
#define BIT6                (0x0040)
#define BIT7                (0x0080)
#define BIT8                (0x0100)
#define BIT9                (0x0200)
#define BITA                (0x0400)
#define BITB                (0x0800)
#define BITC                (0x1000)
#define BITD                (0x2000)
#define BITE                (0x4000)
#define BITF                (0x8000)

#define PLL_MTY		23
#define GET_LCD_180	1

#define RGB_TYPE	1

static __inline void spi_send_data(unsigned int data)
{

	unsigned int i;
	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
	UDELAY(10);
	SET_LSCE_LOW;
	UDELAY(10);
	for (i = 0; i < 24; ++i)
	{
		if(data & (1 << 23))
		{
			SET_LSDA_HIGH;
		}
		else
		{
			SET_LSDA_LOW;
		}
		data <<= 1;
		UDELAY(10);
		SET_LSCK_LOW;
		UDELAY(10);
		SET_LSCK_HIGH;
		UDELAY(10);
	}
	SET_LSDA_HIGH;
	SET_LSCE_HIGH;
}


static __inline void Write_com(unsigned int cmd)
{
	unsigned int out = ((HX_WR_COM<<16) | (cmd & 0xFFFF));
	spi_send_data(out);
}

static __inline void Write_register(unsigned int data)
{
	unsigned int out = ((HX_WR_REGISTER<<16) |(data & 0xFFFF));
	spi_send_data(out);
}

static __inline unsigned short Read_register(void)
{
	unsigned char i,j,front_data;
	unsigned short value = 0;
	front_data=HX_RD_REGISTER;
	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
	UDELAY(10);
	SET_LSCE_LOW;
	UDELAY(10); 
	for(i=0;i<8;i++)
	{
		if(front_data & 0x80)
			SET_LSDA_HIGH;
		else
			SET_LSDA_LOW;
		front_data<<= 1;
		UDELAY(10);
		SET_LSCK_LOW;
		UDELAY(10); 
		SET_LSCK_HIGH;
		UDELAY(10);       
	}
	MDELAY(1); 
	for(j=0;j<16;j++)
	{
		SET_LSCK_HIGH;
		UDELAY(10);
		SET_LSCK_LOW;
		value <<= 1;
		if (GET_HX_SDI)
			value |= 1;

		UDELAY(10); 
	}
	SET_LSCE_HIGH;
	return value;      
}

static __inline unsigned short Read_8bit(void)
{
	unsigned char i,j,front_data;
	unsigned short value = 0;
	front_data=HX_RD_REGISTER;
	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
	UDELAY(10);
	SET_LSCE_LOW;
	UDELAY(10); 
	for(i=0;i<8;i++)
	{
		if(front_data& 0x80)
			SET_LSDA_HIGH;
		else
			SET_LSDA_LOW;
		front_data<<= 1;
		UDELAY(10);
		SET_LSCK_LOW;
		UDELAY(10); 
		SET_LSCK_HIGH;
		UDELAY(10);       
	}
	MDELAY(1); 
	for(j=0;j<8;j++)
	{
		SET_LSCK_HIGH;
		UDELAY(10);
		SET_LSCK_LOW;
		value<<= 1;
		if (GET_HX_SDI)
			value |= 1;

		UDELAY(10); 
	}
	SET_LSCE_HIGH;
	return value;      
}

void SPI_2825_WrReg(unsigned char c,unsigned short value)
{
	Write_com(c);
	Write_register(value);
}

void SPI_2825_WrCmd(unsigned char cmd)
{
	Write_com(cmd);
}

unsigned short get_lcd_driver_id(void)
{
	unsigned short reg_mipi_buffer[2] = {0,0};
	unsigned long i = 0, loop_en = 1;
	unsigned short r_data;
	SPI_2825_WrReg(0xbc, 0x0002);
	SPI_2825_WrReg(0xBF, 0x0028);
	while(loop_en)
	{
		SPI_2825_WrReg(0xbc, 0x0002);
		SPI_2825_WrReg(0xBF, 0x00B0);
		SPI_2825_WrReg(0xb7, 0x0382);
		SPI_2825_WrReg(0xbb, 0x0006);
		SPI_2825_WrReg(0xC1, 0x000a);
		SPI_2825_WrReg(0xc0, 0x0001);
		SPI_2825_WrReg(0xbc, 0x0002);

		SPI_2825_WrReg(0xbf, 0xda + i);
		Write_com(0xc6);
		r_data = Read_register();
		if(r_data & 1)
		{
			Write_com(0xff);
			reg_mipi_buffer[i] = Read_8bit();
			if(++i >= 2)
			{
				loop_en = 0;
			}
		}
	}
	return ((reg_mipi_buffer[0] << 8) | reg_mipi_buffer[1]);
}

unsigned short get_lcd_driver_id_new(unsigned short driver_id)
{
	unsigned short reg_mipi_buffer[2] = {0,0};
    	unsigned long i = 0, loop_en = 1, ret;
	unsigned short r_data;
	SPI_2825_WrReg(0xc0, 0x0001);
	SPI_2825_WrReg(0xbc, 0x0002);
	SPI_2825_WrReg(0xBF, 0x0028);
	while(loop_en)
	{
		switch(driver_id)
		{
			default:
			case 0x8260:
			{
				SPI_2825_WrReg(0xbc, 0x0002);
				SPI_2825_WrReg(0xBF, 0x00B0);
				SPI_2825_WrReg(0xb7, 0x0382);
				SPI_2825_WrReg(0xbb, 0x0006);
				SPI_2825_WrReg(0xC1, 0x000a);
				SPI_2825_WrReg(0xc0, 0x0001);
				SPI_2825_WrReg(0xbc, 0x0002);
				SPI_2825_WrReg(0xbf, 0x00fc + i);
				break;
			}
			case 0x8394:
			{
				SPI_2825_WrReg(0xb7, 0x0382);
				SPI_2825_WrReg(0xbb, 0x0006);
				SPI_2825_WrReg(0xC1, 0x000a);
				SPI_2825_WrReg(0xc0, 0x0001);
				SPI_2825_WrReg(0xbc, 0x0002);
				SPI_2825_WrReg(0xbf, 0x00da + i);
				break;
			}
		}
        	Write_com(0xc6);

		r_data = Read_register();
		if(r_data & 1)
		{
				Write_com(0xff);
				reg_mipi_buffer[i] = Read_register();
				if(++i >= 2)
				{
					loop_en = 0;
				}
		}
    	}
	ret = (((reg_mipi_buffer[0] & 0xff) << 8) +
		(reg_mipi_buffer[1] & 0xff));
	return ret;
}

void init_hir080(void)
{
	LCM_DEBUG("[LCM0x8260]: init_lcm_registers. \n");

	SPI_2825_WrReg(0xc0, 0x0001);

	SPI_2825_WrReg(0xb1, ((LCD_VSPW << 8) + LCD_HSPW));
	SPI_2825_WrReg(0xb2, (((LCD_VBPD) << 8) + (LCD_HBPD)));
	SPI_2825_WrReg(0xb3, ((LCD_VFPD << 8) + LCD_HFPD));
	SPI_2825_WrReg(0xb4, HACT);
	SPI_2825_WrReg(0xb5, VACT);

	SPI_2825_WrReg(0xb6, BITF | BITE | BITD | 0x1b);

	SPI_2825_WrReg(0xde, 0x0003);

	SPI_2825_WrReg(0xc9, 0x1408); 
	SPI_2825_WrReg(0xca, 0x8804);
	SPI_2825_WrReg(0xcb, 0x05aa);

	SPI_2825_WrReg(0xba, 0x8440);
	SPI_2825_WrReg(0xbb, 0x0006);

	SPI_2825_WrReg(0xb9, 0x0001);
	SPI_2825_WrReg(0xb7, 0x0550);
	SPI_2825_WrReg(0xb8, 0x0000);

	mdelay(50);

	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF); 
	Write_register(((unsigned short)(BIT6 | BIT0 | BIT1 | BIT0 | BIT0)
		<< 8) | 0xB2);

	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x38B3);

	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x73B7);

	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0xABBA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0xE8BB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0xFFBD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x3FBE);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x50C3);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x14C4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0xB9C5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x14C6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x15C7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00CA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x3FCB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x34CC);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x2CCD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x2ACE);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x17CF);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0dD0);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0CD1);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11D2);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11D3);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11D4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0AD5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x3FD6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x34D7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x2CD8);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x2AD9);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x17DA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0dDB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0CDC);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11DD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11DE);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11DF);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0AE0);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x52E5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x02B0);

	/* MUXL Control register(page2 B1h) */
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x25B1);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00B2);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11B3);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x12B4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x09B5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0bB6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x05B7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x07B8);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x01B9);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BC);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BE);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00BF);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x03C0);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C1);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C2);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C3);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x25C7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00C8);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x11C9);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x12CA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0ACB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0CCC);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x06CD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x08CE);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x02CF);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D0);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D1);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D2);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D3);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D5);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x04D6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D8);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00D9);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00DA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00DB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00DC);

	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x40DD);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x03B0);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0BC4);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x07C6);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x01C7);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x80CA);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x0ACB);
	
	SPI_2825_WrReg(0xbc, 0x0002);
	Write_com(0xBF);
	Write_register(0x00CC);

	MDELAY(10);

	SPI_2825_WrReg(0xde, 0x0003);
	SPI_2825_WrReg(0xd6, 0x0005);
	SPI_2825_WrReg(0xb9, 0x0000);
	SPI_2825_WrReg(0xc4, 0x0001);

	SPI_2825_WrReg(0xba, (BITF+BITE) | PLL_MTY);

	SPI_2825_WrReg(0xbb, 0x0006);
	SPI_2825_WrReg(0xb9, 0x0001);
	SPI_2825_WrReg(0xb8, 0x0000);

	SPI_2825_WrReg(0xb7, 0x030B);

	Write_com(0x00bc);
	Write_register(2);
	Write_com(0xBF);
	Write_register(0x0011);
	MDELAY(120);

	Write_com(0x00bc);
	Write_register(2);
	Write_com(0xBF);
	Write_register(0x0029);
}

void init_HY_CCG6169(void)
{
	SPI_2825_WrReg(0xc0, 0x0001);

#if(RGB_TYPE == 0)
	Write_com(0xb7); 
	Write_register(BIT6); 
	Write_com(0xc1);
	Write_register(0x0004);
	Write_com(0x00de);

	Write_register(0x0000); 

	Write_com(0x00d6);
	Write_register(0x0004);

	Write_com(0x00b9);
	Write_register(0x0000);

	Write_com(0x00c4);
	Write_register(0x0001);

	Write_com(0x00ba);
	Write_register(0x8440);

	Write_com(0x00bb);
	Write_register(0x0006);

	Write_com(0x00b9);
	Write_register(0x0001);

	MDELAY(10);
	Write_com(0x00c9);

	Write_register(0x2302);
	Write_com(0x00b8);
	Write_register(0x0000);

	Write_com(0x00ca);

	Write_register(0x2302);

	Write_com(0x00cb); 

	Write_register(0x05aa);



	MDELAY(10);

	Write_com(0xb7);
	Write_register(BIT6);

	Write_com(0x00bc);
	Write_register(0x0004);

	Write_com(0xBF);

	Write_register(0xffb9);
	Write_register(0x9483);


	Write_com(0x00bc);
	Write_register(16);
	Write_com(0xBF);
	Write_register(0x64b1);
	Write_register(0x3010);
	Write_register(0x3443);
	Write_register(0xf111);
	Write_register(0x7081);
	Write_register(0x34d9);
	Write_register(0xc080);
	Write_register(0x42d2);

	Write_com(0x00bc);
	Write_register(13);

	Write_com(0xBF);

	Write_register(0x45b2);
	Write_register(0x0f64);
	Write_register(0x4009);
	Write_register(0x081c);
	Write_register(0x1c08);
	Write_register(0x004d);
	Write_register(0x00);

	Write_com(0x00bc);
	Write_register(23);

	Write_com(0xBF);

	Write_register(0x07b4);
	Write_register(0x076e);

	Write_register(0x6f71);
	Write_register(0x0070);
	Write_register(0x0100);
	Write_register(0x0f6e);
	Write_register(0x076e);
	Write_register(0x6f71);
	Write_register(0x0070);
	Write_register(0x0100);
	Write_register(0x0f6e);
	Write_register(0x006e);

	Write_com(0x00bc);
	Write_register(3);

	Write_com(0xBF);

	Write_register(0x6fb6);
	Write_register(0x6f);


	Write_com(0x00bc);
	Write_register(2);

	Write_com(0xBF);

	Write_register(0x09cc);

	Write_com(0x00bc);
	Write_register(33);


	Write_com(0xBF);

	Write_register(0x00d3);

	Write_register(0x0008);
	Write_register(0x0701);
	Write_register(0x0800);
	Write_register(0x1032);
	Write_register(0x000a);
	Write_register(0x0005);
	Write_register(0x0a20);
	Write_register(0x0905);
	Write_register(0x3200);
	Write_register(0x0810);
	Write_register(0x3500);
	Write_register(0x0d33);
	Write_register(0x4707);
	Write_register(0x070d);
	Write_register(0x0f47);
	Write_register(0x08);

	Write_com(0x00bc);
	Write_register(45);

	Write_com(0xBF);

	Write_register(0x03d5);
	Write_register(0x0302);
	Write_register(0x0102);
	Write_register(0x0100);
	Write_register(0x0700);
	Write_register(0x0706);
	Write_register(0x0506);
	Write_register(0x0504);
	Write_register(0x2104);
	Write_register(0x1820);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x2318);
	Write_register(0x1822);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x18);

	Write_com(0x00bc);
	Write_register(43);

	Write_com(0xBF);

	Write_register(0x03e0);
	Write_register(0x1c17);
	Write_register(0x302d);
	Write_register(0x273b);
	Write_register(0x0840);
	Write_register(0x0d0b);
	Write_register(0x0f18);
	Write_register(0x1512);
	Write_register(0x1413);
	Write_register(0x1207);
	Write_register(0x1714);
	Write_register(0x1703);
	Write_register(0x2d1c);
	Write_register(0x3b30);
	Write_register(0x4027);
	Write_register(0x0b08);
	Write_register(0x180d);
	Write_register(0x120f);
	Write_register(0x1315);
	Write_register(0x0714);
	Write_register(0x1412);
	Write_register(0x17);

	Write_com(0x00bc);
	Write_register(6);

	Write_com(0xBF);

	Write_register(0x1fc9);

	Write_register(0x1e2e);

	Write_register(0x101e);



	Write_com(0x00b1); 
	Write_register((LCD_VSPW<<8)|LCD_HSPW);

	Write_com(0x00b2);
	Write_register((LCD_VBPD<<8)|LCD_HBPD);

	Write_com(0x00b3);
	Write_register((LCD_VFPD<<8)|LCD_HFPD);

	Write_com(0x00b4);
	Write_register(HACT);

	Write_com(0x00b5);
	Write_register(VACT);

	Write_com(0x00b6); 
	
	Write_register(BITF + BITE + BITD | 0x1b);
	

	Write_com(0x00de);
	Write_register(0x0003);

	Write_com(0x00d6);
	Write_register(0x0005);

	Write_com(0x00b9);
	Write_register(0x0000);

	Write_com(0x00c4);
	Write_register(0x0001);

	Write_com(0x00ba);

	Write_register(BITF + BITE | 20);


	Write_com(0x00bb);
	Write_register(0x0006);

	Write_com(0x00b9);
	Write_register(0x0001);

	MDELAY(200);
	Write_com(0x00b8);
	Write_register(0x0000);

	Write_com(0xb7);
	
	Write_register(0x030B | BIT5);

	Write_com(0x00bc);
	Write_register(2);

	Write_com(0xBF);

	Write_register(0x0011);

	MDELAY(120);

	Write_com(0x00bc);
	Write_register(2);
	Write_com(0xBF);
	Write_register(0x0029);

	MDELAY(50);
#else
	LCM_DEBUG("[LCM0x8394]: init_lcm_registers. \n");
	Write_com(0x00de);
	Write_register(0x0003); 
	Write_com(0x00d6);
	Write_register(0x0004); 
	Write_com(0x00b9);
	Write_register(0x0000); 
	Write_com(0x00c4);
	Write_register(0x0001); 
	Write_com(0x00ba);
	Write_register(0x8440); 
	Write_com(0x00bb);
	Write_register(0x0006); 
	Write_com(0x00b9);
	Write_register(0x0001); 

	MDELAY(10);
	Write_com(0x00c9);
	Write_register(0x2302); 
	Write_com(0x00b8);
	Write_register(0x0000); 

	MDELAY(10);

	Write_com(0xb7);
	Write_register(0x0550);
	Write_com(0x00bc);
	Write_register(0x0004); 
	Write_com(0xBF);
	Write_register(0xffb9);
	Write_register(0x9483);

	Write_com(0x00bc);
	Write_register(16); 
	Write_com(0xBF);
	Write_register(0x64b1);
	Write_register(0x3010);
	Write_register(0x3443);
	Write_register(0xf111);
	Write_register(0x7081);;
	Write_register(0x34d9);
	Write_register(0xc080);
	Write_register(0x42d2);

	Write_com(0x00bc);
	Write_register(13); 
	Write_com(0xBF);
	Write_register(0x45b2);
	Write_register(0x0f64);
	Write_register(0x4009);
	Write_register(0x081c);
	Write_register(0x1c08);
	Write_register(0x004d);
	Write_register(0x00);

	Write_com(0x00bc);
	Write_register(23); 
	Write_com(0xBF);
	Write_register(0x07b4);
	Write_register(0x076e);
	Write_register(0x6f71);
	Write_register(0x0070);
	Write_register(0x0100);
	Write_register(0x0f6e);
	Write_register(0x076e);
	Write_register(0x6f71);
	Write_register(0x0070);
	Write_register(0x0100);
	Write_register(0x0f6e);
	Write_register(0x006e);

	Write_com(0x00bc);
	Write_register(3); 
	Write_com(0xBF);
	Write_register(0x6fb6);
	Write_register(0x6f);

	Write_com(0x00bc);
	Write_register(2); 
	Write_com(0xBF);
	Write_register(0x09cc);

	Write_com(0x00bc);
	Write_register(33); 
	Write_com(0xBF);
	Write_register(0x00d3);
	Write_register(0x0008);
	Write_register(0x0701);
	Write_register(0x0800);
	Write_register(0x1032);
	Write_register(0x000a);
	Write_register(0x0005);
	Write_register(0x0a20);
	Write_register(0x0905);
	Write_register(0x3200);
	Write_register(0x0810);
	Write_register(0x3500);
	Write_register(0x0d33);
	Write_register(0x4707);
	Write_register(0x070d);
	Write_register(0x0f47);
	Write_register(0x08);

	Write_com(0x00bc);
	Write_register(45); 
	Write_com(0xBF);
	Write_register(0x03d5);
	Write_register(0x0302);
	Write_register(0x0102);
	Write_register(0x0100);
	Write_register(0x0700);
	Write_register(0x0706);
	Write_register(0x0506);
	Write_register(0x0504);
	Write_register(0x2104);
	Write_register(0x1820);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x2318);
	Write_register(0x1822);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x1818);
	Write_register(0x18);

	Write_com(0x00bc);
	Write_register(43); 
	Write_com(0xBF);
	Write_register(0x03e0);
	Write_register(0x1c17);
	Write_register(0x302d);
	Write_register(0x273b);
	Write_register(0x0840);
	Write_register(0x0d0b);
	Write_register(0x0f18);
	Write_register(0x1512);
	Write_register(0x1413);
	Write_register(0x1207);
	Write_register(0x1714);
	Write_register(0x1703);
	Write_register(0x2d1c);
	Write_register(0x3b30);
	Write_register(0x4027);
	Write_register(0x0b08);
	Write_register(0x180d);
	Write_register(0x120f);
	Write_register(0x1315);
	Write_register(0x0714);
	Write_register(0x1412);
	Write_register(0x17);

	Write_com(0x00bc);
	Write_register(6); 
	Write_com(0xBF);
	Write_register(0x1fc9);
	Write_register(0x1e2e);
	Write_register(0x101e);

	Write_com(0x00b1);
	Write_register((LCD_VSPW<<8)|LCD_HSPW);
	Write_com(0x00b2);
	Write_register((LCD_VBPD<<8)|LCD_HBPD);
	Write_com(0x00b3);
	Write_register((LCD_VFPD<<8)|LCD_HFPD);
	Write_com(0x00b4);
	Write_register(LCD_XSIZE_TFT);
	Write_com(0x00b5);
	Write_register(LCD_YSIZE_TFT);

	Write_com(0x00b6);
	Write_register(0xC01B);				
	Write_com(0x00de);
	Write_register(0x0003);
	Write_com(0x00d6);
	Write_register(0x0005);
	Write_com(0x00b9);
	Write_register(0x0000);
	Write_com(0x00c4);
	Write_register(0x0001);
	Write_com(0x00ba);
	Write_register(0x8020);
	Write_com(0x00bb);
	Write_register(0x0006);

	Write_com(0x00b9);
	Write_register(0x0001);

	MDELAY(200);
	Write_com(0x00b8);
	Write_register(0x0000);

	Write_com(0x00b7);
	Write_register(0x030B | 0x0020);

	Write_com(0x00bc);
	Write_register(1); 
	Write_com(0xBF);
	Write_register(0x0011);      
	MDELAY(200); 

	Write_com(0x00bc);
	Write_register(1); 
	Write_com(0xBF);
	Write_register(0x0029);
	MDELAY(50);
#endif
}

static void init_lcm_registers(void)
{
	unsigned short lcd_driver_id = 0;
	LCM_DEBUG("[LCM************]: init_lcm_registers.%d \n", 0);

	SPI_2825_WrReg(0xb6, BITF | BITE | BITD | 0x1b);
	SPI_2825_WrReg(0xde, 0x0003);

	SPI_2825_WrReg(0xba, 0x8440);
	SPI_2825_WrReg(0xbb, 0x0006);
	SPI_2825_WrReg(0xb9, 0x0001);
	SPI_2825_WrReg(0xb7, 0x0550);
	SPI_2825_WrReg(0xb8, 0x0000);

	MDELAY(50);

	lcd_driver_id = get_lcd_driver_id_new(0x8260);
	LCM_DEBUG("[LCM Driver ID = 0x%04X]\n", lcd_driver_id);

	if(lcd_driver_id == 0x8260)
	{
		init_hir080();
	}
	else
	{
		lcd_driver_id = get_lcd_driver_id_new(0x8394);
		LCM_DEBUG("[LCM Driver ID = 0x%04X]\n", lcd_driver_id);
		if(lcd_driver_id == 0x8394)
		init_HY_CCG6169();
	}
}

void ssd2828_init(void)
{
	u16 id;

	LCM_DEBUG("SSD2828 config GPIO...\n");
	gpio_request_one(GPIO_TO_PIN(1, 19),
		GPIOF_OUT_INIT_HIGH, "ssd2828-reset");
	gpio_request(GPIO_TO_PIN(0, 12), "ssd2828-spi0-cs0");
	gpio_request(GPIO_TO_PIN(3, 14), "ssd2828-spi0-sclk");
	gpio_request(GPIO_TO_PIN(3, 15), "ssd2828-spi0-d0");
	gpio_request(GPIO_TO_PIN(3, 16), "ssd2828-spi0-d1");

	LCM_DEBUG("SSD2828 config GPIO completed.\n");

	Write_com(0x00b0);    
	id=Read_register();
	LCM_DEBUG("Linux Kernel--SSD2828 id is: 0x%x\n",id);

	SET_RESET_PIN(0);
	MDELAY(25);

	SET_RESET_PIN(1);
	MDELAY(120);

	init_lcm_registers();
	Write_com(0x00b0);    
	id=Read_register();
	LCM_DEBUG("SSD2828 id is: 0x%x\n",id);

	Write_com(0x00b1);
	id=Read_register();
	LCM_DEBUG("SSD2828 b1 is: 0x%x\n",id);

	Write_com(0x00b2);
	id=Read_register();
	LCM_DEBUG("SSD2828 b2 is: 0x%x\n",id);

	Write_com(0x00b3);
	id=Read_register();
	LCM_DEBUG("SSD2828 b3 is: 0x%x\n",id);

	Write_com(0x00b4);
	id=Read_register();
	LCM_DEBUG("SSD2828 b4 is: 0x%x\n",id);

	Write_com(0x00b5);
	id=Read_register();
	LCM_DEBUG("SSD2828 b5 is: 0x%x\n",id);

	Write_com(0x00b6);
	id=Read_register();
	LCM_DEBUG("SSD2828 b6 is: 0x%x\n",id);

	Write_com(0x00b7);
	id=Read_register();
	LCM_DEBUG("SSD2828 b7 is: 0x%x\n",id);

	Write_com(0x00b8);
	id=Read_register();
	LCM_DEBUG("SSD2828 b8 is: 0x%x\n",id);

	Write_com(0x00b9);
	id=Read_register();
	LCM_DEBUG("SSD2828 b9 is: 0x%x\n",id);

	Write_com(0x00ba);
	id=Read_register();
	LCM_DEBUG("SSD2828 ba is: 0x%x\n",id);

	Write_com(0x00bb);
	id=Read_register();
	LCM_DEBUG("SSD2828 bb is: 0x%x\n",id);
}

