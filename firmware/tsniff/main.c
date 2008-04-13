//-----------------------------------------------------------------------------
//   File:      main.c
//   Contents:  Hooks required to implement USB peripheral function.
//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//-----------------------------------------------------------------------------
#ifndef SDCC
#pragma noiv               // Do not generate interrupt vectors
#endif

#include "fx2.h"
#include "fx2regs.h"
#include "syncdly.h"            // SYNCDELAY macro
#include "tsniff_if.h"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;

#define GPIFTRIGRD 4
#define GPIF_EP2 0
#define GPIF_EP4 1
#define GPIF_EP6 2
#define GPIF_EP8 3

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings
BOOL enum_high_speed = FALSE;       // flag to let firmware know FX2 enumerated at high speed

void timer_init();
extern u16 us10_wait;

u8 val_ifconfig;
extern u16 ir_code;
extern u8 xdata ir_rbuf[256];
extern u8 xdata ir_wbuf[256];
extern u8 ir_wbuf_len, ir_wbuf_idx;
extern u8 ir_rbuf_idx;
u8 ir_rbuf_idx_cpy;

#define VR_NAKALL_ON    0xD0
#define VR_NAKALL_OFF   0xD1

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

void TD_Init(void)             // Called once at startup
{ 
	//	unsigned char i;

	// set the CPU clock to 48MHz
	CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1 | bmCLKOE);
	SYNCDELAY; 

	//set FD bus 8bit
	EP2FIFOCFG = 0x04;
	SYNCDELAY;
	EP4FIFOCFG = 0x04;
	SYNCDELAY;
	EP6FIFOCFG = 0x04;
	SYNCDELAY;
	EP8FIFOCFG = 0x04;
	SYNCDELAY;

	timer_init();

	// we are just using the default values, yes this is not necessary...
	EP1OUTCFG = 0xA0;
	EP1INCFG = 0xA0;
	SYNCDELAY;                    // see TRM section 15.14
	EP1OUTBC = 0;

	REVCTL = 0x03; SYNCDELAY;		// see TRM 15.5.9

	EP2CFG = 0xA2;	//(OUT, Size = 512, buf = Quad (Buf x2), BULK);
	SYNCDELAY;                    

	EP4CFG = 0xE2;	//(IN, Size = 512, buf = Quad (Buf x2), BULK)
	SYNCDELAY;                    

	EP6CFG = 0xE0;	//(IN, Size = 512, buf = Quad (Buf x4), BULK) 
	SYNCDELAY;

	EP8CFG = 0x00;
	SYNCDELAY;                    
  
	FIFOPINPOLAR = 0x00; SYNCDELAY;
	EP4AUTOINLENH = 0x00; SYNCDELAY;
	EP4AUTOINLENL = 0x20; SYNCDELAY;
	EP6AUTOINLENH = 0x02; SYNCDELAY;
	EP6AUTOINLENL = 0x00; SYNCDELAY;

	FIFORESET = 0x80; SYNCDELAY; // activate NAK-ALL to avoid race conditions
	FIFORESET = 0x02; SYNCDELAY; // reset, FIFO 2
	FIFORESET = 0x04; SYNCDELAY; // reset, FIFO 4
	FIFORESET = 0x06; SYNCDELAY; // reset, FIFO 6
	FIFORESET = 0x08; SYNCDELAY; // reset, FIFO 8
	FIFORESET = 0x00; SYNCDELAY; // deactivate NAK-ALL

	// enable dual autopointer feature & inc
	AUTOPTRSETUP = 0x07;

	IOD = 0x00;	//MODE_IDLE
	OED = 0xc0;	//MODE is output

	PINFLAGSAB = 0xe8;	//FLAGA=ep2EF FLAGB=ep6FF
	SYNCDELAY;                    

#ifdef SDCC
	{
		// wait
		WORD i;
		for(i = 0; i < 30000; i++);
	}
#endif
}

