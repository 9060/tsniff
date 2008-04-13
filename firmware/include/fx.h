//-----------------------------------------------------------------------------
//	File:		Fx.h
//	Contents:	EZ-USB register declarations and bit mask definitions.
//
//	Copyright (c) 1999 Cypress Semiconductor, All rights reserved
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Fx Related Register Assignments
//-----------------------------------------------------------------------------

#ifdef ALLOCATE_EXTERN
#define NEWEZREG(_name,_where,_size) volatile xdata at _where _size _name
#else                           /*  */
#define NEWEZREG(_name,_where,_size) extern volatile xdata _size _name
#endif                          /*  */

NEWEZREG(AINDATA,0x7800,BYTE); // FIFO A Read Data
NEWEZREG(AINBC,0x7801,BYTE); // FIFO A In Byte Count
NEWEZREG(AINPF,0x7802,BYTE); // FIFO A In Programmable Flag
NEWEZREG(AINPFPIN,0x7803,BYTE); // FIFO A In Pin Progr Flag
NEWEZREG(BINDATA,0x7805,BYTE); // FIFO B Read Data
NEWEZREG(BINBC,0x7806,BYTE); // FIFO B In Byte Count
NEWEZREG(BINPF,0x7807,BYTE); // FIFO B In Programmable Flag
NEWEZREG(BINPFPIN,0x7808,BYTE); // FIFO B In Pin Progr Flag

NEWEZREG(ABINTF,0x780A,BYTE); // Input FIFO A/B Flags & Toggle
NEWEZREG(ABINIE,0x780B,BYTE); // Input FIFO A/B Int Enables
#define bmAINPF  0x20
#define bmAINEF  0x10
#define bmAINFF  0x08
#define bmBINPF  0x04
#define bmBINEF  0x02
#define bmBINFF  0x01

NEWEZREG(ABINIRQ,0x780C,BYTE); // Input FIFO A/B Int Requests

NEWEZREG(AOUTDATA,0x780E,BYTE); // FIFO A Out Write Data
NEWEZREG(AOUTBC,0x780F,BYTE); // FIFO A Out Byte Count
NEWEZREG(AOUTPF,0x7810,BYTE); // FIFO A Out Programmable Flag
NEWEZREG(AOUTPFPIN,0x7811,BYTE); // FIFO A Out Pin Progr Flag
NEWEZREG(BOUTDATA,0x7813,BYTE); // FIFO B Out Write Data
NEWEZREG(BOUTBC,0x7814,BYTE); // FIFO B Out Byte Count
NEWEZREG(BOUTPF,0x7815,BYTE); // FIFO B Out Programmable Flag
NEWEZREG(BOUTPFPIN,0x7816,BYTE); // FIFO B Out Pin Progr Flag

NEWEZREG(ABOUTCS,0x7818,BYTE); // Output FIFO A/B Flags & Toggle
NEWEZREG(ABOUTIE,0x7819,BYTE); // Output FIFO A/B Int Enables
NEWEZREG(ABOUTIRQ,0x781A,BYTE); // Output FIFO A/B Int Requests
#define bmAOUTPF 0x20
#define bmAOUTEF 0x10
#define bmAOUTFF 0x08
#define bmBOUTPF 0x04
#define bmBOUTEF 0x02
#define bmBOUTFF 0x01


NEWEZREG(ABSETUP,0x781C,BYTE); // FIFO A/B Setup
NEWEZREG(ABPOLAR,0x781D,BYTE); // FIFO signal polarities
NEWEZREG(ABFLUSH,0x781E,BYTE); // Reset all FIFO Flags


