//-----------------------------------------------------------------------------
//   File:      FX2regs.h
//   Contents:   EZ-USB FX2 register declarations and bit mask definitions.
//
// $Archive: /USB/Target/Inc/fx2regs.h $
// $Date: 2006/03/06 17:26:34 $
// $Revision: 1.3 $
//
//
//   Copyright (c) 2000 Cypress Semiconductor, All rights reserved
//-----------------------------------------------------------------------------

#ifndef FX2REGS_H   /* Header Sentry */
#define FX2REGS_H

//-----------------------------------------------------------------------------
// FX2 Related Register Assignments
//-----------------------------------------------------------------------------

// The Ez-USB FX2 registers are defined here. We use FX2regs.h for register 
// address allocation by using "#define ALLOCATE_EXTERN". 
// When using "#define ALLOCATE_EXTERN", you get (for instance): 
// xdata volatile BYTE OUT7BUF[64]   _at_   0x7B40;
// Such lines are created from FX2.h by using the preprocessor. 
// Incidently, these lines will not generate any space in the resulting hex 
// file; they just bind the symbols to the addresses for compilation. 
// You just need to put "#define ALLOCATE_EXTERN" in your main program file; 
// i.e. fw.c or a stand-alone C source file. 
// Without "#define ALLOCATE_EXTERN", you just get the external reference: 
// extern xdata volatile BYTE OUT7BUF[64]   ;//   0x7B40;
// This uses the concatenation operator "##" to insert a comment "//" 
// to cut off the end of the line, "_at_   0x7B40;", which is not wanted.

// For SDCC
#define NEWEZREG(_name,_where,_size) volatile xdata at _where _size _name

NEWEZREG(GPIF_WAVE_DATA,0xE400,BYTE);
NEWEZREG(RES_WAVEDATA_END,0xE480,BYTE);

// General Configuration

NEWEZREG(CPUCS,0xE600,BYTE);  // Control & Status
NEWEZREG(IFCONFIG,0xE601,BYTE);  // Interface Configuration
NEWEZREG(PINFLAGSAB,0xE602,BYTE);  // FIFO FLAGA and FLAGB Assignments
NEWEZREG(PINFLAGSCD,0xE603,BYTE);  // FIFO FLAGC and FLAGD Assignments
NEWEZREG(FIFORESET,0xE604,BYTE);  // Restore FIFOS to default state
NEWEZREG(BREAKPT,0xE605,BYTE);  // Breakpoint
NEWEZREG(BPADDRH,0xE606,BYTE);  // Breakpoint Address H
NEWEZREG(BPADDRL,0xE607,BYTE);  // Breakpoint Address L
NEWEZREG(UART230,0xE608,BYTE);  // 230 Kbaud clock for T0,T1,T2
NEWEZREG(FIFOPINPOLAR,0xE609,BYTE);  // FIFO polarities
NEWEZREG(REVID,0xE60A,BYTE);  // Chip Revision
NEWEZREG(REVCTL,0xE60B,BYTE);  // Chip Revision Control

// Endpoint Configuration

