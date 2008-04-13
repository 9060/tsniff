//-----------------------------------------------------------------------------
//      File:           ezregs.h
//      Contents:       EZ-USB register declarations and bit mask definitions.
//
//      Copyright (c) 1997 AnchorChips, Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef EZREGS_H		/* Header Sentry */
#define EZREGS_H

/*-----------------------------------------------------------------------------
	Global Variables
-----------------------------------------------------------------------------*/
// The Ez-USB registers are defined here. There used to be 3 files containing
// this information: ezregs.h (external refs), ezregs.c (bound reg names to an 
// explicit address in ezusb.lib), and ezregs.inc (included in assembly files).
// We now use ezregs.h for register address allocation instead of ezregs.c by  
// using "#define ALLOCATE_EXTERN". The file ezregs.c now just includes ezregs.h.
// When using "#define ALLOCATE_EXTERN", you get: 
// xdata volatile BYTE OUT7BUF[64]      _at_    0x7B40;
// Such lines used to be in ezregs.c, but now are created from ezregs.h by using
// the preprocessor. The definitions in ezregs.c were redundant.
// Incidently, these lines will not generate any space in the resulting hex 
// file; they just bind the symbols to the addresses for compilation. 
// Since the file ezregs.c is still used in the library build, you normally
// just need to use #include "ezregs.h" in your files (i.e. fw.c).
// If you want to generate your own (non-frameworks based) C example, then you 
// just need to put "#define ALLOCATE_EXTERN" in your main program file; 
// i.e. fw.c or a stand-alone C source file. Any time you link with ezusb.lib,
// it will not be necessary to "#define ALLOCATE_EXTERN".
// Without "#define ALLOCATE_EXTERN", you just get the external reference: 
// extern xdata volatile BYTE OUT7BUF[64]       ;//     0x7B40;
// This uses the concatenation operator "##" to insert a comment "//" 
// to cut off the end of the line, "_at_        0x7B40;", which is not wanted.
// If you modify the register definitions below, please regenerate the file 
// "ezregs.inc" which uses the same basic information, but which could not be 
// derived automatically from this one source file using the preprocessor.

#ifdef ALLOCATE_EXTERN
#define NEWEZREG(_name,_where,_size) volatile xdata at _where _size _name
#else				/*  */
#define NEWEZREG(_name,_where,_size) extern volatile xdata _size _name
#endif				/*  */