NEWEZREG(WFSELECT,0x7824,BYTE);
NEWEZREG(IDLE_CS,0x7825,BYTE);
NEWEZREG(IDLE_CTLOUT,0x7826,BYTE);
NEWEZREG(CTLOUTCFG,0x7827,BYTE);
NEWEZREG(GPIFADRL,0x782A,BYTE);
NEWEZREG(GPIFADRH,0x782B,BYTE);
NEWEZREG(AINTC,0x782C,BYTE);
NEWEZREG(AOUTTC,0x782D,BYTE);
NEWEZREG(ATRIG,0x782E,BYTE);
NEWEZREG(BINTC,0x7830,BYTE);
NEWEZREG(BOUTTC,0x7831,BYTE);
NEWEZREG(BTRIG,0x7832,BYTE);
NEWEZREG(SGLDATH,0x7834,BYTE);
NEWEZREG(SGLDATLTRIG,0x7835,BYTE);
NEWEZREG(SGLDATLNTRIG,0x7836,BYTE);
NEWEZREG(READY,0x7838,BYTE);
NEWEZREG(ABORT,0x7839,BYTE);
NEWEZREG(GENIE,0x783B,BYTE);
// for both GENIE and GENIRQ
#define bmDMADONE   bmBIT2 
#define bmGPIFWF   bmBIT1
#define bmGPIFDONE   bmBIT0

NEWEZREG(GENIRQ,0x783C,BYTE);
NEWEZREG(OUTD,0x7841,BYTE);
NEWEZREG(PINSD,0x7842,BYTE);
NEWEZREG(OED,0x7843,BYTE);
NEWEZREG(OUTE,0x7845,BYTE);
NEWEZREG(PINSE,0x7846,BYTE);
NEWEZREG(OEE,0x7847,BYTE);
NEWEZREG(PORTSETUP,0x7849,BYTE);

NEWEZREG(IFCONFIG,0x784A,BYTE); // Interface Configuration
        // bmBIT0 IF0
        // bmBIT1 IF1
        // bmBIT2 BUS16
        // bmBIT3 GSTATE
        // bmBIT7 52One
NEWEZREG(PORTACF2,0x784B,BYTE); // PORTA Alternate Config #2
        // bmBIT4 SLWR
        // bmBIT5 SLRD
NEWEZREG(PORTCCF2,0x784C,BYTE); // PORTC Alternate Config #2
        // bmBIT0 RDY0
        // bmBIT1 RDY1
        // bmBIT3 RDY3
        // bmBIT4 CTRL1
        // bmBIT5 CTRL3
        // bmBIT6 CTRL4
        // bmBIT7 CTRL5

NEWEZREG(DMASRC,0x784F,WORD);
//NEWEZREG(DMASRCH,0x784F,BYTE); // DMA Source Address (H)
//NEWEZREG(DMASRCL,0x7850,BYTE); // DMA Source Address (L)
NEWEZREG(DMADEST,0x7851,WORD);
//NEWEZREG(DMADESTH,0x7851,BYTE); // DMA Destination Address (H)
//NEWEZREG(DMADESTL,0x7852,BYTE); // DMA Destination Address (L)
NEWEZREG(DMALEN,0x7854,BYTE); // DMA Transfer Length
NEWEZREG(DMAGO,0x7855,BYTE); // DMA Start and Status

NEWEZREG(DMABURST,0x7857,BYTE); // DMA Burst Mode; for ext DMA transfers
        // bmBIT0 BW
        // bmBIT1 BR
        // bmBIT2 DSTR0
        // bmBIT3 DSTR1
        // bmBIT4 DSTR2
NEWEZREG(DMAEXTFIFO,0x7858,BYTE);
NEWEZREG(INT4IVEC,0x785D,BYTE); // Interrupt 4 AutoVector
NEWEZREG(INT4SETUP,0x785E,BYTE); // Interrupt 4 Setup
#define  INT4SFC     bmBIT2 
#define  INT4_INTERNAL  bmBIT1
#define  INT4_AV4EN    bmBIT0

// New bits in USBBAV
#define  INT2SFC     bmBIT4
NEWEZREG(WFDESC[0x80],0x7900,BYTE);

NEWEZREG(I2CMODE,0x7FA7,BYTE);  // STOPIE and 400KHZ bits
NEWEZREG(IBNIRQ,0x7FB0,BYTE);   // IN-bulk-NAK IRQ
NEWEZREG(IBNIEN,0x7FB1,BYTE);         // IN-bulk-NAK Int enable