NEWEZREG(EP1OUTCFG,0xE610,BYTE);  // Endpoint 1-OUT Configuration
NEWEZREG(EP1INCFG,0xE611,BYTE);  // Endpoint 1-IN Configuration
NEWEZREG(EP2CFG,0xE612,BYTE);  // Endpoint 2 Configuration
NEWEZREG(EP4CFG,0xE613,BYTE);  // Endpoint 4 Configuration
NEWEZREG(EP6CFG,0xE614,BYTE);  // Endpoint 6 Configuration
NEWEZREG(EP8CFG,0xE615,BYTE);  // Endpoint 8 Configuration
NEWEZREG(EP2FIFOCFG,0xE618,BYTE);  // Endpoint 2 FIFO configuration
NEWEZREG(EP4FIFOCFG,0xE619,BYTE);  // Endpoint 4 FIFO configuration
NEWEZREG(EP6FIFOCFG,0xE61A,BYTE);  // Endpoint 6 FIFO configuration
NEWEZREG(EP8FIFOCFG,0xE61B,BYTE);  // Endpoint 8 FIFO configuration
NEWEZREG(EP2AUTOINLENH,0xE620,BYTE);  // Endpoint 2 Packet Length H (IN only)
NEWEZREG(EP2AUTOINLENL,0xE621,BYTE);  // Endpoint 2 Packet Length L (IN only)
NEWEZREG(EP4AUTOINLENH,0xE622,BYTE);  // Endpoint 4 Packet Length H (IN only)
NEWEZREG(EP4AUTOINLENL,0xE623,BYTE);  // Endpoint 4 Packet Length L (IN only)
NEWEZREG(EP6AUTOINLENH,0xE624,BYTE);  // Endpoint 6 Packet Length H (IN only)
NEWEZREG(EP6AUTOINLENL,0xE625,BYTE);  // Endpoint 6 Packet Length L (IN only)
NEWEZREG(EP8AUTOINLENH,0xE626,BYTE);  // Endpoint 8 Packet Length H (IN only)
NEWEZREG(EP8AUTOINLENL,0xE627,BYTE);  // Endpoint 8 Packet Length L (IN only)
NEWEZREG(EP2FIFOPFH,0xE630,BYTE);  // EP2 Programmable Flag trigger H
NEWEZREG(EP2FIFOPFL,0xE631,BYTE);  // EP2 Programmable Flag trigger L
NEWEZREG(EP4FIFOPFH,0xE632,BYTE);  // EP4 Programmable Flag trigger H
NEWEZREG(EP4FIFOPFL,0xE633,BYTE);  // EP4 Programmable Flag trigger L
NEWEZREG(EP6FIFOPFH,0xE634,BYTE);  // EP6 Programmable Flag trigger H
NEWEZREG(EP6FIFOPFL,0xE635,BYTE);  // EP6 Programmable Flag trigger L
NEWEZREG(EP8FIFOPFH,0xE636,BYTE);  // EP8 Programmable Flag trigger H
NEWEZREG(EP8FIFOPFL,0xE637,BYTE);  // EP8 Programmable Flag trigger L
NEWEZREG(EP2ISOINPKTS,0xE640,BYTE);  // EP2 (if ISO) IN Packets per frame (1-3)
NEWEZREG(EP4ISOINPKTS,0xE641,BYTE);  // EP4 (if ISO) IN Packets per frame (1-3)
NEWEZREG(EP6ISOINPKTS,0xE642,BYTE);  // EP6 (if ISO) IN Packets per frame (1-3)
NEWEZREG(EP8ISOINPKTS,0xE643,BYTE);  // EP8 (if ISO) IN Packets per frame (1-3)
NEWEZREG(INPKTEND,0xE648,BYTE);  // Force IN Packet End
NEWEZREG(OUTPKTEND,0xE649,BYTE);  // Force OUT Packet End

// Interrupts

NEWEZREG(EP2FIFOIE,0xE650,BYTE);  // Endpoint 2 Flag Interrupt Enable
NEWEZREG(EP2FIFOIRQ,0xE651,BYTE);  // Endpoint 2 Flag Interrupt Request
NEWEZREG(EP4FIFOIE,0xE652,BYTE);  // Endpoint 4 Flag Interrupt Enable
NEWEZREG(EP4FIFOIRQ,0xE653,BYTE);  // Endpoint 4 Flag Interrupt Request
NEWEZREG(EP6FIFOIE,0xE654,BYTE);  // Endpoint 6 Flag Interrupt Enable
NEWEZREG(EP6FIFOIRQ,0xE655,BYTE);  // Endpoint 6 Flag Interrupt Request
NEWEZREG(EP8FIFOIE,0xE656,BYTE);  // Endpoint 8 Flag Interrupt Enable
NEWEZREG(EP8FIFOIRQ,0xE657,BYTE);  // Endpoint 8 Flag Interrupt Request
NEWEZREG(IBNIE,0xE658,BYTE);  // IN-BULK-NAK Interrupt Enable
NEWEZREG(IBNIRQ,0xE659,BYTE);  // IN-BULK-NAK interrupt Request
NEWEZREG(NAKIE,0xE65A,BYTE);  // Endpoint Ping NAK interrupt Enable
NEWEZREG(NAKIRQ,0xE65B,BYTE);  // Endpoint Ping NAK interrupt Request
NEWEZREG(USBIE,0xE65C,BYTE);  // USB Int Enables
NEWEZREG(USBIRQ,0xE65D,BYTE);  // USB Interrupt Requests
NEWEZREG(EPIE,0xE65E,BYTE);  // Endpoint Interrupt Enables
NEWEZREG(EPIRQ,0xE65F,BYTE);  // Endpoint Interrupt Requests
NEWEZREG(GPIFIE,0xE660,BYTE);  // GPIF Interrupt Enable
NEWEZREG(GPIFIRQ,0xE661,BYTE);  // GPIF Interrupt Request
NEWEZREG(USBERRIE,0xE662,BYTE);  // USB Error Interrupt Enables
NEWEZREG(USBERRIRQ,0xE663,BYTE);  // USB Error Interrupt Requests
NEWEZREG(ERRCNTLIM,0xE664,BYTE);  // USB Error counter and limit
NEWEZREG(CLRERRCNT,0xE665,BYTE);  // Clear Error Counter EC[3..0]
NEWEZREG(INT2IVEC,0xE666,BYTE);  // Interupt 2 (USB) Autovector
NEWEZREG(INT4IVEC,0xE667,BYTE);  // Interupt 4 (FIFOS & GPIF) Autovector
NEWEZREG(INTSETUP,0xE668,BYTE);  // Interrupt 2&4 Setup

