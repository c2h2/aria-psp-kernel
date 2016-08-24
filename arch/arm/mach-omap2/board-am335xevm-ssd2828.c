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
#define SET_RESET_PIN_LCD(v)    gpio_direction_output(GPIO_TO_PIN(1, 17), v)


#define SET_LSCE_LOW   gpio_direction_output(GPIO_TO_PIN(0, 5), 0)
#define SET_LSCE_HIGH  gpio_direction_output(GPIO_TO_PIN(0, 5), 1)

#define SET_LSCK_LOW   gpio_direction_output(GPIO_TO_PIN(0, 2), 0)
#define SET_LSCK_HIGH  gpio_direction_output(GPIO_TO_PIN(0, 2), 1)

#define SET_LSDA_LOW   gpio_direction_output(GPIO_TO_PIN(0, 3), 0)
#define SET_LSDA_HIGH  gpio_direction_output(GPIO_TO_PIN(0, 3), 1)

#define GET_HX_SDI     gpio_get_value(GPIO_TO_PIN(0, 4))


#define HX_WR_COM       (0x70)
#define HX_WR_REGISTER  (0x72)
#define HX_RD_REGISTER  (0x73)

//8" show config
#define LCD_XSIZE_TFT	(800) 
#define LCD_YSIZE_TFT	(1280)
#define LCD_VBPD	(20)
#define LCD_VFPD	(20)
#define LCD_VSPW	(2)
#define LCD_HBPD	(42)
#define LCD_HFPD	(44)
#define LCD_HSPW	(2)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
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
		if (data & (1 << 23)) {
			SET_LSDA_HIGH;
		} else {
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
	for(i=0;i<8;i++) // 8  Data
	{
		if ( front_data& 0x80)
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
	for(j=0;j<16;j++) // 16 Data
	{
		SET_LSCK_HIGH;
		UDELAY(10);
		SET_LSCK_LOW;
		value<<= 1;
		//value |= GET_HX_SDI;
		if (GET_HX_SDI)
			value |= 1;

		UDELAY(10); 
	}
	SET_LSCE_HIGH;
	return value;      
}


void SPI_3W_SET_PAs(unsigned char data)
{
	unsigned int i;
	SET_LSCE_HIGH;
	SET_LSCK_HIGH;
	SET_LSDA_HIGH;
	UDELAY(10);
	SET_LSCE_LOW;
	UDELAY(10);
	for (i = 0; i < 8; ++i)
	{
		if (data & (1 << 7)) {
			SET_LSDA_HIGH;
		} else {
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
void SPI_WriteData(unsigned char value)
{
	SPI_3W_SET_PAs(value);
}




static void init_lcm_registers(void)
{
	LCM_DEBUG("[SSD2828************]: Initializing SSD2828 registers.\n");

	//8" show config
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

	Write_com(0xBF);//Set EXTC

	Write_register(0xffb9);
	Write_register(0x9483);
	
	Write_com(0x00bc);
	Write_register(16); 
	

	Write_com(0xBF);
	Write_register(0x64b1);
	//SPI_WriteData(0xB1); 
	//SPI_WriteData(0x64);
	Write_register(0x3010);
	//SPI_WriteData(0x10);
	//SPI_WriteData(0x30);
	Write_register(0x3443);
	//SPI_WriteData(0x43);
	//SPI_WriteData(0x34);
	Write_register(0xf111);
	//SPI_WriteData(0x11);
	//SPI_WriteData(0xF1);
	Write_register(0x7081);
	//SPI_WriteData(0x81);
	//SPI_WriteData(0x70);
	Write_register(0x34d9);
	//SPI_WriteData(0xD9);
	//SPI_WriteData(0x34);
	Write_register(0xc080);
	//SPI_WriteData(0x80);
	//SPI_WriteData(0xC0);
	Write_register(0x42d2);
	//SPI_WriteData(0xD2);
	//SPI_WriteData(0x41);


	Write_com(0x00bc);
	Write_register(13); 

	Write_com(0xBF);

	Write_register(0x45b2);
	//SPI_WriteData(0xB2);
	//SPI_WriteData(0x45);
	Write_register(0x0f64);
	//SPI_WriteData(0x64);
	//SPI_WriteData(0x0f);//0f
	Write_register(0x4009);
	//SPI_WriteData(0x09);//09
	//SPI_WriteData(0x40);
	Write_register(0x081c);
	//SPI_WriteData(0x1C);
	//SPI_WriteData(0x08);
	Write_register(0x1c08);
	//SPI_WriteData(0x08);
	//SPI_WriteData(0x1C);
	Write_register(0x004d);
	//SPI_WriteData(0x4D);
	//SPI_WriteData(0x00);
	Write_register(0x00);
	//SPI_WriteData(0x00);

	Write_com(0x00bc);
	Write_register(23); 

	Write_com(0xBF);

	Write_register(0x07b4);
	//SPI_WriteData(0xB4);
	//SPI_WriteData(0x07);
	Write_register(0x076e);
	//SPI_WriteData(0x6E);
	//SPI_WriteData(0x07);
	Write_register(0x6f71);
	//SPI_WriteData(0x71);
	//SPI_WriteData(0x6F);
	Write_register(0x0070);
	//SPI_WriteData(0x70);
	//SPI_WriteData(0x00);
	Write_register(0x0100);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x01);
	Write_register(0x0f6e);
	//SPI_WriteData(0x6E);
	//SPI_WriteData(0x0F);
	Write_register(0x076e);
	//SPI_WriteData(0x6E);
	//SPI_WriteData(0x07);
	Write_register(0x6f71);
	//SPI_WriteData(0x71);
	//SPI_WriteData(0x6F);
	Write_register(0x0070);
	//SPI_WriteData(0x70);
	//SPI_WriteData(0x00);
	Write_register(0x0100);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x01);
	Write_register(0x0f6e);
	//SPI_WriteData(0x6E);
	//SPI_WriteData(0x0F);
	Write_register(0x006e);
	//SPI_WriteData(0x6E);



	Write_com(0x00bc);
	Write_register(3); 

	Write_com(0xBF);

	Write_register(0x6fb6);
	//SPI_WriteData(0xB6);
	//SPI_WriteData(0x6F);
	Write_register(0x6f);
	//SPI_WriteData(0x6F);

	Write_com(0x00bc);
	Write_register(2); 

	Write_com(0xBF);

	Write_register(0x09cc);
	//SPI_WriteData(0xCC);
	//SPI_WriteData(0x09);


	Write_com(0x00bc);
	Write_register(33); 

	
	Write_com(0xBF);

	Write_register(0x00d3);
	//SPI_WriteData(0xD3);
	//SPI_WriteData(0x00);
	Write_register(0x0008);
	//SPI_WriteData(0x08);
	//SPI_WriteData(0x00);
	Write_register(0x0701);
	//SPI_WriteData(0x01);
	//SPI_WriteData(0x07);
	Write_register(0x0800);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x08);
	Write_register(0x1032);
	//SPI_WriteData(0x32);
	//SPI_WriteData(0x10);
	Write_register(0x000a);
	//SPI_WriteData(0x0A);
	//SPI_WriteData(0x00);
	Write_register(0x0005);
	//SPI_WriteData(0x05);
	//SPI_WriteData(0x00);
	Write_register(0x0a20);
	//SPI_WriteData(0x20);
	//SPI_WriteData(0x0A);
	Write_register(0x0905);
	//SPI_WriteData(0x05);
	//SPI_WriteData(0x09);
	Write_register(0x3200);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x32);
	Write_register(0x0810);
	//SPI_WriteData(0x10);
	//SPI_WriteData(0x08);
	Write_register(0x3500);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x35);
	Write_register(0x0d33);
	//SPI_WriteData(0x33);
	//SPI_WriteData(0x0D);
	Write_register(0x4707);
	//SPI_WriteData(0x07);
	//SPI_WriteData(0x47);
	Write_register(0x070d);
	//SPI_WriteData(0x0D);
	//SPI_WriteData(0x07);
	Write_register(0x0f47);
	//SPI_WriteData(0x47);
	//SPI_WriteData(0x0F);
	Write_register(0x08);
	//SPI_WriteData(0x08);

	Write_com(0x00bc);
	Write_register(45); 
	
	Write_com(0xBF);

	Write_register(0x03d5);
	//SPI_WriteData(0xD5);
	//SPI_WriteData(0x03);
	Write_register(0x0302);
	//SPI_WriteData(0x02);
	//SPI_WriteData(0x03);
	Write_register(0x0102);
	//SPI_WriteData(0x02);
	//SPI_WriteData(0x01);
	Write_register(0x0100);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x01);
	Write_register(0x0700);
	//SPI_WriteData(0x00);
	//SPI_WriteData(0x07);
	Write_register(0x0706);
	//SPI_WriteData(0x06);
	//SPI_WriteData(0x07);
	Write_register(0x0506);
	//SPI_WriteData(0x06);
	//SPI_WriteData(0x05);
	Write_register(0x0504);
	//SPI_WriteData(0x04);
	//SPI_WriteData(0x05);
	Write_register(0x2104);
	//SPI_WriteData(0x04);
	//SPI_WriteData(0x21);
	Write_register(0x1820);
	//SPI_WriteData(0x20);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x2318);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x23);
	Write_register(0x1822);
	//SPI_WriteData(0x22);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x1818);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x18);
	Write_register(0x18);
	//SPI_WriteData(0x18);

	Write_com(0x00bc);
	Write_register(43); 
	
	Write_com(0xBF);

	Write_register(0x03e0);
	//SPI_WriteData(0xE0);
	//SPI_WriteData(0x03);
	Write_register(0x1c17);
	//SPI_WriteData(0x17);
	//SPI_WriteData(0x1C);
	Write_register(0x302d);
	//SPI_WriteData(0x2D);
	//SPI_WriteData(0x30);
	Write_register(0x273b);
	//SPI_WriteData(0x3B);
	//SPI_WriteData(0x27);
	Write_register(0x0840);
	//SPI_WriteData(0x40);
	//SPI_WriteData(0x08);
	Write_register(0x0d0b);
	//SPI_WriteData(0x0B);
	//SPI_WriteData(0x0D);
	Write_register(0x0f18);
	//SPI_WriteData(0x18);
	//SPI_WriteData(0x0F);
	Write_register(0x1512);
	//SPI_WriteData(0x12);
	//SPI_WriteData(0x15);
	Write_register(0x1413);
	//SPI_WriteData(0x13);
	//SPI_WriteData(0x14);
	Write_register(0x1207);
	//SPI_WriteData(0x07);
	//SPI_WriteData(0x12);
	Write_register(0x1714);
	//SPI_WriteData(0x14);
	//SPI_WriteData(0x17);
	Write_register(0x1703);
	//SPI_WriteData(0x03);
	//SPI_WriteData(0x17);
	Write_register(0x2d1c);
	//SPI_WriteData(0x1C);
	//SPI_WriteData(0x2D);
	Write_register(0x3b30);
	//SPI_WriteData(0x30);
	//SPI_WriteData(0x3B);
	Write_register(0x4027);
	//SPI_WriteData(0x27);
	//SPI_WriteData(0x40);
	Write_register(0x0b08);
	//SPI_WriteData(0x08);
	//SPI_WriteData(0x0B);
	Write_register(0x180d);
	//SPI_WriteData(0x0D);
	//SPI_WriteData(0x18);
	Write_register(0x120f);
	//SPI_WriteData(0x0F);
	//SPI_WriteData(0x12);
	Write_register(0x1315);
	//SPI_WriteData(0x15);
	//SPI_WriteData(0x13);
	Write_register(0x0714);
	//SPI_WriteData(0x14);
	//SPI_WriteData(0x07);
	Write_register(0x1412);
	//SPI_WriteData(0x12);
	//SPI_WriteData(0x14);
	Write_register(0x17);
	//SPI_WriteData(0x17);

	Write_com(0x00bc);
	Write_register(6); 
	
	Write_com(0xBF);

	Write_register(0x1fc9);
	//SPI_WriteData(0xC9);
	//SPI_WriteData(0x1F);
	Write_register(0x1e2e);
	//SPI_WriteData(0x2E);
	//SPI_WriteData(0x1E);
	Write_register(0x101e);
	//SPI_WriteData(0x1E);
	//SPI_WriteData(0x10);


	

	Write_com(0x00b1);
	Write_register((LCD_VSPW<<8)|LCD_HSPW); //Vertical sync and horizontal sync active period 
	
	Write_com(0x00b2);
	Write_register((LCD_VBPD<<8)|LCD_HBPD);	//Vertical and horizontal back porch period 
	 
	Write_com(0x00b3);
	Write_register((LCD_VFPD<<8)|LCD_HFPD);	//Vertical and horizontal front porch period 
	
	Write_com(0x00b4);
	Write_register(LCD_XSIZE_TFT);		//Horizontal active period 
	
	Write_com(0x00b5);
	Write_register(LCD_YSIZE_TFT);		//Vertical active period
	
	Write_com(0x00b6);
	Write_register(0xc01b);				
	
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
	
	Write_com(0xBF);//Sleep out     

	Write_register(0x0011);
	//SPI_WriteData(0x11);      
	MDELAY(200); 

	Write_com(0x00bc);
	Write_register(1); 
	
	Write_com(0xBF);//Display on

	Write_register(0x0029);
	//SPI_WriteData(0x29);
	MDELAY(50); 
}