/* Register Assignments 3/18/99 TPM */
NEWEZREG(OUT7BUF[64],0x7B40,BYTE);
NEWEZREG(IN7BUF[64],0x7B80,BYTE);
NEWEZREG(OUT6BUF[64],0x7BC0,BYTE);
NEWEZREG(IN6BUF[64],0x7C00,BYTE);
NEWEZREG(OUT5BUF[64],0x7C40,BYTE);
NEWEZREG(IN5BUF[64],0x7C80,BYTE);
NEWEZREG(OUT4BUF[64],0x7CC0,BYTE);
NEWEZREG(IN4BUF[64],0x7D00,BYTE);
NEWEZREG(OUT3BUF[64],0x7D40,BYTE);
NEWEZREG(IN3BUF[64],0x7D80,BYTE);
NEWEZREG(OUT2BUF[64],0x7DC0,BYTE);
NEWEZREG(IN2BUF[64],0x7E00,BYTE);
NEWEZREG(OUT1BUF[64],0x7E40,BYTE);
NEWEZREG(IN1BUF[64],0x7E80,BYTE);
NEWEZREG(OUT0BUF[64],0x7EC0,BYTE);
NEWEZREG(IN0BUF[64],0x7F00,BYTE);
NEWEZREG(OUT8DATA,0x7F60,BYTE);
NEWEZREG(OUT9DATA,0x7F61,BYTE);
NEWEZREG(OUT10DATA,0x7F62,BYTE);
NEWEZREG(OUT11DATA,0x7F63,BYTE);
NEWEZREG(OUT12DATA,0x7F64,BYTE);
NEWEZREG(OUT13DATA,0x7F65,BYTE);
NEWEZREG(OUT14DATA,0x7F66,BYTE);
NEWEZREG(OUT15DATA,0x7F67,BYTE);
NEWEZREG(IN8DATA,0x7F68,BYTE);
NEWEZREG(IN9DATA,0x7F69,BYTE);
NEWEZREG(IN10DATA,0x7F6A,BYTE);
NEWEZREG(IN11DATA,0x7F6B,BYTE);
NEWEZREG(IN12DATA,0x7F6C,BYTE);
NEWEZREG(IN13DATA,0x7F6D,BYTE);
NEWEZREG(IN14DATA,0x7F6E,BYTE);
NEWEZREG(IN15DATA,0x7F6F,BYTE);
NEWEZREG(OUT8BCH,0x7F70,BYTE);
NEWEZREG(OUT8BCL,0x7F71,BYTE);
NEWEZREG(OUT9BCH,0x7F72,BYTE);
NEWEZREG(OUT9BCL,0x7F73,BYTE);
NEWEZREG(OUT10BCH,0x7F74,BYTE);
NEWEZREG(OUT10BCL,0x7F75,BYTE);
NEWEZREG(OUT11BCH,0x7F76,BYTE);
NEWEZREG(OUT11BCL,0x7F77,BYTE);
NEWEZREG(OUT12BCH,0x7F78,BYTE);
NEWEZREG(OUT12BCL,0x7F79,BYTE);
NEWEZREG(OUT13BCH,0x7F7A,BYTE);
NEWEZREG(OUT13BCL,0x7F7B,BYTE);
NEWEZREG(OUT14BCH,0x7F7C,BYTE);
NEWEZREG(OUT14BCL,0x7F7D,BYTE);
NEWEZREG(OUT15BCH,0x7F7E,BYTE);
NEWEZREG(OUT15BCL,0x7F7F,BYTE);
NEWEZREG(CPUCS,0x7F92,BYTE);
NEWEZREG(PORTACFG,0x7F93,BYTE);
NEWEZREG(PORTBCFG,0x7F94,BYTE);
NEWEZREG(PORTCCFG,0x7F95,BYTE);
NEWEZREG(OUTA,0x7F96,BYTE);
NEWEZREG(OUTB,0x7F97,BYTE);
NEWEZREG(OUTC,0x7F98,BYTE);
NEWEZREG(PINSA,0x7F99,BYTE);
NEWEZREG(PINSB,0x7F9A,BYTE);
NEWEZREG(PINSC,0x7F9B,BYTE);
NEWEZREG(OEA,0x7F9C,BYTE);
NEWEZREG(OEB,0x7F9D,BYTE);
NEWEZREG(OEC,0x7F9E,BYTE);
NEWEZREG(UART230,0x7F9F,BYTE);
NEWEZREG(ISOERR,0x7FA0,BYTE);
NEWEZREG(ISOCTL,0x7FA1,BYTE);
NEWEZREG(ZBCOUT,0x7FA2,BYTE);
NEWEZREG(ZBCIN,0x7FA3,BYTE);
NEWEZREG(I2CS,0x7FA5,BYTE);
NEWEZREG(I2DAT,0x7FA6,BYTE);
NEWEZREG(IVEC,0x7FA8,BYTE);
NEWEZREG(IN07IRQ,0x7FA9,BYTE);
NEWEZREG(OUT07IRQ,0x7FAA,BYTE);
NEWEZREG(USBIRQ,0x7FAB,BYTE);
NEWEZREG(IN07IEN,0x7FAC,BYTE);
NEWEZREG(OUT07IEN,0x7FAD,BYTE);
NEWEZREG(USBIEN,0x7FAE,BYTE);
NEWEZREG(USBBAV,0x7FAF,BYTE);
NEWEZREG(BPADDR,0x7FB2,WORD);