// Input/Output

NEWEZREG(PORTACFG,0xE670,BYTE);  // I/O PORTA Alternate Configuration
NEWEZREG(PORTCCFG,0xE671,BYTE);  // I/O PORTC Alternate Configuration
NEWEZREG(PORTECFG,0xE672,BYTE);  // I/O PORTE Alternate Configuration
NEWEZREG(I2CS,0xE678,BYTE);  // Control & Status
NEWEZREG(I2DAT,0xE679,BYTE);  // Data
NEWEZREG(I2CTL,0xE67A,BYTE);  // I2C Control
NEWEZREG(XAUTODAT1,0xE67B,BYTE);  // Autoptr1 MOVX access
NEWEZREG(XAUTODAT2,0xE67C,BYTE);  // Autoptr2 MOVX access

#define EXTAUTODAT1 XAUTODAT1
#define EXTAUTODAT2 XAUTODAT2

// USB Control

NEWEZREG(USBCS,0xE680,BYTE);  // USB Control & Status
NEWEZREG(SUSPEND,0xE681,BYTE);  // Put chip into suspend
NEWEZREG(WAKEUPCS,0xE682,BYTE);  // Wakeup source and polarity
NEWEZREG(TOGCTL,0xE683,BYTE);  // Toggle Control
NEWEZREG(USBFRAMEH,0xE684,BYTE);  // USB Frame count H
NEWEZREG(USBFRAMEL,0xE685,BYTE);  // USB Frame count L
NEWEZREG(MICROFRAME,0xE686,BYTE);  // Microframe count, 0-7
NEWEZREG(FNADDR,0xE687,BYTE);  // USB Function address

// Endpoints

