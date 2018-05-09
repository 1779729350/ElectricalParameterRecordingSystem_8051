/*
Copyright 2018 by 1779729350 All Rights Reserved.
License : GNU General Public License v3.0
 */
#include <stdio.h>
#include <intrins.h>
#include "STC12C5A60S2.H"

#define ADC_IN 0x01 //P1.0用于ADC输入
#define ELECT_MAX 10    //满量程时电流
#define TEMPERATURE_MAX 100     //满量程时温度
#define VOLTAGE_MAX 500     //满量程时电压
#define LCD1602 P0      //LCD的连接并行口

sbit RS = P2^0;    //对象标志位： = 0（对象是指令或状态位）；= 1（对象是数据）
sbit RW = P2^1;    //读写标志位： = 0（写）；= 1（读）
sbit E  = P2^2;    //使能位：为 1 时，读写信息，为 负跳变 时，执行指令
sbit BF = P0^7;    //HD44780控制的1602LCD的 忙 状态位（只能通过指令读取）

//SPI接口定义
sbit spi_di=P1^5;
sbit spi_clk=P1^7;
sbit spi_do=P1^6;

//要显示的字符串
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
        sprintf(lcd_str1,"U=%0.1f",voltage," I=%0.2f",elect);       //拼接屏幕第一行字符串
        LCD_Display(lcd_str1,0);    //显示第一行字符
        sprintf(lcd_str2,"f=%0.2",frequency," T=%0.2f",temperature);    //拼接屏幕第二行字符串
        LCD_Display(lcd_str2,1);    //显示第二行字符
        while(flag) ;
    }

}

void init()
{
    EA=1;           //开CPU中断
    IT0=1;          //边沿触发
    EX0=1;          //开外部中断0
    IT1=1;          //边沿触发
    EX1=1;          //  开外部中断1
    ET0=1;          //开定时器0中断，11.0592MHz
	AUXR &= 0x7F;	//定时器时钟12T模式
	TMOD &= 0xF0;	//设置定时器模式
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x4C;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}

bit LCD_Busy_Test(void)
{
    //定义变量，存储返回值
    bit result;
    //读状态位
    RS = 0;
    RW = 1;
    E = 1;
    _nop_(); _nop_(); _nop_(); _nop_();  //延时4个机器周期，给硬件响应,使E=1稳定
    result = BF ;   //将忙状态位BF的值读给函数要返回的变量
    //将时能位E复位到低电平
    E = 0;
    //将忙状态位读的结果作为函数的参数返回
    return result;
}
void LCD_Write_Com(unsigned char dictate)
{
    //遇忙等待
    while(LCD_Busy_Test()==1) ;
    //写控制指令
    RS=0;
    RW=0;
    //E=0时数据输出在P0口
    E=0;
    _nop_(); _nop_();   //等待E=0稳定
    LCD1602=dictate;
    _nop_(); _nop_();   //等待P0稳定
    _nop_(); _nop_();
    E=1;          //E=1写入LCD寄存器
    _nop_(); _nop_();   //等待E=1稳定
    E=0;          //E=0,LCD显示寄存器内容
}
void LCD_Write_Address(unsigned char addr)
{
    LCD_Write_Com(0x80|addr);
}
void LCD_Write_Data(unsigned char char_data)
{
    //遇忙等待
    while(LCD_Busy_Test()==1) ;
    RS=1;
    RW=0;
    //E=0时数据输出在P0口
    E=0;
    _nop_(); _nop_();   //等待E=0稳定
    LCD1602=char_data;
    _nop_(); _nop_();   //等待P0稳定
    _nop_(); _nop_();
    E=1;          //E=1写入LCD寄存器
    _nop_(); _nop_();   //等待E=1稳定
    E=0;          //E=0,LCD显示寄存器内容
}
void LCD_init()
{
    Delay1ms(15);
    //匹配硬件的设置: 指令6：8位数据线，2行显示，5*7点阵
    LCD_Write_Com(0x38); Delay1ms(5);
    LCD_Write_Com(0x38); Delay1ms(5);
    LCD_Write_Com(0x38); Delay1ms(5);
    //显示屏、光标、闪烁的设置: 指令4：开显示，开光标，光标闪烁
    LCD_Write_Com(0x0f); Delay1ms(5);
    //光标、屏幕移动的设置: 指令3：光标右移，屏幕整体不移动
    LCD_Write_Com(0x06); Delay1ms(5);
    //清屏
    LCD_Write_Com(0x01); Delay1ms(5);
}
void LCD_Display(unsigned char strinput[],bit lineinput)
{
    unsigned char i=0;
    //清屏,显示第一排字符
    //LCD_Write_Com(0x01); Delay1ms(5);
        //设置起始位置
    if(lineinput==0)
        LCD_Write_Address(0x00+0);
    else
        LCD_Write_Address(0x40+0);
    Delay1ms(5);
        //显示字符串
    while(strinput[i] != '\0')
    {
        LCD_Write_Data(strinput[i]);
        i++;
        Delay1ms(5);
    }
}

void spi_write(unsigned char spi_dat)    //向SPI器件写入一个字节数据
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
unsigned char spi_read()    //从SPI器件读出一个字节数据
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

void cs5463_init()  //初始化CS5463
{
    //初始化
    spi_write(0xff);
    spi_write(0xff);
    spi_write(0xff);
    spi_write(0xfe);
    //写状态寄存器
    spi_write(0x5e);
    spi_write(0x80);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x00);
    //写配置寄存器
    spi_write(0x40);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x01);
    //写周期计数寄存器,N=4000
    spi_write(0x4a);
    spi_write(0x00);
    spi_write(0x0f);
    spi_write(0xa0);
    //屏蔽计数寄存器
    spi_write(0x74);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x00);
    //写操作模式寄存器
    spi_write(0x64);
    spi_write(0x00);
    spi_write(0x00);
    spi_write(0x01);
    //开启连续计算
    spi_write(0xe8);
}
unsigned long cs5463_readVrms() //读取电压有效值，以长整形返回值
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
unsigned long cs5463_readIrms() //读取电流有效值
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
unsigned long cs5463_readT()    //读取温度
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

/* unsigned int ADC_get() //AD转换
{
    unsigned int ADC_Result;
    unsigned char ADC_Result1,ADC_Result2;
    P1ASF = ADC_IN; //X选择P1.0作为ADC转换输入通道
    ADC_CONTR = 0X80; //打开ADC转换电源
    _nop_();
    _nop_();

    AUXR1 = 0X04; //将高两位存放在寄存器ADC_RES，低八位存放在寄存器ADC_RESL中
    ADC_CONTR |= 0X68; //ADC初始化设置
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

void INT0_int() interrupt 0    //施密特触发器输出方波下降沿触发中断
{
    f_count++;
}

void T0_int() interrupt 1      //1秒计算一次频率
{
    timecount++;
    if(timecount==20)
    {
        timecount=0;
        frequency=1/f_count;
    }
}

void INT1_int() interrupt 2    //暂停
{
    flag=~flag;
}