//NEWEZREG(BPADDRL,0x7FB3,BYTE);
NEWEZREG(EPIO[16],0x7FB4,EPIOC);
NEWEZREG(SUDPTRH,0x7FD4,BYTE);
NEWEZREG(SUDPTRL,0x7FD5,BYTE);
NEWEZREG(USBCS,0x7FD6,BYTE);
NEWEZREG(TOGCTL,0x7FD7,BYTE);
NEWEZREG(USBFRAMEL,0x7FD8,BYTE);
NEWEZREG(USBFRAMEH,0x7FD9,BYTE);
NEWEZREG(FNADDR,0x7FDB,BYTE);
NEWEZREG(USBPAIR,0x7FDD,BYTE);
NEWEZREG(IN07VAL,0x7FDE,BYTE);
NEWEZREG(OUT07VAL,0x7FDF,BYTE);
NEWEZREG(INISOVAL,0x7FE0,BYTE);
NEWEZREG(OUTISOVAL,0x7FE1,BYTE);
NEWEZREG(FASTXFR,0x7FE2,BYTE);
NEWEZREG(AUTOPTRH,0x7FE3,BYTE);
NEWEZREG(AUTOPTRL,0x7FE4,BYTE);
NEWEZREG(AUTODATA,0x7FE5,BYTE);
NEWEZREG(SETUPDAT[8],0x7FE8,BYTE);
NEWEZREG(OUT8ADDR,0x7FF0,BYTE);
NEWEZREG(OUT9ADDR,0x7FF1,BYTE);
NEWEZREG(OUT10ADDR,0x7FF2,BYTE);
NEWEZREG(OUT11ADDR,0x7FF3,BYTE);
NEWEZREG(OUT12ADDR,0x7FF4,BYTE);
NEWEZREG(OUT13ADDR,0x7FF5,BYTE);
NEWEZREG(OUT14ADDR,0x7FF6,BYTE);
NEWEZREG(OUT15ADDR,0x7FF7,BYTE);
NEWEZREG(IN8ADDR,0x7FF8,BYTE);
NEWEZREG(IN9ADDR,0x7FF9,BYTE);
NEWEZREG(IN10ADDR,0x7FFA,BYTE);
NEWEZREG(IN11ADDR,0x7FFB,BYTE);
NEWEZREG(IN12ADDR,0x7FFC,BYTE);
NEWEZREG(IN13ADDR,0x7FFD,BYTE);
NEWEZREG(IN14ADDR,0x7FFE,BYTE);
NEWEZREG(IN15ADDR,0x7FFF,BYTE);


/*-----------------------------------------------------------------------------
	Special Function Registers (SFRs)
	The byte registers and bits defined in the following list are based
	on the Synopsis definition of the 8051 Special Function Registers for EZ-USB. 
    If you modify the register definitions below, please regenerate the file 
    "ezregs.inc" which includes the same basic information for assembly inclusion.
-----------------------------------------------------------------------------*/
sfr at 0x81 SP;
sfr at 0x82 DPL;
sfr at 0x83 DPH;
sfr at 0x84 DPL1;
sfr at 0x85 DPH1;
sfr at 0x86 DPS;

  /*  DPS  */
sbit at 0x86+0 SEL;
sfr at 0x87 PCON;		/*  PCON  */

  //sbit at 0x87+0 IDLE;
  //sbit at 0x87+1 STOP;
  //sbit at 0x87+2 GF0;
  //sbit at 0x87+3 GF1;
sbit at 0x87+7 SMOD0;
sfr at 0x88 TCON;

  /*  TCON  */
sbit at 0x88+0 IT0;  
sbit at 0x88+1 IE0;
sbit at 0x88+2 IT1;
sbit at 0x88+3 IE1;
sbit at 0x88+4 TR0;
sbit at 0x88+5 TF0;
sbit at 0x88+6 TR1;
sbit at 0x88+7 TF1;
sfr at 0x89 TMOD;

  /*  TMOD  */
  //sbit at 0x89+0 M00;
  //sbit at 0x89+1 M10;
  //sbit at 0x89+2 CT0;
  //sbit at 0x89+3 GATE0;
  //sbit at 0x89+4 M01;
  //sbit at 0x89+5 M11;
  //sbit at 0x89+6 CT1;
  //sbit at 0x89+7 GATE1;