NEWEZREG(EP0BCH,0xE68A,BYTE);  // Endpoint 0 Byte Count H
NEWEZREG(EP0BCL,0xE68B,BYTE);  // Endpoint 0 Byte Count L
NEWEZREG(EP1OUTBC,0xE68D,BYTE);  // Endpoint 1 OUT Byte Count
NEWEZREG(EP1INBC,0xE68F,BYTE);  // Endpoint 1 IN Byte Count
NEWEZREG(EP2BCH,0xE690,BYTE);  // Endpoint 2 Byte Count H
NEWEZREG(EP2BCL,0xE691,BYTE);  // Endpoint 2 Byte Count L
NEWEZREG(EP4BCH,0xE694,BYTE);  // Endpoint 4 Byte Count H
NEWEZREG(EP4BCL,0xE695,BYTE);  // Endpoint 4 Byte Count L
NEWEZREG(EP6BCH,0xE698,BYTE);  // Endpoint 6 Byte Count H
NEWEZREG(EP6BCL,0xE699,BYTE);  // Endpoint 6 Byte Count L
NEWEZREG(EP8BCH,0xE69C,BYTE);  // Endpoint 8 Byte Count H
NEWEZREG(EP8BCL,0xE69D,BYTE);  // Endpoint 8 Byte Count L
NEWEZREG(EP0CS,0xE6A0,BYTE);  // Endpoint  Control and Status
NEWEZREG(EP1OUTCS,0xE6A1,BYTE);  // Endpoint 1 OUT Control and Status
NEWEZREG(EP1INCS,0xE6A2,BYTE);  // Endpoint 1 IN Control and Status
NEWEZREG(EP2CS,0xE6A3,BYTE);  // Endpoint 2 Control and Status
NEWEZREG(EP4CS,0xE6A4,BYTE);  // Endpoint 4 Control and Status
NEWEZREG(EP6CS,0xE6A5,BYTE);  // Endpoint 6 Control and Status
NEWEZREG(EP8CS,0xE6A6,BYTE);  // Endpoint 8 Control and Status
NEWEZREG(EP2FIFOFLGS,0xE6A7,BYTE);  // Endpoint 2 Flags
NEWEZREG(EP4FIFOFLGS,0xE6A8,BYTE);  // Endpoint 4 Flags
NEWEZREG(EP6FIFOFLGS,0xE6A9,BYTE);  // Endpoint 6 Flags
NEWEZREG(EP8FIFOFLGS,0xE6AA,BYTE);  // Endpoint 8 Flags
NEWEZREG(EP2FIFOBCH,0xE6AB,BYTE);  // EP2 FIFO total byte count H
NEWEZREG(EP2FIFOBCL,0xE6AC,BYTE);  // EP2 FIFO total byte count L
NEWEZREG(EP4FIFOBCH,0xE6AD,BYTE);  // EP4 FIFO total byte count H
NEWEZREG(EP4FIFOBCL,0xE6AE,BYTE);  // EP4 FIFO total byte count L
NEWEZREG(EP6FIFOBCH,0xE6AF,BYTE);  // EP6 FIFO total byte count H
NEWEZREG(EP6FIFOBCL,0xE6B0,BYTE);  // EP6 FIFO total byte count L
NEWEZREG(EP8FIFOBCH,0xE6B1,BYTE);  // EP8 FIFO total byte count H
NEWEZREG(EP8FIFOBCL,0xE6B2,BYTE);  // EP8 FIFO total byte count L
NEWEZREG(SUDPTRH,0xE6B3,BYTE);  // Setup Data Pointer high address byte
NEWEZREG(SUDPTRL,0xE6B4,BYTE);  // Setup Data Pointer low address byte
NEWEZREG(SUDPTRCTL,0xE6B5,BYTE);  // Setup Data Pointer Auto Mode
NEWEZREG(SETUPDAT[8],0xE6B8,BYTE);  // 8 bytes of SETUP data

// GPIF

NEWEZREG(GPIFWFSELECT,0xE6C0,BYTE);  // Waveform Selector
NEWEZREG(GPIFIDLECS,0xE6C1,BYTE);  // GPIF Done, GPIF IDLE drive mode
NEWEZREG(GPIFIDLECTL,0xE6C2,BYTE);  // Inactive Bus, CTL states
NEWEZREG(GPIFCTLCFG,0xE6C3,BYTE);  // CTL OUT pin drive
NEWEZREG(GPIFADRH,0xE6C4,BYTE);  // GPIF Address H
NEWEZREG(GPIFADRL,0xE6C5,BYTE);  // GPIF Address L

NEWEZREG(GPIFTCB3,0xE6CE,BYTE);  // GPIF Transaction Count Byte 3
NEWEZREG(GPIFTCB2,0xE6CF,BYTE);  // GPIF Transaction Count Byte 2
NEWEZREG(GPIFTCB1,0xE6D0,BYTE);  // GPIF Transaction Count Byte 1
NEWEZREG(GPIFTCB0,0xE6D1,BYTE);  // GPIF Transaction Count Byte 0

#define EP2GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP2GPIFTCL GPIFTCB0   // before REVE silicon (ie. REVB and REVD)
#define EP4GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP4GPIFTCL GPIFTCB0   // before REVE silicon (ie. REVB and REVD)
#define EP6GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP6GPIFTCL GPIFTCB0   // before REVE silicon (ie. REVB and REVD)
#define EP8GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP8GPIFTCL GPIFTCB0   // before REVE silicon (ie. REVB and REVD)

