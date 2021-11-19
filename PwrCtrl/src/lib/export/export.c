/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: export.c
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	: 
* DATE			: 2019-01-06
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "export.h"
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "timer.h"
#include "log_sys.h"
/*
* *******************************************************************************
*                                  Public variables
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private variables
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private function prototypes
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private functions
* *******************************************************************************
*/
// AD检测
/*
* AD1-PT1_WK1 AD3-CC1_1 AD2-PT1_WK3 AD4-PT2_WK1 TEMP VER AD5-CC1_2 AD6-PT2_WK3
*/
// AD1 AD3 AD2 AD4 TEMP VER AD5 AD6

// ADC_CH8 ADC_CH9
#define 	AI8_DATA_PATH 		"/sys/devices/platform/soc/2100000.aips-bus/2198000.adc/iio:device0/in_voltage8_raw"
#define 	AI9_DATA_PATH 		"/sys/devices/platform/soc/2100000.aips-bus/2198000.adc/iio:device0/in_voltage9_raw"
#define 	SYSFS_GPIO_DIR 		"/sys/class/gpio"

#define 	MAX_BUF 			64
#define 	MAX_GPIO			2

#pragma pack(1)

typedef struct
{
	char name[16];			// gpio名称
	int gpio;				// gpio号
	char dir[8];			// gpio方向
	int fd;					// 文件描述符
	int enable;				// 0-未使能 1-使能
}GPIO_STRUCT;
#pragma pack()

static	GPIO_STRUCT		g_gpio[MAX_GPIO] =
{
//	name 				gpio 	dir  		enable
	{{"GPIO5_IO05"}, 	133, 	{"IN"}, 	1},			//这个dir不能是null啊
	{{"GPIO5_IO09"}, 	137, 	{"IN"}, 	1},
};

double ad_read(int channel)
{
	int fd_ad;
	int ret = 0;
	
	char data[20] = {0};
	double vref = 3.36;

	switch(channel)
	{
		case 0:
				fd_ad = open(AI8_DATA_PATH, O_RDONLY);
				break;
		case 1:
				fd_ad = open(AI9_DATA_PATH, O_RDONLY);
				break;
	}
	if(fd_ad < 0)
	{
		perror("open AI_DATA_PATH error");
		return RESULT_ERR;
	}
		
	memset(data, 0, sizeof(data));
	ret = read(fd_ad, data, sizeof(data));
	if(ret < 0) {
		perror("read data error");
		close(fd_ad);
		return RESULT_ERR;
	}
	close(fd_ad);

	return (vref * (atof(data) / 4096));
}

static unsigned char mode = 3;	//SPI_CPOL = 1   SPI_CPHA = 1
static unsigned char bits = 8;
static int speed = 100000; 		//100k
static int delay = 0;

/*
 * 发送一个消息
 */
static int mcp23s17_command(int fd, void *txbuf, void *rxbuf, int len)
{
	struct spi_ioc_transfer msg = {
		.tx_buf		= (int)txbuf,
		.rx_buf		= (int)rxbuf,
		.len		= len,
		.speed_hz	= speed,
		.delay_usecs	= delay,
		.bits_per_word	= bits,
		.cs_change	= 0,
	};

	return ioctl(fd, SPI_IOC_MESSAGE(1), &msg);
}

/*
 * 发送一个消息，接收一个返回值
 */
static int mcp23s17_command2(int fd, void *txbuf, void *rxbuf, int len)
{
	struct spi_ioc_transfer msg[] = {
		{
			.tx_buf		= (int)txbuf,
			.rx_buf		= 0,
			.len		= len,
			.speed_hz	= speed,
			.delay_usecs	= delay,
			.bits_per_word	= bits,
			.cs_change	= 0,
		},
		{
			.tx_buf		= 0,
			.rx_buf		= (int)rxbuf,
			.len		= len,
			.speed_hz	= speed,
			.delay_usecs	= delay,
			.bits_per_word	= bits,
			.cs_change	= 0,
		}	
	};
	
	return ioctl(fd, SPI_IOC_MESSAGE(2), &msg[0]);
}

/*
 * 配置一组寄存器
 */