sfr at 0x8A TL0;
sfr at 0x8B TL1;
sfr at 0x8C TH0;
sfr at 0x8D TH1;
sfr at 0x8E CKCON;

  /*  CKCON  */
  //sbit at 0x89+0 MD0;
  //sbit at 0x89+1 MD1;
  //sbit at 0x89+2 MD2;
  //sbit at 0x89+3 T0M;
  //sbit at 0x89+4 T1M;
  //sbit at 0x89+5 T2M;
sfr at 0x8F SPC_FNC;		// Was WRS in Reg320
			/*  CKCON  */
  //sbit at 0x8F+0 WRS;
sfr at 0x91 EXIF;		// EXIF Bit Values differ from Reg320
			/*  EXIF  */
  //sbit at 0x91+4 USBINT;
  //sbit at 0x91+5 I2CINT;
  //sbit at 0x91+6 IE4;
  //sbit at 0x91+7 IE5;
sfr at 0x92 MPAGE;
sfr at 0x98 SCON0;

  /*  SCON0  */
sbit at 0x98+0 RI;
sbit at 0x98+1 TI;
sbit at 0x98+2 RB8;
sbit at 0x98+3 TB8;
sbit at 0x98+4 REN;
sbit at 0x98+5 SM2;
sbit at 0x98+6 SM1;
sbit at 0x98+7 SM0;
sfr at 0x99 SBUF0;
sfr at 0xA8 IE;

  /*  IE  */
sbit at 0xA8+0 EX0;
sbit at 0xA8+1 ET0;
sbit at 0xA8+2 EX1;
sbit at 0xA8+3 ET1;
sbit at 0xA8+4 ES0;
sbit at 0xA8+5 ET2;
sbit at 0xA8+6 ES1;
sbit at 0xA8+7 EA;
sfr at 0xB8 IP;

  /*  IP  */
sbit at 0xB8+0 PX0;
sbit at 0xB8+1 PT0;
sbit at 0xB8+2 PX1;
sbit at 0xB8+3 PT1;
sbit at 0xB8+4 PS0;
sbit at 0xB8+5 PT2;
sbit at 0xB8+6 PS1;
sfr at 0xC0 SCON1;

  /*  SCON1  */
sbit at 0xC0+0 RI1;
sbit at 0xC0+1 TI1;
sbit at 0xC0+2 RB81;
sbit at 0xC0+3 TB81;
sbit at 0xC0+4 REN1;
sbit at 0xC0+5 SM21;
sbit at 0xC0+6 SM11;
sbit at 0xC0+7 SM01;
sfr at 0xC1 SBUF1;
sfr at 0xC8 T2CON;

  /*  T2CON  */
sbit at 0xC8+0 CP_RL2;
sbit at 0xC8+1 C_T2;
sbit at 0xC8+2 TR2;
sbit at 0xC8+3 EXEN2;
sbit at 0xC8+4 TCLK;
sbit at 0xC8+5 RCLK;
sbit at 0xC8+6 EXF2;
sbit at 0xC8+7 TF2;
sfr at 0xCA RCAP2L;
sfr at 0xCB RCAP2H;
sfr at 0xCC TL2;
sfr at 0xCD TH2;
sfr at 0xD0 PSW;

  /*  PSW  */
sbit at 0xD0+0 P;
sbit at 0xD0+1 FL;
sbit at 0xD0+2 OV;
sbit at 0xD0+3 RS0;
sbit at 0xD0+4 RS1;
sbit at 0xD0+5 F0;
sbit at 0xD0+6 AC;
sbit at 0xD0+7 CY;
sfr at 0xD8 EICON;		// Was WDCON in DS80C320; Bit Values differ from Reg320
			/*  EICON  */