void ssd2828_init(void)
{
	u16 id;

	gpio_request_one(GPIO_TO_PIN(1, 19),
		GPIOF_OUT_INIT_HIGH, "lcd-reset");
	gpio_request_one(GPIO_TO_PIN(1, 17),
		GPIOF_OUT_INIT_HIGH, "ssd2828-reset");
	gpio_request(GPIO_TO_PIN(0, 5), "ssd2828-spi0-cs0");
	gpio_request(GPIO_TO_PIN(0, 2), "ssd2828-spi0-sclk");
	gpio_request(GPIO_TO_PIN(0, 3), "ssd2828-spi0-d0");
	gpio_request(GPIO_TO_PIN(0, 4), "ssd2828-spi0-d1");

	
	Write_com(0x00b0);    
	id=Read_register();
	LCM_DEBUG("Linux Kernel--SSD2828 id is: 0x%x\n",id);

	SET_RESET_PIN(0);
	SET_RESET_PIN_LCD(0);
	MDELAY(25);

	SET_RESET_PIN(1);
	SET_RESET_PIN_LCD(1);
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
	LCM_DEBUG("SSD2828 b6 is: 0x%x\n",id);

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


	Write_com(0x00bc);
	Write_register(0x0001);
	Write_com(0xBF);
	Write_register(0x00da);
	Write_com(0x00d4);
	Write_register(0x00fa);
	Write_com(0x00ff);
	Write_register(0xfa);
	id=Read_register();
	LCM_DEBUG("SSD2828 LCD ID1 is: 0x%x\n",id);

	Write_com(0x00bc);
	Write_register(0x0001);
	Write_com(0xBF);
	Write_register(0x00db);
	Write_com(0x00d4);
	Write_register(0x00fa);
	Write_com(0x00ff);
	Write_register(0xfa);
	id=Read_register();
	LCM_DEBUG("SSD2828 LCD ID2 is: 0x%x\n",id);

	Write_com(0x00bc);
	Write_register(0x0001);
	Write_com(0xBF);
	Write_register(0x00dc);
	Write_com(0x00d4);
	Write_register(0x00fa);
	Write_com(0x00ff);
	Write_register(0xfa);
	id=Read_register();
	LCM_DEBUG("SSD2828 LCD ID3 is: 0x%x\n",id);

	Write_com(0x00ee);
	Write_register(0x0600);
}