// Called repeatedly while the device is idle
void TD_Poll(void){
	u8 i, j;
	u8 cmd_cnt,ret_cnt;
	u8 addr_mask;
	if (!(EP1OUTCS & bmEPBUSY)) {
		cmd_cnt = EP1OUTBC;
		AUTOPTR1H = MSB(&EP1OUTBUF);
		AUTOPTR1L = LSB(&EP1OUTBUF);
		AUTOPTRH2 = MSB(&EP1INBUF);
		AUTOPTRL2 = LSB(&EP1INBUF);
		ret_cnt = 0;
		for (;;) {
			if (cmd_cnt-- == 0)
				break;
			switch (XAUTODAT1) {
			case CMD_EP4AUTOINLEN:
				EP4FIFOCFG = 0x04;
				SYNCDELAY;
				REVCTL = 0x03; SYNCDELAY;		// see TRM 15.5.9
				EP4CFG = 0xE2;	//(IN, Size = 512, buf = Quad (Buf x2), BULK)
				SYNCDELAY;                    
				FIFOPINPOLAR = 0x00; SYNCDELAY;
				EP4AUTOINLENH = XAUTODAT1; SYNCDELAY;
				EP4AUTOINLENL = XAUTODAT1; SYNCDELAY;
				FIFORESET = 0x80; SYNCDELAY; // activate NAK-ALL to avoid race conditions
				FIFORESET = 0x04; SYNCDELAY; // reset, FIFO 4
				FIFORESET = 0x00; SYNCDELAY; // deactivate NAK-ALL
				AUTOPTRSETUP = 0x07;
				cmd_cnt-=2;
				break;

			case CMD_IR_WBUF:
				i = XAUTODAT1;
				j = XAUTODAT1;
				cmd_cnt -= 2+j;
				for (;;) {
					ir_wbuf[i++] = XAUTODAT1;
					if (--j == 0) break;
				}
				ir_wbuf_len = i;
				ir_wbuf_idx = 0;
				break;

			case CMD_IR_RBUF:
				if (XAUTODAT1 == 0)
					ir_rbuf_idx_cpy = ir_rbuf_idx;
				cmd_cnt--;
				for (i = 0; i < 64; i++) {
					XAUTODAT2 = ir_rbuf[--ir_rbuf_idx_cpy];
					ret_cnt++;
				}
				break;

			case CMD_IR_CODE:
				*((u8 *)&ir_code+1) = XAUTODAT1;
				*((u8 *)&ir_code) = XAUTODAT1;
				cmd_cnt -= 2;
				break;

			case CMD_REG_WRITE:
				IOD = MODE_REGW | (IOD & addr_mask) | XAUTODAT1;
				IFCONFIG = 0xE0;	//48MHz IFCLK & PORTB enable
				OEB = 0xff;			//PORTB all output
				IOB = XAUTODAT1;
				cmd_cnt -= 2;
				break;

			case CMD_REG_READ:
				IOD = MODE_REGR | (IOD & addr_mask) | XAUTODAT1;
				IFCONFIG = 0xE0;	//48MHz IFCLK & PORTB enable
				OEB = 0x00;			//PORTB all input
				XAUTODAT2=IOB;
				ret_cnt++;
				cmd_cnt -= 1;
				break;

			case CMD_PORT_WRITE:
				IOD = (IOD & ~addr_mask) | XAUTODAT1;
				cmd_cnt -= 1;
				break;

			case CMD_PORT_READ:
				XAUTODAT2 = IOD;
				ret_cnt++;
				break;

			case CMD_PORT_CFG:
				addr_mask = (XAUTODAT1 | 0xc0);
				OED = XAUTODAT1 | addr_mask;
				addr_mask = ~addr_mask;
				cmd_cnt -= 2;
				break;

			case CMD_IFCONFIG:
				val_ifconfig = XAUTODAT1;
				cmd_cnt -= 1;
				break;

			case CMD_EP6IN_START:
				IFCONFIG = val_ifconfig;

				FIFORESET = 0x06;	// reset, FIFO 6
				SYNCDELAY;

				EP6FIFOCFG = 0x0C;	//8bit, Auto-IN
				SYNCDELAY;

				IOD = MODE_START | (IOD & 0x3f);
				break;

			case CMD_EP6IN_STOP:
				EP6FIFOCFG = 0x04;	//8bit
				break;

			case CMD_EP4IN_START:
				IFCONFIG = val_ifconfig;

				FIFORESET = 0x04;	// reset, FIFO 4
				SYNCDELAY;

				EP4FIFOCFG = 0x0C;	//8bit, Auto-IN
				SYNCDELAY;

				IOD = MODE_START | (IOD & 0x3f);
				break;

			case CMD_EP4IN_STOP:
				EP4FIFOCFG = 0x04;	//8bit
				break;

			case CMD_EP2OUT_START:
				IFCONFIG = val_ifconfig;

				FIFORESET = 0x02;	// reset, FIFO 2
				SYNCDELAY;

				OUTPKTEND = 0x82;
				SYNCDELAY;
				OUTPKTEND = 0x82;
				SYNCDELAY;
				OUTPKTEND = 0x82;
				SYNCDELAY;
				OUTPKTEND = 0x82;
				SYNCDELAY;
				EP2FIFOCFG = 0x10;	//8bit, Auto-OUT
				SYNCDELAY;

				IOD = MODE_START | (IOD & 0x3f);
				break;

			case CMD_EP2OUT_STOP:
				EP2FIFOCFG = 0x04;	//8bit
				break;

			case CMD_MODE_IDLE:
				IOD = MODE_IDLE | (IOD & 0x3f);
				break;
			}
		}
		EP1OUTBC = 0;			//arm EP1OUT
		if (ret_cnt) {
			EP1INBC = ret_cnt;
		}
	}
}