//NEWEZREG(FASTXFR,0x7FE2,BYTE); // Fast XFer; for external DMA transfers
        // bmBIT0 WMOD0
        // bmBIT1 WMOD1
        // bmBIT2 WPOL
        // bmBIT3 RMOD0
        // bmBIT4 RMOD1
        // bmBIT5 RPOL
        // bmBIT6 FBLK
        // bmBIT7 FISO
                                                                
// The TUFT provides 8 memory mapped control registers to configure the FIFO tests
//NEWEZREG(TUFT_SEL,0x7900,BYTE); // TUFT Register Addr Select
//NEWEZREG(TUFT_SEL,0x8000,BYTE); // TUFT Register Addr Select
//NEWEZREG(TUFT_CONF_A,0x8001,BYTE); // Configure FIFO A test
//        // bmBIT0 FA_ENA_COUNT
//        // bmBIT1 FA_CONTINUOUS
//        // bmBIT2 FA_ENA_RELOAD
//        // bmBIT3 FA_DIR
//        // bmBIT4 FA_HANDSHAKE
//        // bmBIT5 FA_EAGER
//NEWEZREG(TUFT_CONF_B,0x8002,BYTE); // Configure FIFO B test
//        // bmBIT0 FA_ENA_COUNT
//        // bmBIT1 FA_CONTINUOUS
//        // bmBIT2 FA_ENA_RELOAD
//        // bmBIT3 FA_DIR
//        // bmBIT4 FA_HANDSHAKE
//        // bmBIT5 FA_EAGER
//NEWEZREG(TUFT_MODE,0x8003,BYTE); // Strobe, FIFO test Modes
//        // bmBIT 1:0 STROBE_ACTIVE
//        // bmBIT 2:3 STROBE_INACTIVE
//        // bmBIT4 16_BIT_FIFO_MODE 
//               // bmBIT5 16_BIT_BUS_MODE 
//NEWEZREG(COUNT_DOWN_A,0x8004,BYTE); // PreLoad Count Down Value
//NEWEZREG(COUNT_DOWN_B,0x8005,BYTE); // PreLoad Count Down Value
//NEWEZREG(COUNT_WRAP,0x8006,BYTE); // PreLoad Wrap Count Value

//NEWEZREG(CPUCS,0x7F92,BYTE); // TBD: I2C Mode
 #define CPUCS_48MHZ  bmBIT3    
 #define CPUCS_CLKINV bmBIT2
//NEWEZREG(CONFIG0,0x7FA6,BYTE); // TBD: Config0
        // bmBIT0 400KHZ
//NEWEZREG(I2CMODE,0x7FA7,BYTE); // TBD: I2C Mode
        // bmBIT0 400KHZ
        // bmBIT1 STOPIE
                                
                                
// Note:  These registers will not work until
// bit 0 in PORTSETUP is set.
sfr at 0x80 IOA;
sfr at 0x90 IOB;
sfr at 0xA0 IOC;
sfr at 0xB0 IOD;
sfr at 0xB1 IOE;

sfr at 0xB2 SOEA;
sfr at 0xB3 SOEB;
sfr at 0xB4 SOEC;
sfr at 0xB5 SOED;
sfr at 0xB6 SOEE;

// Note -- Requires bit 4 in USBBAV to be set!
sfr at 0xA1 INT2CLR;

// Note -- Requires bit 2 in INT4SETUP to be set!
sfr at 0xA2 INT4CLR;

#ifdef AGGRESSIVELY_USE_TNG_FEATURES
// Note these features will not work unless they are enabled via the 
// USBBAV, INT4SETUP and PORTSETUP are initialized to enable them.
 #define OUTA IOA
 #define OUTB IOB
 #define OUTC IOC
 #define OUTD IOD
 #define OUTE IOE
 #define PINSA IOA
 #define PINSB IOB
 #define PINSC IOC
 #define PINSD IOD
 #define PINSE IOE

 #define OEA SOEA
 #define OEB SOEB
 #define OEC SOEC
 #define OED SOED
 #define OEE SOEE

 // Globally change USBIRQ clearing to INT2CLR clearing
 #undef USBIRQ
 #define USBIRQ INT2CLR
#endif