sbit at 0xD8+3 INT6;
sbit at 0xD8+4 RESI;
sbit at 0xD8+5 ERESI;
sbit at 0xD8+7 SMOD1;
sfr at 0xE0 ACC;
sfr at 0xE8 EIE;		// EIE Bit Values differ from Reg320
			/*  EIE  */
sbit at 0xE8+0 EUSB;
sbit at 0xE8+1 EI2C;
sbit at 0xE8+2 EIEX4;
sbit at 0xE8+3 EIEX5;
sbit at 0xE8+4 EIEX6;
sfr at 0xF0 B;
sfr at 0xF8 EIP;		// EIP Bit Values differ from Reg320
			/*  EIP  */
sbit at 0xF8+0 PUSB;
sbit at 0xF8+1 PI2C;
sbit at 0xF8+2 EIPX4;
sbit at 0xF8+3 EIPX5;
sbit at 0xF8+4 EIPX6;

/*-----------------------------------------------------------------------------
	Bit Masks
-----------------------------------------------------------------------------*/

/* CPU Control & Status Register */
#define bmCHIPREV		(bmBIT7 | bmBIT6 | bmBIT5 | bmBIT4)
#define bmCLK24OE		bmBIT1
#define bm8052RES		bmBIT0
/* Port Configuration Registers */
/* Port A */
#define bmRXD1OUT		bmBIT7
#define bmRXD0OUT		bmBIT6
#define bmFRD			bmBIT5
#define bmFWR			bmBIT4
#define bmCS			bmBIT3
#define bmOE			bmBIT2
#define bmT1OUT			bmBIT1
#define bmT0OUT			bmBIT0
/* Port B */
#define bmT2OUT			bmBIT7
#define bmINT6			bmBIT6
#define bmINT5			bmBIT5
#define bmINT4			bmBIT4
#define bmTXD1			bmBIT3
#define bmRXD1			bmBIT2
#define bmT2EX			bmBIT1
#define bmT2			bmBIT0
/* Port C */
#define bmRD			bmBIT7
#define bmWR			bmBIT6
#define bmT1			bmBIT5
#define bmT0			bmBIT4
#define bmINT1			bmBIT3
#define bmINT0			bmBIT2
#define bmTXD0			bmBIT1
#define bmRXD0			bmBIT0
/* Isochronous Status & End Point Valid Registers */
#define bmEP15			bmBIT7
#define bmEP14			bmBIT6
#define bmEP13			bmBIT5
#define bmEP12			bmBIT4
#define bmEP11			bmBIT3
#define bmEP10			bmBIT2
#define bmEP9			bmBIT1
#define bmEP8			bmBIT0
/* I2C Control & Status Register */
#define bmSTART			bmBIT7
#define bmSTOP			bmBIT6
#define bmLASTRD		bmBIT5
#define bmID			(bmBIT4 | bmBIT3)
#define bmBERR			bmBIT2
#define bmACK			bmBIT1
#define bmDONE			bmBIT0
/* Interrupt Vector Register */
#define bmIV4			bmBIT6
#define bmIV3			bmBIT5
#define bmIV2			bmBIT4
#define bmIV1			bmBIT3
#define bmIV0			bmBIT2
/* End point Interrupt Request, End Point Interrupt Enable */
/* And End Point Valid Registers */
#define bmEP7			bmBIT7
#define bmEP6			bmBIT6
#define bmEP5			bmBIT5
#define bmEP4			bmBIT4
#define bmEP3			bmBIT3
#define bmEP2			bmBIT2
#define bmEP1			bmBIT1
#define bmEP0			bmBIT0
/* Global Interrupt Request & Enable Registers */
#define bmIBN        bmBIT5
#define bmURES			bmBIT4
#define bmSUSP			bmBIT3
#define bmSUTOK			bmBIT2
#define bmSOF			bmBIT1
#define bmSUDAV			bmBIT0
/* Global Control */
#define bmBREAK			bmBIT3
#define bmBPPULSE		bmBIT2
#define bmBPEN			bmBIT1
#define bmAVEN			bmBIT0
/* USB Control & Status Register */
#define bmRWAKEUP		bmBIT7
#define bmDISCON		bmBIT3
#define bmDISCOE		bmBIT2
#define bmRENUM			bmBIT1
#define bmSIGRESUME		bmBIT0
/* End Point 0 Control & Status Register */
#define bmOUT			bmBIT3
#define bmIN			bmBIT2
#define bmHS			bmBIT1
#define bmHSSTALL		bmBIT0
/* End Point Control & Status Registers */
#define bmEPSTALL		bmBIT0
#define bmEPBUSY		bmBIT1
/* Fast Transfer Register */
#define bmFISO			bmBIT7
#define bmFBLK			bmBIT6
#define bmRPOL			bmBIT5
#define bmRMOD1			bmBIT4
#define bmRMOD0			bmBIT3
#define bmWPOL			bmBIT2
#define bmWMOD1			bmBIT1
#define bmWMOD0			bmBIT0
/* Endpoint Pairing Register */
#define bmISOSEND0		bmBIT7
#define bmPR6OUT		bmBIT5
#define bmPR4OUT		bmBIT4
#define bmPR2OUT		bmBIT3
#define bmPR6IN			bmBIT2
#define bmPR4IN			bmBIT1
#define bmPR2IN			bmBIT0
/* End point control offsets */
enum { IN0BUF_ID =
	0, IN1BUF_ID, IN2BUF_ID, IN3BUF_ID, IN4BUF_ID, IN5BUF_ID,
	IN6BUF_ID,
    IN7BUF_ID, OUT0BUF_ID, OUT1BUF_ID, OUT2BUF_ID, OUT3BUF_ID,
    OUT4BUF_ID, OUT5BUF_ID, OUT6BUF_ID, OUT7BUF_ID
};