int mcp23s17_write(int fd, unsigned char addr, unsigned char valA,unsigned char valB)
{
	int ret;
 	unsigned char tx[4];
	tx[0] = 0x40 | 0x00;
	tx[1] = addr;
	tx[2] = valA;
	tx[3] = valB;
	
	ret = mcp23s17_command(fd, tx, NULL, sizeof(tx));
	if (ret < 0) 
	{
		log_send("##########__EXPORT__##########: can't write register [%x]!",addr);
	}

	return ret;
}

/*
 * 读取一组寄存器
 */
int mcp23s17_read(int fd, unsigned char addr, unsigned char *pdata)
{
	int ret;
 	unsigned char tx[2], rx[2];
	rx[0] = 0;
	rx[1] = 0;
	tx[0] = 0x40 | 0x01;
	tx[1] = addr;

	ret = mcp23s17_command2(fd, tx, rx, sizeof(tx));
	if (ret < 0) 
	{
		log_send("##########__EXPORT__##########: can't read register [%x]!",addr);
	}

	*pdata =  rx[0];
	 pdata++;
	*pdata =  rx[1];
	return ret;
}

/*
 * 配置输入输出
 */
int mcp23s17_init()
{
	int fd;

	fd = open(MCP23S17, O_RDWR);
	if (fd < 0)
		log_send("##########__EXPORT__##########: can't open mcp23s17 device!");
	else
		log_send("##########__EXPORT__##########: open mcp23s17 device success!");

	
	/*
	 * spi mode 工作模式设置/读回
	 */
	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't set wr spi mode!");
		return RESULT_ERR;
	}

	if (ioctl(fd, SPI_IOC_RD_MODE, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't get spi mode!");
		return RESULT_ERR;
	}

	/*
	 * bits per word 设置每个字节的位数
	 */
	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't set bits per word!");
		return RESULT_ERR;
	}

	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't get bits per word!");
		return RESULT_ERR;
	}

	/*
	 * max speed hz 设置最大波特率
	 */
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't set max speed hz!");
		return RESULT_ERR;
	}

	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &mode) == (-1))
	{
		log_send("##########__EXPORT__##########: can't get max speed hz!");
		return RESULT_ERR;
	}
	
/*	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	usleep(100000);	*/
	
	//IOCON设置为IOCON_SEQOP = 1；字节模式
	mcp23s17_write(fd, 0x0A, 0x20, 0x20); 
	log_send("##########__EXPORT__##########: SPI send 0x40 0x0A 0x20");
	usleep(100000);
	
	//GPA0 GPA1 
	//GPB0 GPB4 GPB5 GPB6 GPB7 
	//设置为输入，其他设置为输出
	//1-输入 0-输出
	mcp23s17_write(fd, 0x00, GPA_DIR, GPB_DIR);
	log_send("##########__EXPORT__##########: GPIOA GPIOB OUTPUT");
	usleep(100000);	

	mcp23s17_write(fd, 0x12, 0x00, 0x00);
	usleep(100000);	

	return fd;
} 