// NEWEZREG(EP2GPIFTCH,0xE6D0,BYTE);  // EP2 GPIF Transaction Count High
// NEWEZREG(EP2GPIFTCL,0xE6D1,BYTE);  // EP2 GPIF Transaction Count Low
NEWEZREG(EP2GPIFFLGSEL,0xE6D2,BYTE);  // EP2 GPIF Flag select
NEWEZREG(EP2GPIFPFSTOP,0xE6D3,BYTE);  // Stop GPIF EP2 transaction on prog. flag
NEWEZREG(EP2GPIFTRIG,0xE6D4,BYTE);  // EP2 FIFO Trigger
// NEWEZREG(EP4GPIFTCH,0xE6D8,BYTE);  // EP4 GPIF Transaction Count High
// NEWEZREG(EP4GPIFTCL,0xE6D9,BYTE);  // EP4 GPIF Transactionr Count Low
NEWEZREG(EP4GPIFFLGSEL,0xE6DA,BYTE);  // EP4 GPIF Flag select
NEWEZREG(EP4GPIFPFSTOP,0xE6DB,BYTE);  // Stop GPIF EP4 transaction on prog. flag
NEWEZREG(EP4GPIFTRIG,0xE6DC,BYTE);  // EP4 FIFO Trigger
// NEWEZREG(EP6GPIFTCH,0xE6E0,BYTE);  // EP6 GPIF Transaction Count High
// NEWEZREG(EP6GPIFTCL,0xE6E1,BYTE);  // EP6 GPIF Transaction Count Low
NEWEZREG(EP6GPIFFLGSEL,0xE6E2,BYTE);  // EP6 GPIF Flag select
NEWEZREG(EP6GPIFPFSTOP,0xE6E3,BYTE);  // Stop GPIF EP6 transaction on prog. flag
NEWEZREG(EP6GPIFTRIG,0xE6E4,BYTE);  // EP6 FIFO Trigger
// NEWEZREG(EP8GPIFTCH,0xE6E8,BYTE);  // EP8 GPIF Transaction Count High
// NEWEZREG(EP8GPIFTCL,0xE6E9,BYTE);  // EP8GPIF Transaction Count Low
NEWEZREG(EP8GPIFFLGSEL,0xE6EA,BYTE);  // EP8 GPIF Flag select
NEWEZREG(EP8GPIFPFSTOP,0xE6EB,BYTE);  // Stop GPIF EP8 transaction on prog. flag
NEWEZREG(EP8GPIFTRIG,0xE6EC,BYTE);  // EP8 FIFO Trigger
NEWEZREG(XGPIFSGLDATH,0xE6F0,BYTE);  // GPIF Data H (16-bit mode only)
NEWEZREG(XGPIFSGLDATLX,0xE6F1,BYTE);  // Read/Write GPIF Data L & trigger transac
NEWEZREG(XGPIFSGLDATLNOX,0xE6F2,BYTE);  // Read GPIF Data L, no transac trigger
NEWEZREG(GPIFREADYCFG,0xE6F3,BYTE);  // Internal RDY,Sync/Async, RDY5CFG
NEWEZREG(GPIFREADYSTAT,0xE6F4,BYTE);  // RDY pin states
NEWEZREG(GPIFABORT,0xE6F5,BYTE);  // Abort GPIF cycles

// UDMA

NEWEZREG(FLOWSTATE, 0xE6C6,BYTE); //Defines GPIF flow state
NEWEZREG(FLOWLOGIC, 0xE6C7,BYTE); //Defines flow/hold decision criteria
NEWEZREG(FLOWEQ0CTL, 0xE6C8,BYTE); //CTL states during active flow state
NEWEZREG(FLOWEQ1CTL, 0xE6C9,BYTE); //CTL states during hold flow state
NEWEZREG(FLOWHOLDOFF, 0xE6CA,BYTE);
NEWEZREG(FLOWSTB, 0xE6CB,BYTE); //CTL/RDY Signal to use as master data strobe 
NEWEZREG(FLOWSTBEDGE, 0xE6CC,BYTE); //Defines active master strobe edge
NEWEZREG(FLOWSTBHPERIOD, 0xE6CD,BYTE); //Half Period of output master strobe
NEWEZREG(GPIFHOLDAMOUNT, 0xE60C,BYTE); //Data delay shift 
NEWEZREG(UDMACRCH, 0xE67D,BYTE); //CRC Upper byte
NEWEZREG(UDMACRCL, 0xE67E,BYTE); //CRC Lower byte
NEWEZREG(UDMACRCQUAL, 0xE67F,BYTE); //UDMA In only, host terminated use only


// Debug/Test

