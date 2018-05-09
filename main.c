/*
Copyright 2018 by 1779729350 All Rights Reserved.
License : GNU General Public License v3.0
 */
#include <stdio.h>
#include <intrins.h>
#include "STC12C5A60S2.H"

#define ADC_IN 0x01 //P1.0����ADC����
#define ELECT_MAX 10    //������ʱ����
#define TEMPERATURE_MAX 100     //������ʱ�¶�
#define VOLTAGE_MAX 500     //������ʱ��ѹ
#define LCD1602 P0      //LCD�����Ӳ��п�

sbit RS = P2^0;    //�����־λ�� = 0��������ָ���״̬λ����= 1�����������ݣ�
sbit RW = P2^1;    //��д��־λ�� = 0��д����= 1������
sbit E  = P2^2;    //ʹ��λ��Ϊ 1 ʱ����д��Ϣ��Ϊ ������ ʱ��ִ��ָ��
sbit BF = P0^7;    //HD44780���Ƶ�1602LCD�� æ ״̬λ��ֻ��ͨ��ָ���ȡ��

//SPI�ӿڶ���
sbit spi_di=P1^5;
sbit spi_clk=P1^7;
sbit spi_do=P1^6;

//Ҫ��ʾ���ַ���
unsigned char code lcd_str0[]="Starting...";
unsigned char lcd_str1[17];
unsigned char lcd_str2[17];
bit flag=0;
float voltage=120.0, elect=5.02,temperature=24.77,frequency=49.76;
unsigned char timecount=0,f_count=0;

void Delay1ms(unsigned char a);
bit LCD_Busy_Test(void);
void LCD_Write_Com(unsigned char dictate);
void LCD_Write_Address(unsigned char addr);
void LCD_Write_Data(unsigned char char_data);
void LCD_init();
void LCD_Display(unsigned char strinput[],bit lineinput);
unsigned char spi_read();
void spi_write(unsigned char spi_dat);
void cs5463_init();
unsigned long cs5463_readVrms();
unsigned long cs5463_readIrms();
unsigned long cs5463_readT();
//unsigned int ADC_get();
void init();

void main(void)
{
    LCD_init();
    cs5463_init();
    init();
    LCD_Display(lcd_str0,0);
    Delay1ms(1000);
    while(1)
    {
        elect=cs5463_readIrms();
        elect=elect/16777215;
        elect=elect*ELECT_MAX;
        temperature=cs5463_readT();
        temperature/=16777215;
        temperature=temperature*TEMPERATURE_MAX;
        voltage=cs5463_readVrms();
        voltage/=16777215;
        voltage=voltage*VOLTAGE_MAX;
        sprintf(lcd_str1,"U=%0.1f",voltage," I=%0.2f",elect);       //ƴ����Ļ��һ���ַ���
        LCD_Display(lcd_str1,0);    //��ʾ��һ���ַ�
        sprintf(lcd_str2,"f=%0.2",frequency," T=%0.2f",temperature);    //ƴ����Ļ�ڶ����ַ���
        LCD_Display(lcd_str2,1);    //��ʾ�ڶ����ַ�
        while(flag) ;
    }

}

void init()
{
    EA=1;           //��CPU�ж�
    IT0=1;          //���ش���
    EX0=1;          //���ⲿ�ж�0
    IT1=1;          //���ش���
    EX1=1;          //  ���ⲿ�ж�1
    ET0=1;          //����ʱ��0�жϣ�11.0592MHz
	AUXR &= 0x7F;	//��ʱ��ʱ��12Tģʽ
	TMOD &= 0xF0;	//���ö�ʱ��ģʽ
	TL0 = 0x00;		//���ö�ʱ��ֵ
	TH0 = 0x4C;		//���ö�ʱ��ֵ
	TF0 = 0;		//���TF0��־
	TR0 = 1;		//��ʱ��0��ʼ��ʱ
}

bit LCD_Busy_Test(void)
{
    //����������洢����ֵ
    bit result;
    //��״̬λ
    RS = 0;
    RW = 1;
    E = 1;
    _nop_(); _nop_(); _nop_(); _nop_();  //��ʱ4���������ڣ���Ӳ����Ӧ,ʹE=1�ȶ�
    result = BF ;   //��æ״̬λBF��ֵ��������Ҫ���صı���
    //��ʱ��λE��λ���͵�ƽ
    E = 0;
    //��æ״̬λ���Ľ����Ϊ�����Ĳ�������
    return result;
}
void LCD_Write_Com(unsigned char dictate)
{
    //��æ�ȴ�
    while(LCD_Busy_Test()==1) ;
    //д����ָ��
    RS=0;
    RW=0;
    //E=0ʱ���������P0��
    E=0;
    _nop_(); _nop_();   //�ȴ�E=0�ȶ�
    LCD1602=dictate;
    _nop_(); _nop_();   //�ȴ�P0�ȶ�
    _nop_(); _nop_();
    E=1;          //E=1д��LCD�Ĵ���
    _nop_(); _nop_();   //�ȴ�E=1�ȶ�
    E=0;          //E=0,LCD��ʾ�Ĵ�������
}
void LCD_Write_Address(unsigned char addr)
{
    LCD_Write_Com(0x80|addr);
}
void LCD_Write_Data(unsigned char char_data)
{
    //��æ�ȴ�
    while(LCD_Busy_Test()==1) ;
    RS=1;
    RW=0;
    //E=0ʱ���������P0��
    E=0;
    _nop_(); _nop_();   //�ȴ�E=0�ȶ�
    LCD1602=char_data;
    _nop_(); _nop_();   //�ȴ�P0�ȶ�
    _nop_(); _nop_();
    E=1;          //E=1д��LCD�Ĵ���
    _nop_(); _nop_();   //�ȴ�E=1�ȶ�
    E=0;          //E=0,LCD��ʾ�Ĵ�������
}
void LCD_init()
{
    Delay1ms(15);
    //ƥ��Ӳ��������: ָ��6��8λ�����ߣ�2����ʾ��5*7����
    LCD_Write_Com(0x38); Delay1ms(5);
    LCD_Write_Com(0x38); Delay1ms(5);
    LCD_Write_Com(0x38); Delay1ms(5);
    //��ʾ������ꡢ��˸������: ָ��4������ʾ������꣬�����˸
    LCD_Write_Com(0x0f); Delay1ms(5);
    //��ꡢ��Ļ�ƶ�������: ָ��3��������ƣ���Ļ���岻�ƶ�
    LCD_Write_Com(0x06); Delay1ms(5);
    //����
    LCD_Write_Com(0x01); Delay1ms(5);
}
void LCD_Display(unsigned char strinput[],bit lineinput)
{
    unsigned char i=0;
    //����,��ʾ��һ���ַ�
    //LCD_Write_Com(0x01); Delay1ms(5);
        //������ʼλ��
    if(lineinput==0)
        LCD_Write_Address(0x00+0);
    else
        LCD_Write_Address(0x40+0);
    Delay1ms(5);
        //��ʾ�ַ���
    while(strinput[i] != '\0')
    {
        LCD_Write_Data(strinput[i]);
        i++;
        Delay1ms(5);
    }
}