int gpio_get_value(unsigned int gpioIndex, unsigned int *value)
{
	int fd;
	char buf[MAX_BUF];
	char ch;

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", g_gpio[gpioIndex].gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0)
	{
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0')
	{
		*value = GPIO_H;
	} else
	{
		*value = GPIO_L;
	}

	close(fd);

	return 0;
}

int gpio_set_value(unsigned int gpioIndex, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", g_gpio[gpioIndex].gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0)
	{
		return fd;
	}

	if (value != GPIO_L)
	{
		write(fd, "1", 2);
	}
	else
	{
		write(fd, "0", 2);
	}

	close(fd);

	return 0;
}

int gpio_init(void)
{
	int i, fd, len;
	char buf[MAX_BUF];

	for(i = 0; i < MAX_GPIO; i++)
	{
		if(g_gpio[i].enable == 1)
		{
			fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
			if (fd < 0)
			{
				return fd;
			}
			len = snprintf(buf, sizeof(buf), "%d", g_gpio[i].gpio);
			write(fd, buf, len);
			close(fd);
			
			snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", g_gpio[i].gpio);
			fd = open(buf, O_WRONLY);
			if (fd < 0)
			{
				return fd;
			}
			write(fd, g_gpio[i].dir, strlen(g_gpio[i].dir)+1);
			close(fd);
		}
	}
	
	return 0;
}

int pwm_init(void)
{
	int fd = 0;
	
	//system("GodHand -a  0x20e0054 -w -v 0x4");
	//system("GodHand -a  0x20E02E0 -w -v 0x17059");
	//usleep(100*1000);
/*
	fd = open(PWM_K_5_EXPORT, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_5_EXPORT##########: Failed PWM 5");
		printf("##########PWM_K_5_EXPORT##########: Failed PWM 5\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_DISABLE, strlen(PWM_DISABLE));//复用定义而已
	close(fd);

	fd = open(PWM_K_5_PERIOD_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_5_PERIOD_PATH##########: Failed PWM 5");
		printf("##########PWM_K_5_PERIOD_PATH##########: Failed PWM 5\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_PERIOD, strlen(PWM_PERIOD));
	close(fd);

	fd = open(PWM_K_5_DUTY_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_5_DUTY_PATH##########: Failed PWM 5");
		printf("##########PWM_K_5_DUTY_PATH##########: Failed PWM 5\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_DUTY, strlen(PWM_DUTY));
	close(fd);

	fd = open(PWM_K_5_ENABLE_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_5_ENABLE_PATH##########: Failed PWM 5");
		printf("##########PWM_K_5_ENABLE_PATH##########: Failed PWM 5\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_ENABLE, strlen(PWM_ENABLE));
	close(fd);
*/
	fd = open(PWM_K_7_EXPORT, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_7_EXPORT##########: Failed PWM 7");
		printf("##########PWM_K_7_EXPORT##########: Failed PWM 7\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_DISABLE, strlen(PWM_DISABLE));//复用定义而已
	close(fd);

	fd = open(PWM_K_7_PERIOD_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_7_PERIOD_PATH##########: Failed PWM 7");
		printf("##########PWM_K_7_PERIOD_PATH##########: Failed PWM 7\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_PERIOD, strlen(PWM_PERIOD));
	close(fd);

	fd = open(PWM_K_7_DUTY_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_7_DUTY_PATH##########: Failed PWM 7");
		printf("##########PWM_K_7_DUTY_PATH##########: Failed PWM 7\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_DUTY, strlen(PWM_DUTY));
	close(fd);

	fd = open(PWM_K_7_ENABLE_PATH, O_WRONLY);
	if (fd < 0)
	{
		log_send("##########PWM_K_7_ENABLE_PATH##########: Failed PWM 7");
		printf("##########PWM_K_7_ENABLE_PATH##########: Failed PWM 7\n\r");
		return RESULT_ERR;
	}
	write(fd, PWM_ENABLE, strlen(PWM_ENABLE));
	close(fd);

	return 0;
}

/****************PWM*****************/
int pwm_ctrl(char *pwm, int val)
{
	int fd = 0;
	char buffer[256] = {0};

	if (!strcmp(pwm, PWM_K_5))
	{
		fd = open(PWM_K_5_DUTY_PATH, O_WRONLY);
		if (fd < 0)
		{
			log_send("##########__EXPORT__##########: Failed OPEN PWM 5 DUTYCYCLE!");
			return RESULT_ERR;
		}
		sprintf(buffer, "%d", 1000000*val/100);
		write(fd, buffer, strlen(buffer));
		close(fd);
		return RESULT_OK;
	}
	else if (!strcmp(pwm, PWM_K_7))
	{
		fd = open(PWM_K_7_DUTY_PATH, O_WRONLY);
		if (fd < 0)
		{
			log_send("##########PWM_K_7_DUTY_PATH##########: Failed OPEN PWM 7 DUTYCYCLE!");
			printf("##########PWM_K_7_DUTY_PATH##########: Failed PWM 7\n\r");
			return RESULT_ERR;
		}
		sprintf(buffer, "%d", 1000000*val/100);
		write(fd, buffer, strlen(buffer));
		close(fd);
		return RESULT_OK;
	}

	return RESULT_OK;
}

