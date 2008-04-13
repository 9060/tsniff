#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;

u16 us20_wait = 0;
u16 ir_code = 0;
u8  ir_state = 0;

u16 ir_bits;
bit ir_old = 1;
u8 ir_cnt = 0;

u8 xdata ir_rbuf[256];
u8 xdata ir_wbuf[256];
u8 ir_wbuf_len, ir_wbuf_idx;
u8 ir_rbuf_idx = 0;
u8 ir_wbuf_cnt = 0;

void timer0() interrupt 1  //register BANK1
{

	if(us20_wait != 0)
 	{
		--us20_wait;
	}

	if(us20_wait == 0)
	{
		if(ir_code == 0)
		{
			PD4 = 1;
			if(PD3 == ir_old){
				ir_cnt++;
				if(ir_cnt == 0xff){
					ir_rbuf[ir_rbuf_idx++] = 0xfe | ir_old;
					ir_cnt = 0;
				}
			}
			else{
				ir_rbuf[ir_rbuf_idx++] = (ir_cnt & 0xfe) | ir_old;
				ir_old = PD3;
				ir_cnt = 0;
			}
		}
		else if(ir_code == 0xffff){
			if(--ir_wbuf_cnt == 0){
				if(ir_wbuf[ir_wbuf_idx] & 1){
					PD4 = 1;
				}
				else{
					PD4 = 0;
				}
				ir_wbuf_cnt = ir_wbuf[ir_wbuf_idx] & 0xfe;
				ir_wbuf_idx++;
				if(ir_wbuf_idx == ir_wbuf_len)
					ir_wbuf_idx = 0;
			}
		}
		else{
			switch(ir_state)
			{
			case 0:
				us20_wait = 2400/2;	//22000us Interval time
				PD4 = 1;
				break;
			case 1:
				us20_wait = 230/2;	//2300us Header low
				PD4 = 0;
				break;
			case 2:
				us20_wait = 110/2;	//1100us Header hi
				PD4 = 1;
				break;
			case 3:
				us20_wait = 60/2;	//600us Header low
				PD4 = 0;
				ir_bits = ir_code;
				break;
			default:
				if(ir_state & 1)
				{
					us20_wait = 60/2;
					PD4 = 0;
				}
				else
				{
					if(ir_bits & 1)
					{
						us20_wait = 160/2;	//1:1600us
					}
					else
					{
						us20_wait = 50/2;	//0:500us
					}
					PD4 = 1;
					ir_bits = ir_bits >> 1;
				}
				break;
			}
			if(++ir_state == 4+16*2)
			{
				ir_state = 0;
			}
		}
	}
}

void timer_init()
{
	TMOD = 0x02;	//GATE0:0 CT0:0 M1M0:10
	CKCON = (CKCON&(~0x08)) | 0x08; //T0M:1(48MHz/4=12MHz)
	TL0 = 0;
	TH0 = 0x100-240;	//20us = 240count

	TR0 = 1;		//enable timer0
	ET0 = 1;		//timer0 irq enable
}