void spi_write(unsigned char spi_dat)    //��SPI����д��һ���ֽ�����
{
   unsigned char i;
   //spi_cs=0;
   for(i=0;i<8;i++)
   {
        spi_clk=0;
        if((spi_dat & 0x80)==0x80)
            spi_di=1;
        else
            spi_di=0;
        spi_clk=1;
        spi_dat=(spi_dat<<1);
   }
}
unsigned char spi_read()    //��SPI��������һ���ֽ�����
{
   unsigned char i,spi_dat;
   //spi_cs=0;
   for (i=0;i<8;i++)
   {
        spi_clk=0;
        spi_dat=(spi_dat<<1);
        spi_clk=1;
        if(spi_do==1)
            spi_dat|=0x01;
        else
            spi_dat&=~0x01;
   }
   return spi_dat;
}

void cs5463_init()  //��ʼ��CS5463
{
    //��ʼ��
    spi_write(0xff);
    spi_write(0xff);
    spi_write(0xff);
    spi_write(0xfe);
    //д״̬�Ĵ���
    spi_write(0x5e);
    spi_write(0x80);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x00);
    //д���üĴ���
    spi_write(0x40);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x01);
    //д���ڼ����Ĵ���,N=4000
    spi_write(0x4a);
    spi_write(0x00);
    spi_write(0x0f);
    spi_write(0xa0);
    //���μ����Ĵ���
    spi_write(0x74);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x00);
    //д����ģʽ�Ĵ���
    spi_write(0x64);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x01);
    //������������
    spi_write(0xe8);
}
unsigned long cs5463_readVrms() //��ȡ��ѹ��Чֵ���Գ����η���ֵ
{
    union{
        unsigned char c[3];
        unsigned long buf;
    }temp;
    spi_write(0x18);
    temp.c[0]=spi_read();
    temp.c[1]=spi_read();
    temp.c[2]=spi_read();
    return temp.buf;
}
unsigned long cs5463_readIrms() //��ȡ������Чֵ
{
    union{
        unsigned char c[3];
        unsigned long buf;
    }temp;
    spi_write(0x16);
    temp.c[0]=spi_read();
    temp.c[1]=spi_read();
    temp.c[2]=spi_read();
    return temp.buf;
}
unsigned long cs5463_readT()    //��ȡ�¶�
{
    union{
        unsigned char c[3];
        unsigned long buf;
    }temp;
    spi_write(0x26);
    temp.c[0]=spi_read();
    temp.c[1]=spi_read();
    temp.c[2]=spi_read();
    return temp.buf;
}

/* unsigned int ADC_get() //ADת��
{
    unsigned int ADC_Result;
    unsigned char ADC_Result1,ADC_Result2;
    P1ASF = ADC_IN; //Xѡ��P1.0��ΪADCת������ͨ��
    ADC_CONTR = 0X80; //��ADCת����Դ
    _nop_();
    _nop_();

    AUXR1 = 0X04; //������λ����ڼĴ���ADC_RES���Ͱ�λ����ڼĴ���ADC_RESL��
    ADC_CONTR |= 0X68; //ADC��ʼ������
    _nop_();
    _nop_();
    _nop_();
    _nop_();

    ADC_Result1 = ADC_RES;
    ADC_Result2 = ADC_RESL;
    ADC_Result=ADC_Result2+ADC_Result1*256;
    return ADC_Result;
}
*/
void Delay1ms(unsigned char n)		//@11.0592MHz
{
    for(;n--;n==0)
        {
	        unsigned char i, j;

            _nop_();
            i = 11;
            j = 190;
            do
            {
                while (--j);
            } while (--i);
        }
}

void INT0_int() interrupt 0    //ʩ���ش�������������½��ش����ж�
{
    f_count++;
}

void T0_int() interrupt 1      //1�����һ��Ƶ��
{
    timecount++;
    if(timecount==20)
    {
        timecount=0;
        frequency=1/f_count;
    }
}

void INT1_int() interrupt 2    //��ͣ
{
    flag=~flag;
}