NEWEZREG(DBUG,0xE6F8,BYTE);  // Debug
NEWEZREG(TESTCFG,0xE6F9,BYTE);  // Test configuration
NEWEZREG(USBTEST,0xE6FA,BYTE);  // USB Test Modes
NEWEZREG(CT1,0xE6FB,BYTE);  // Chirp Test--Override
NEWEZREG(CT2,0xE6FC,BYTE);  // Chirp Test--FSM
NEWEZREG(CT3,0xE6FD,BYTE);  // Chirp Test--Control Signals
NEWEZREG(CT4,0xE6FE,BYTE);  // Chirp Test--Inputs

// Endpoint Buffers

NEWEZREG(EP0BUF[64],0xE740,BYTE);  // EP0 IN-OUT buffer
NEWEZREG(EP1OUTBUF[64],0xE780,BYTE);  // EP1-OUT buffer
NEWEZREG(EP1INBUF[64],0xE7C0,BYTE);  // EP1-IN buffer
NEWEZREG(EP2FIFOBUF[1024],0xF000,BYTE);  // 512/1024-byte EP2 buffer (IN or OUT)
NEWEZREG(EP4FIFOBUF[1024],0xF400,BYTE);  // 512 byte EP4 buffer (IN or OUT)
NEWEZREG(EP6FIFOBUF[1024],0xF800,BYTE);  // 512/1024-byte EP6 buffer (IN or OUT)
NEWEZREG(EP8FIFOBUF[1024],0xFC00,BYTE);  // 512 byte EP8 buffer (IN or OUT)


/*-----------------------------------------------------------------------------
   Special Function Registers (SFRs)
   The byte registers and bits defined in the following list are based
   on the Synopsis definition of the 8051 Special Function Registers for EZ-USB. 
    If you modify the register definitions below, please regenerate the file 
    "ezregs.inc" which includes the same basic information for assembly inclusion.
-----------------------------------------------------------------------------*/

sfr at 0x80 IOA;
sfr at 0x81 SP;
sfr at 0x82 DPL;
sfr at 0x83 DPH;
sfr at 0x84 DPL1;
sfr at 0x85 DPH1;
sfr at 0x86 DPS;
         /*  DPS  */
         sbit at 0x86+0 SEL;
sfr at 0x87 PCON;   /*  PCON  */
         //sbit at 0x87+0 IDLE;
         //sbit at 0x87+1 STOP;
         //sbit at 0x87+2 GF0;
         //sbit at 0x87+3 GF1;
         //sbit at 0x87+7 SMOD0;
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
sfr at 0x8F SPC_FNC; // Was WRS in Reg320
         /*  CKCON  */
         //sbit at 0x8F+0 WRS;
sfr at 0x90 IOB;
sfr at 0x91 EXIF; // EXIF Bit Values differ from Reg320
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

sfr at 0x9A APTR1H; // old name
sfr at 0x9B APTR1L; // old name
sfr at 0x9A AUTOPTR1H;
sfr at 0x9B AUTOPTR1L;
sfr at 0x9D AUTOPTRH2;
sfr at 0x9E AUTOPTRL2; 
sfr at 0xA0 IOC;
sfr at 0xA1 INT2CLR;
sfr at 0xA2 INT4CLR;

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

sfr at 0xAA EP2468STAT;
         /* EP2468STAT */
         //sbit at EP2E 0xAA+0;
         //sbit at EP2F 0xAA+1;
         //sbit at EP4E 0xAA+2;
         //sbit at EP4F 0xAA+3;
         //sbit at EP6E 0xAA+4;
         //sbit at EP6F 0xAA+5;
         //sbit at EP8E 0xAA+6;
         //sbit at EP8F 0xAA+7;

sfr at 0xAB EP24FIFOFLGS;
sfr at 0xAC EP68FIFOFLGS;
sfr at 0xAF AUTOPTRSETUP;
            /* AUTOPTRSETUP */
            sbit at 0xAF+0 EXTACC;
            sbit at 0xAF+1 APTR1FZ;
            sbit at 0xAF+2 APTR2FZ;

sfr at 0xB0 IOD;
sfr at 0xB1 IOE;
sfr at 0xB2 OEA;
sfr at 0xB3 OEB;
sfr at 0xB4 OEC;
sfr at 0xB5 OED;
sfr at 0xB6 OEE;

