
/* NAND FLASH������ */
#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))

/* GPIO */
#define GPHCON              (*(volatile unsigned long *)0x56000070)
#define GPHUP               (*(volatile unsigned long *)0x56000078)

/* UART registers*/
#define ULCON0              (*(volatile unsigned long *)0x50000000)
#define UCON0               (*(volatile unsigned long *)0x50000004)
#define UFCON0              (*(volatile unsigned long *)0x50000008)
#define UMCON0              (*(volatile unsigned long *)0x5000000c)
#define UTRSTAT0            (*(volatile unsigned long *)0x50000010)
#define UTXH0               (*(volatile unsigned char *)0x50000020)
#define URXH0               (*(volatile unsigned char *)0x50000024)
#define UBRDIV0             (*(volatile unsigned long *)0x50000028)

#define TXD0READY    (1<<2)

void nand_read_ll(unsigned int addr, unsigned char *buf,unsigned int len);


static int isBootFromNorFlash(void)
{
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if (*p == 0x12345678) {
		*p = val;
		return 0;
	} else {
		return 1;
	}
}


void copy_code_to_sdram(unsigned char * src, unsigned char * dest, unsigned int len)
{
	int i = 0;
	if (isBootFromNorFlash()) {
		while (i < len) {
			dest[i] =  src[i];
			i++;
		}
	} else {
		//nand_init();
		nand_read_ll((unsigned int)src, dest, len);
	}
}

void clear_bss(void)
{
	extern int __bss_start, __bss_end__;
	int *p = &__bss_start;

	for (; p < &__bss_end__; p++)
		*p = 0;
}

void nand_init_ll(void)
{
#define TACLS   0
#define TWRPH0  1
#define TWRPH1  0

	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	NFCONT = (1<<4)|(1<<1)|(1<<0);
}

static void nand_select(void)
{
	NFCONT &= ~(1<<1);
}

static void nand_deselect(void)
{
	NFCONT |= (1<<1);
}


static void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}

static void nand_addr(unsigned int addr)
{
	unsigned int col = addr % 2048;
	unsigned page = addr / 2048;
	volatile int i;

	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col << 8) & 0xff;
	for (i = 0; i < 10; i++);

	NFADDR = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 16) & 0xff;
	for (i = 0; i < 10; i++);
}

static void nand_wait_ready(void)
{
	while (!(NFSTAT & 1));
}

static unsigned char nand_data(void)
{
	return NFDATA;
}

void nand_read_ll(unsigned int addr, unsigned char *buf,unsigned int len)
{
	int col = addr % 2048;
	int i = 0;
	// 选中
	nand_select();

	while (i < len) {

		// 发出读命令00h
		nand_cmd(0x00);

		// 发出地址（分5步发出）
		nand_addr(addr);

		// 发出读命令30h
		nand_cmd(0x30);

		// 判断状态
		nand_wait_ready();

		// 读数据
		for (; (col < 2048) && (i < len); col++) {
			buf[i] = nand_data();
			i++;
			addr++;
		}
		col = 0;
	}

	// 取消选中
	nand_deselect();
}

#if 0
#define PCLK            50000000    // init.c�е�clock_init��������PCLKΪ50MHz
#define UART_CLK        PCLK        //  UART0��ʱ��Դ��ΪPCLK
#define UART_BAUD_RATE  115200      // ������
#define UART_BRD        ((UART_CLK  / (UART_BAUD_RATE * 16)) - 1)
#endif

#if 0
/*
 * ��ʼ��UART0
 * 115200,8N1,������
 */
void uart0_init(void)
{
    GPHCON  |= 0xa0;    // GPH2,GPH3����TXD0,RXD0
    GPHUP   = 0x0c;     // GPH2,GPH3�ڲ�����

    ULCON0  = 0x03;     // 8N1(8������λ���޽��飬1��ֹͣλ)
    UCON0   = 0x05;     // ��ѯ��ʽ��UARTʱ��ԴΪPCLK
    UFCON0  = 0x00;     // ��ʹ��FIFO
    UMCON0  = 0x00;     // ��ʹ������
    UBRDIV0 = UART_BRD; // ������Ϊ115200
}

/*
 * ����һ���ַ�
 */
void putc(unsigned char c)
{
    /* �ȴ���ֱ�����ͻ������е������Ѿ�ȫ�����ͳ�ȥ */
    while (!(UTRSTAT0 & TXD0READY));
    
    /* ��UTXH0�Ĵ�����д�����ݣ�UART���Զ��������ͳ�ȥ */
    UTXH0 = c;
}

void puts(char *str)
{
	int i = 0;
	while (str[i])
	{
		putc(str[i]);
		i++;
	}
}

void puthex(unsigned int val)
{
	/* 0x1234abcd */
	int i;
	int j;
	
	puts("0x");

	for (i = 0; i < 8; i++)
	{
		j = (val >> ((7-i)*4)) & 0xf;
		if ((j >= 0) && (j <= 9))
			putc('0' + j);
		else
			putc('A' + j - 0xa);
		
	}
	
}
#endif