#define EP0CS	EPIO[0].cntrl
#define IN0BC	EPIO[0].bytes
#define IN1CS	EPIO[1].cntrl
#define IN1BC	EPIO[1].bytes
#define IN2CS	EPIO[2].cntrl
#define IN2BC	EPIO[2].bytes
#define IN3CS	EPIO[3].cntrl
#define IN3BC	EPIO[3].bytes
#define IN4CS	EPIO[4].cntrl
#define IN4BC	EPIO[4].bytes
#define IN5CS	EPIO[5].cntrl
#define IN5BC	EPIO[5].bytes
#define IN6CS	EPIO[6].cntrl
#define IN6BC	EPIO[6].bytes
#define IN7CS	EPIO[7].cntrl
#define IN7BC	EPIO[7].bytes
#define OUT0CS	EPIO[8].cntrl
#define OUT0BC	EPIO[8].bytes
#define OUT1CS	EPIO[9].cntrl
#define OUT1BC	EPIO[9].bytes
#define OUT2CS	EPIO[10].cntrl
#define OUT2BC	EPIO[10].bytes
#define OUT3CS	EPIO[11].cntrl
#define OUT3BC	EPIO[11].bytes
#define OUT4CS	EPIO[12].cntrl
#define OUT4BC	EPIO[12].bytes
#define OUT5CS	EPIO[13].cntrl
#define OUT5BC	EPIO[13].bytes
#define OUT6CS	EPIO[14].cntrl
#define OUT6BC	EPIO[14].bytes
#define OUT7CS	EPIO[15].cntrl
#define OUT7BC	EPIO[15].bytes

/*-----------------------------------------------------------------------------
	Macros
-----------------------------------------------------------------------------*/
/* Convert End point ID (d0000eee) to EPIO offset */
#define EPID(id)		(((~id & 0x80) >> 4) + (id & 0x07))

#endif				/* EZREGS_H */