sfr at 0xB8 IP;
         /*  IP  */
         sbit at 0xB8+0 PX0;
         sbit at 0xB8+1 PT0;
         sbit at 0xB8+2 PX1;
         sbit at 0xB8+3 PT1;
         sbit at 0xB8+4 PS0;
         sbit at 0xB8+5 PT2;
         sbit at 0xB8+6 PS1;

sfr at 0xBA EP01STAT;
sfr at 0xBB GPIFTRIG;
                
sfr at 0xBD GPIFSGLDATH;
sfr at 0xBE GPIFSGLDATLX;
sfr at 0xBF GPIFSGLDATLNOX;

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
sfr at 0xD8 EICON; // Was WDCON in DS80C320; Bit Values differ from Reg320
         /*  EICON  */
         sbit at 0xD8+3 INT6;
         sbit at 0xD8+4 RESI;
         sbit at 0xD8+5 ERESI;
         sbit at 0xD8+7 SMOD1;
sfr at 0xE0 ACC;
sfr at 0xE8 EIE; // EIE Bit Values differ from Reg320
                        /*  EIE  */
         sbit at 0xE8+0 EUSB;
         sbit at 0xE8+1 EI2C;
         sbit at 0xE8+2 EIEX4;
         sbit at 0xE8+3 EIEX5;
         sbit at 0xE8+4 EIEX6;
sfr at 0xF0 B;
sfr at 0xF8 EIP; // EIP Bit Values differ from Reg320
                        /*  EIP  */
         sbit at 0xF8+0 PUSB;
         sbit at 0xF8+1 PI2C;
         sbit at 0xF8+2 EIPX4;
         sbit at 0xF8+3 EIPX5;
         sbit at 0xF8+4 EIPX6;

/*-----------------------------------------------------------------------------
   Bit Masks
-----------------------------------------------------------------------------*/

/* CPU Control & Status Register (CPUCS) */
#define bmPRTCSTB    bmBIT5
#define bmCLKSPD     (bmBIT4 | bmBIT3)
#define bmCLKSPD1    bmBIT4
#define bmCLKSPD0    bmBIT3
#define bmCLKINV     bmBIT2
#define bmCLKOE      bmBIT1
#define bm8051RES    bmBIT0
/* Port Alternate Configuration Registers */
/* Port A (PORTACFG) */
#define bmFLAGD      bmBIT7
#define bmINT1       bmBIT1
#define bmINT0       bmBIT0
/* Port C (PORTCCFG) */
#define bmGPIFA7     bmBIT7
#define bmGPIFA6     bmBIT6
#define bmGPIFA5     bmBIT5
#define bmGPIFA4     bmBIT4
#define bmGPIFA3     bmBIT3
#define bmGPIFA2     bmBIT2
#define bmGPIFA1     bmBIT1
#define bmGPIFA0     bmBIT0
/* Port E (PORTECFG) */
#define bmGPIFA8     bmBIT7
#define bmT2EX       bmBIT6
#define bmINT6       bmBIT5
#define bmRXD1OUT    bmBIT4
#define bmRXD0OUT    bmBIT3
#define bmT2OUT      bmBIT2
#define bmT1OUT      bmBIT1
#define bmT0OUT      bmBIT0