#ifdef SDCC
#define BOOL BYTE
#endif

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
	return(TRUE);
}

BOOL TD_Resume(void)          // Called after the device resumes
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------

BOOL DR_GetDescriptor(void)
{
	return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{
	Configuration = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
	EP0BUF[0] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
	AlternateSetting = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
	EP0BUF[0] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
	return(TRUE);
}

BOOL DR_ClearFeature(void)
{
	return(TRUE);
}

BOOL DR_SetFeature(void)
{
	return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
	BYTE tmp;
  
	switch (SETUPDAT[1])
		{
		case VR_NAKALL_ON:
			tmp = FIFORESET;
			tmp |= bmNAKALL;      
			SYNCDELAY;                    
			FIFORESET = tmp;
			break;
		case VR_NAKALL_OFF:
			tmp = FIFORESET;
			tmp &= ~bmNAKALL;      
			SYNCDELAY;                    
			FIFORESET = tmp;
			break;
		default:
			return(TRUE);
		}

	return(FALSE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 13
#endif
{
	GotSUD = TRUE;            // Set flag
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

void ISR_Sof(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 14
#endif
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSOF;            // Clear SOF IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 15
#endif
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

void ISR_Susp(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 16
#endif
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUSP;
}

void ISR_Ures(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 17
#endif
{
	// whenever we get a USB reset, we should revert to full speed mode
	pConfigDscr = pFullSpeedConfigDscr;
	((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
	pOtherConfigDscr = pHighSpeedConfigDscr;
	((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Highspeed(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 18
#endif
{
	if (EZUSB_HIGHSPEED())
		{
			pConfigDscr = pHighSpeedConfigDscr;
			((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
			pOtherConfigDscr = pFullSpeedConfigDscr;
			((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
		}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmHSGRANT;
}

void ISR_Ep0ack(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 19
#endif
{
}

void ISR_Stub(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 20
#endif
{
}
void ISR_Ep0in(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 21
#endif
{
}
void ISR_Ep0out(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 22
#endif
{
}
void ISR_Ep1in(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 23
#endif
{
}
void ISR_Ep1out(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 24
#endif
{
}
void ISR_Ep2inout(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 25
#endif
{
}
void ISR_Ep4inout(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 26
#endif
{
}
void ISR_Ep6inout(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 27
#endif
{
}
void ISR_Ep8inout(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 28
#endif
{
}
void ISR_Ibn(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 29
#endif
{
}
void ISR_Ep0pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 30
#endif
{
}
void ISR_Ep1pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 31
#endif
{
}
void ISR_Ep2pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 32
#endif
{
}
void ISR_Ep4pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 33
#endif
{
}
void ISR_Ep6pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 34
#endif
{
}
void ISR_Ep8pingnak(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 35
#endif
{
}

void ISR_Errorlimit(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 36
#endif
{
}
void ISR_Ep2piderror(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 37
#endif
{
}
void ISR_Ep4piderror(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 38
#endif
{
}
void ISR_Ep6piderror(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 39
#endif
{
}
void ISR_Ep8piderror(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 40
#endif
{
}

void ISR_Ep2pflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 41
#endif
{
}
void ISR_Ep4pflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 42
#endif
{
}
void ISR_Ep6pflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 43
#endif
{
}
void ISR_Ep8pflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 44
#endif
{
}
void ISR_Ep2eflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 45
#endif
{
}
void ISR_Ep4eflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 46
#endif
{
}
void ISR_Ep6eflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 47
#endif
{
}
void ISR_Ep8eflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 48
#endif
{
}
void ISR_Ep2fflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 49
#endif
{
}
void ISR_Ep4fflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 50
#endif
{
}
void ISR_Ep6fflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 51
#endif
{
}
void ISR_Ep8fflag(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 52
#endif
{
}
void ISR_GpifComplete(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 53
#endif
{
}
void ISR_GpifWaveform(void)
#ifndef SDCC
	interrupt 0
#else
	interrupt 54
#endif
{
}