/* I2C Control & Status Register (I2CS) */
#define bmSTART      bmBIT7
#define bmSTOP       bmBIT6
#define bmLASTRD     bmBIT5
#define bmID         (bmBIT4 | bmBIT3)
#define bmBERR       bmBIT2
#define bmACK        bmBIT1
#define bmDONE       bmBIT0
/* I2C Control Register (I2CTL) */
#define bmSTOPIE     bmBIT1
#define bm400KHZ     bmBIT0
/* Interrupt 2 (USB) Autovector Register (INT2IVEC) */
#define bmIV4        bmBIT6
#define bmIV3        bmBIT5
#define bmIV2        bmBIT4
#define bmIV1        bmBIT3
#define bmIV0        bmBIT2
/* USB Interrupt Request & Enable Registers (USBIE/USBIRQ) */
#define bmEP0ACK     bmBIT6
#define bmHSGRANT    bmBIT5
#define bmURES       bmBIT4
#define bmSUSP       bmBIT3
#define bmSUTOK      bmBIT2
#define bmSOF        bmBIT1
#define bmSUDAV      bmBIT0
/* Breakpoint register (BREAKPT) */
#define bmBREAK      bmBIT3
#define bmBPPULSE    bmBIT2
#define bmBPEN       bmBIT1
/* Interrupt 2 & 4 Setup (INTSETUP) */
#define bmAV2EN      bmBIT3
#define INT4IN       bmBIT1
#define bmAV4EN      bmBIT0
/* USB Control & Status Register (USBCS) */
#define bmHSM        bmBIT7
#define bmDISCON     bmBIT3
#define bmNOSYNSOF   bmBIT2
#define bmRENUM      bmBIT1
#define bmSIGRESUME  bmBIT0
/* Wakeup Control and Status Register (WAKEUPCS) */
#define bmWU2        bmBIT7
#define bmWU         bmBIT6
#define bmWU2POL     bmBIT5
#define bmWUPOL      bmBIT4
#define bmDPEN       bmBIT2
#define bmWU2EN      bmBIT1
#define bmWUEN       bmBIT0
/* End Point 0 Control & Status Register (EP0CS) */
#define bmHSNAK      bmBIT7
/* End Point 0-1 Control & Status Registers (EP0CS/EP1OUTCS/EP1INCS) */
#define bmEPBUSY     bmBIT1
#define bmEPSTALL    bmBIT0
/* End Point 2-8 Control & Status Registers (EP2CS/EP4CS/EP6CS/EP8CS) */
#define bmNPAK       (bmBIT6 | bmBIT5 | bmBIT4)
#define bmEPFULL     bmBIT3
#define bmEPEMPTY    bmBIT2
/* Endpoint Status (EP2468STAT) SFR bits */
#define bmEP8FULL    bmBIT7
#define bmEP8EMPTY   bmBIT6
#define bmEP6FULL    bmBIT5
#define bmEP6EMPTY   bmBIT4
#define bmEP4FULL    bmBIT3
#define bmEP4EMPTY   bmBIT2
#define bmEP2FULL    bmBIT1
#define bmEP2EMPTY   bmBIT0
/* SETUP Data Pointer Auto Mode (SUDPTRCTL) */
#define bmSDPAUTO    bmBIT0
/* Endpoint Data Toggle Control (TOGCTL) */
#define bmQUERYTOGGLE  bmBIT7
#define bmSETTOGGLE    bmBIT6
#define bmRESETTOGGLE  bmBIT5
#define bmTOGCTLEPMASK bmBIT3 | bmBIT2 | bmBIT1 | bmBIT0
/* IBN (In Bulk Nak) enable and request bits (IBNIE/IBNIRQ) */
#define bmEP8IBN     bmBIT5
#define bmEP6IBN     bmBIT4
#define bmEP4IBN     bmBIT3
#define bmEP2IBN     bmBIT2
#define bmEP1IBN     bmBIT1
#define bmEP0IBN     bmBIT0

/* PING-NAK enable and request bits (NAKIE/NAKIRQ) */
#define bmEP8PING     bmBIT7
#define bmEP6PING     bmBIT6
#define bmEP4PING     bmBIT5
#define bmEP2PING     bmBIT4
#define bmEP1PING     bmBIT3
#define bmEP0PING     bmBIT2
#define bmIBN         bmBIT0

/* Interface Configuration bits (IFCONFIG) */
#define bmIFCLKSRC    bmBIT7
#define bm3048MHZ     bmBIT6
#define bmIFCLKOE     bmBIT5
#define bmIFCLKPOL    bmBIT4
#define bmASYNC       bmBIT3
#define bmGSTATE      bmBIT2
#define bmIFCFG1      bmBIT1
#define bmIFCFG0      bmBIT0
#define bmIFCFGMASK   (bmIFCFG0 | bmIFCFG1)
#define bmIFGPIF      bmIFCFG1

/* EP 2468 FIFO Configuration bits (EP2FIFOCFG,EP4FIFOCFG,EP6FIFOCFG,EP8FIFOCFG) */
#define bmINFM       bmBIT6
#define bmOEP        bmBIT5
#define bmAUTOOUT    bmBIT4
#define bmAUTOIN     bmBIT3
#define bmZEROLENIN  bmBIT2
#define bmWORDWIDE   bmBIT0

/* Chip Revision Control Bits (REVCTL) - used to ebable/disable revision specidic
   features */ 
#define bmNOAUTOARM    bmBIT1
#define bmSKIPCOMMIT   bmBIT0

/* Fifo Reset bits (FIFORESET) */
#define bmNAKALL       bmBIT7

#endif   /* FX2REGS_H */
