/* tic55x bottom support definitions
 *
 * This file is part of 0cpm Firmerware.
 *
 * 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
 *
 * 0cpm Firmerware is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * 0cpm Firmerware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.
 */


/* Below:
 *  - definitions for this architecture to service the top half
 *  - definitions to bind bottom half files together
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* The timer is the top 32-bit half of timer0 */
typedef uint32_t timing_t;
#define TIME_BEFORE(x,y)	(((x)-(y)) >> 31)
#define TIME_BEFOREQ(x,y)	(!(TIME_BEFORE((y),(x))))

#define TIMER_NULL 0

#define TIME_MSEC(x)	(((uint32_t) (x)))
#define TIME_SEC(x)	(((uint32_t) (x))*1000)
#define TIME_MIN(x)	(((uint32_t) (x))*1000*60)
#define TIME_HOUR(x)	(((uint32_t) (x))*1000*60*60)
#define TIME_DAY(x)	(((uint32_t) (x))*1000*60*60*24)


/* Critical region definitions */
#define bottom_critical_region_begin() _disable_interrupts()
#define bottom_critical_region_end()    _enable_interrupts()

/* Mapping data from/to network format */
#define htons(x) (x)
#define htonl(x) (x)
#define ntohs(x) (x)
#define ntohl(x) (x)

/* Misc utility functions */
#define bzero(p,n) memset((p),0,(n))


/* Following definitions are only available if BOTTOM is defined */


#ifdef BOTTOM

/* Interrupt Enable Registers (memory-mapped) */
extern volatile uint16_t IER0, IER1;
asm ("_IER0 .set 0x0000");
asm ("_IER1 .set 0x0045");
extern volatile uint16_t IFR0, IFR1;
asm ("_IFR0 .set 0x0001");
asm ("_IFR1 .set 0x0046");

#define REGBIT_IER0_TINT0	4
#define REGBIT_IER0_DMAC1	9
#define REGBIT_IER0_INT0	2
#define REGBIT_IER1_TINT1	6
#define REGBIT_IER1_DMAC0	2

extern volatile uint16_t IVPD, IVPH;
asm ("_IVPD .set 0x0049");
asm ("_IVPH .set 0x004a");

extern volatile bool tic55x_top_has_been_interrupted;

void setup_interrupts (void);

/* General Purpose I/O registers */
extern volatile uint16_t ioport IODIR, IODATA;
asm ("_IODIR .set 0x3400");
asm ("_IODATA .set 0x3401");

/* EMIF and CE space control registers */
extern volatile uint16_t ioport EGCR1, EGCR2;
extern volatile uint16_t ioport CE0_1, CE0_2, CE1_1, CE1_2, CE2_1, CE2_2, CE3_1, CE3_2;
extern volatile uint16_t ioport CE0_SEC1, CE0_SEC2, CE1_SEC1, CE1_SEC2,
                                CE2_SEC1, CE2_SEC2, CE3_SEC1, CE3_SEC2;
asm ("_EGCR1 .set 0x0800");
asm ("_EGCR2 .set 0x0801");
asm ("_CE1_1 .set 0x0802");
asm ("_CE1_2 .set 0x0803");
asm ("_CE0_1 .set 0x0804");
asm ("_CE0_2 .set 0x0805");
asm ("_CE2_1 .set 0x0808");
asm ("_CE2_2 .set 0x0809");
asm ("_CE3_1 .set 0x080a");
asm ("_CE3_2 .set 0x080b");
asm ("_CE1_SEC1 .set 0x0822");
asm ("_CE1_SEC2 .set 0x0823");
asm ("_CE0_SEC1 .set 0x0824");
asm ("_CE0_SEC2 .set 0x0825");
asm ("_CE2_SEC1 .set 0x0828");
asm ("_CE2_SEC2 .set 0x0829");
asm ("_CE3_SEC1 .set 0x082a");
asm ("_CE3_SEC2 .set 0x082b");

/* EMIF SDRAM control registers */
extern volatile uint16_t ioport SDC1, SDC2, SDRC1, SDRC2, SDX1, SDX2, CESCR1, CESCR2;
asm ("_SDC1 .set 0x080c");
asm ("_SDC2 .set 0x080d");
asm ("_SDRC1 .set 0x080e");
asm ("_SDRC2 .set 0x080f");
asm ("_SDX1 .set 0x0810");
asm ("_SDX2 .set 0x0811");
asm ("_CESCR1 .set 0x0840");
asm ("_CESCR2 .set 0x0841");

/* Pin Control Registers for McBSP0, McBSP1 */
extern volatile uint16_t ioport PCR0, PCR1;
asm ("_PCR0 .set 0x2812");
asm ("_PCR1 .set 0x2c12");

#define REGBIT_PCR_IDLEEN	14
#define REGBIT_PCR_XIOEN	13
#define REGBIT_PCR_RIOEN	12
#define REGBIT_PCR_FSXM		11
#define REGBIT_PCR_FSRM		10
#define REGBIT_PCR_CLKXM	9
#define REGBIT_PCR_CLKRM	8
#define REGBIT_PCR_SCLKME	7
#define REGBIT_PCR_CLKSSTAT	6
#define REGBIT_PCR_DXSTAT	5
#define REGBIT_PCR_DRSTAT	4
#define REGBIT_PCR_FSXP		3
#define REGBIT_PCR_FSRP		2
#define REGBIT_PCR_CLKXP	1
#define REGBIT_PCR_CLKRP	0

#define REGVAL_PCR_CLKRP	0x01
#define REGVAL_PCR_CLKXP	0x02
#define REGVAL_PCR_FSRP		0x04
#define REGVAL_PCR_FSXP		0x08
#define REGVAL_PCR_DRSTAT	0x10

/* Timer0/1 configuration registers */
extern volatile uint16_t ioport GPTCLK_0;
extern volatile uint16_t ioport GPTCNT1_0, GPTCNT2_0, GPTCNT3_0, GPTCNT4_0;
extern volatile uint16_t ioport GPTPRD1_0, GPTPRD2_0, GPTPRD3_0, GPTPRD4_0;
extern volatile uint16_t ioport GPTCTL1_0, GPTCTL2_0, GPTGCTL1_0;
asm ("_GPTCLK_0 .set 0x1003");
asm ("_GPTCNT1_0 .set 0x1008");
asm ("_GPTCNT2_0 .set 0x1009");
asm ("_GPTCNT3_0 .set 0x100a");
asm ("_GPTCNT4_0 .set 0x100b");
asm ("_GPTPRD1_0 .set 0x100c");
asm ("_GPTPRD2_0 .set 0x100d");
asm ("_GPTPRD3_0 .set 0x100e");
asm ("_GPTPRD4_0 .set 0x100f");
asm ("_GPTCTL1_0 .set 0x1010");
asm ("_GPTCTL2_0 .set 0x1011");
asm ("_GPTGCTL1_0 .set 0x1012");

extern volatile uint16_t ioport GPTCLK_1;
extern volatile uint16_t ioport GPTCNT1_1, GPTCNT2_1, GPTCNT3_1, GPTCNT4_1;
extern volatile uint16_t ioport GPTPRD1_1, GPTPRD2_1, GPTPRD3_1, GPTPRD4_1;
extern volatile uint16_t ioport GPTCTL1_1, GPTCTL2_1, GPTGCTL1_1;
asm ("_GPTCLK_1 .set 0x2403");
asm ("_GPTCNT1_1 .set 0x2408");
asm ("_GPTCNT2_1 .set 0x2409");
asm ("_GPTCNT3_1 .set 0x240a");
asm ("_GPTCNT4_1 .set 0x240b");
asm ("_GPTPRD1_1 .set 0x240c");
asm ("_GPTPRD2_1 .set 0x240d");
asm ("_GPTPRD3_1 .set 0x240e");
asm ("_GPTPRD4_1 .set 0x240f");
asm ("_GPTCTL1_1 .set 0x2410");
asm ("_GPTCTL2_1 .set 0x2411");
asm ("_GPTGCTL1_1 .set 0x2412");

#define REGVAL_GCTL_TIMMODE_DUAL32CHAINED 0x0c
#define REGVAL_GCTL_TIM12RS 0x01
#define REGVAL_GCTL_TIM34RS 0x02

#define REGVAL_CTL12_ENAMODE_MASK	0xc0
#define REGVAL_CTL12_ENAMODE_ONETIME	0x40
#define REGVAL_CTL12_ENAMODE_CONTINUOUS	0x80
#define REGVAL_CTL12_CP			0x08
#define REGVAL_CTL12_TSTAT		0x01


/* Idle mode configuration parameters */
extern volatile uint16_t ioport ICR, PICR;
asm ("_ICR .set 0x0001");
asm ("_PICR .set 0x9400");

#define REGBIT_ICR_CLKEI 9
#define REGBIT_ICR_IPORTI 8
#define REGBIT_ICR_MPORTI 7
#define REGBIT_ICR_XPORTI 6
#define REGBIT_ICR_EMIFI 5
#define REGBIT_ICR_CLKI 4
#define REGBIT_ICR_PERI 3
#define REGBIT_ICR_ICACHEI 2
#define REGBIT_ICR_MPI 1
#define REGBIT_ICR_CPUI 0

#define REGBIT_PICR_MISC 13
#define REGBIT_PICR_EMIF 12
#define REGBIT_PICR_BIOST 11
#define REGBIT_PICR_WDT 10
#define REGBIT_PICR_PIO 9
#define REGBIT_PICR_URT 8
#define REGBIT_PICR_I2C 7
#define REGBIT_PICR_ID 6
#define REGBIT_PICR_IO 6
#define REGBIT_PICR_SP1 3
#define REGBIT_PICR_SP0 2
#define REGBIT_PICR_TIM1 1
#define REGBIT_PICR_TIM0 0

/* PLL configuration registers */
extern volatile uint16_t ioport PLLCSR, PLLM, PLLDIV0, PLLDIV1, PLLDIV2, PLLDIV3;
asm ("_PLLCSR .set 0x1c80");
asm ("_PLLM .set 0x1c88");
asm ("_PLLDIV0 .set 0x1c8a");
asm ("_PLLDIV1 .set 0x1c8c");
asm ("_PLLDIV2 .set 0x1c8e");
asm ("_PLLDIV3 .set 0x1c90");

#define REGVAL_PLLM_TIMES_2		0x0002
#define REGVAL_PLLM_TIMES_3		0x0003
#define REGVAL_PLLM_TIMES_4		0x0004
#define REGVAL_PLLM_TIMES_5		0x0005
#define REGVAL_PLLM_TIMES_6		0x0006
#define REGVAL_PLLM_TIMES_7		0x0007
#define REGVAL_PLLM_TIMES_8		0x0008
#define REGVAL_PLLM_TIMES_9		0x0009
#define REGVAL_PLLM_TIMES_10		0x000a
#define REGVAL_PLLM_TIMES_11		0x000b
#define REGVAL_PLLM_TIMES_12		0x000c
#define REGVAL_PLLM_TIMES_13		0x000d
#define REGVAL_PLLM_TIMES_14		0x000e
#define REGVAL_PLLM_TIMES_15		0x000f

#define REGVAL_PLLCSR_PLLEN		0x0001
#define REGVAL_PLLCSR_PLLRST		0x0008
#define REGVAL_PLLCSR_LOCK		0x0020
#define REGVAL_PLLCSR_STABLE		0x0040

#define REGVAL_PLLDIVx_DxEN		0x8000
#define REGVAL_PLLDIVx_PLLDIVx_1	0x0000
#define REGVAL_PLLDIVx_PLLDIVx_2	0x0001
#define REGVAL_PLLDIVx_PLLDIVx_4	0x0003

/* McBSP1 configuration registers */
extern volatile uint16_t ioport SPCR1_1, SPCR2_1, SRGR1_1, SRGR2_1;
extern volatile uint16_t ioport MCR1_1, MCR2_1;
extern volatile uint16_t ioport RCR1_1, RCR2_1, XCR1_1, XCR2_1;
extern volatile uint16_t ioport DXR1_1, DRR1_1;
asm ("_SPCR1_1 .set 0x2c04");
asm ("_SPCR2_1 .set 0x2c05");
asm ("_RCR1_1 .set 0x2c06");
asm ("_RCR2_1 .set 0x2c07");
asm ("_XCR1_1 .set 0x2c08");
asm ("_XCR2_1 .set 0x2c09");
asm ("_SRGR1_1 .set 0x2c0a");
asm ("_SRGR2_1 .set 0x2c0b");
asm ("_MCR1_1 .set 0x2c0c");
asm ("_MCR2_1 .set 0x2c0d");
asm ("_DXR1_1 .set 0x2c02");
asm ("_DRR1_1 .set 0x2c02");

#define REGVAL_SPCR1_CLKSTP_WITHDELAY	0x1800
#define REGVAL_SPCR1_RRST_NOTRESET	0x0001
#define REGVAL_SPCR2_FRST_NOTRESET	0x0080
#define REGVAL_SPCR2_GRST_NOTRESET	0x0040
#define REGVAL_SPCR2_XRST_NOTRESET	0x0001

#define REGVAL_SRGR1_FWID_1		0x0100
#define REGVAL_SRGR1_CLKGDIV_1		0x0001
#define REGVAL_SRGR1_CLKGDIV_4		0x0004
#define REGVAL_SRGR1_CLKGDIV_5		0x0005
#define REGVAL_SRGR1_CLKGDIV_15		0x000f
#define REGVAL_SRGR1_CLKGDIV_30		0x001e
#define REGVAL_SRGR2_GSYNC		0x8000
#define REGVAL_SRGR2_CLKSP		0x4000
#define REGVAL_SRGR2_CLKSM		0x2000
#define REGVAL_SRGR2_FSGM		0x1000
#define REGVAL_SRGR2_FPER_255		255
#define REGVAL_SRGR2_FPER_511		511
#define REGVAL_SRGR2_FPER_999		999
#define REGVAL_SRGR2_FPER_1535		1535

/* I2C configuration registers */

extern volatile uint16_t ioport I2CPSC, I2CCLKL, I2CCLKH, I2COAR;
extern volatile uint16_t ioport I2CMDR, I2CSTR, I2CIER, I2CISRC;
extern volatile uint16_t ioport I2CSAR, I2CCNT, I2CDXR, I2CDRR;

asm ("_I2CPSC .set 0x3c0c");
asm ("_I2CCLKL .set 0x3c03");
asm ("_I2CCLKH .set 0x3c04");
asm ("_I2COAR .set 0x3c00");
asm ("_I2CMDR .set 0x3c09");
asm ("_I2CSTR .set 0x3c02");
asm ("_I2CIER .set 0x3c01");
asm ("_I2CISRC .set 0x3c0a");
asm ("_I2CSAR .set 0x3c07");
asm ("_I2CCNT .set 0x3c05");
asm ("_I2CDXR .set 0x3c08");
asm ("_I2CDRR .set 0x3c06");

#define REGVAL_I2CMDR_NACKMOD	0x8000
#define REGVAL_I2CMDR_FREE	0x4000
#define REGVAL_I2CMDR_STT	0x2000
#define REGVAL_I2CMDR_IDLEEN	0x1000
#define REGVAL_I2CMDR_STP	0x0800
#define REGVAL_I2CMDR_MST	0x0400
#define REGVAL_I2CMDR_TRX	0x0200
#define REGVAL_I2CMDR_XA	0x0100
#define REGVAL_I2CMDR_RM	0x0080
#define REGVAL_I2CMDR_DLB	0x0040
#define REGVAL_I2CMDR_NORESET	0x0020
#define REGVAL_I2CMDR_STB	0x0010
#define REGVAL_I2CMDR_FDF	0x0008
#define REGVAL_I2CMDR_BC_8	0x0000

#define REGVAL_I2CIER_XRDY	0x0010
#define REGVAL_I2CIER_RRDY	0x0008
#define REGVAL_I2CIER_ARDY	0x0004
#define REGVAL_I2CIER_NACK	0x0002
#define REGVAL_I2CIER_AL	0x0001

#define REGVAL_I2CSTR_NACKSN	0x2000
#define REGVAL_I2CSTR_BB	0x1000
#define REGVAL_I2CSTR_RSFULL	0x0800
#define REGVAL_I2CSTR_XSMT	0x0400
#define REGVAL_I2CSTR_AAS	0x0200
#define REGVAL_I2CSTR_AD0	0x0100
#define REGVAL_I2CSTR_XRDY	0x0010
#define REGVAL_I2CSTR_RRDY	0x0008
#define REGVAL_I2CSTR_ARDY	0x0004
#define REGVAL_I2CSTR_NACK	0x0002
#define REGVAL_I2CSTR_AL	0x0001

#define REGVAL_I2COAR		0x005a

/* DMA configuration */

extern volatile uint16_t ioport DMAGCR, DMAGTCR;

extern volatile uint16_t ioport DMACSDP_0, DMACCR_0, DMACICR_0, DMACSR_0;
extern volatile uint16_t ioport DMACSSAL_0, DMACSSAU_0, DMACDSAL_0, DMACDSAU_0;
extern volatile uint16_t ioport DMACEN_0, DMACFN_0, DMACSEI_0, DMACSFI_0, DMACDEI_0, DMACDFI_0;
extern volatile uint16_t ioport DMACSAC_0, DMACDAC_0;

extern volatile uint16_t ioport DMACSDP_1, DMACCR_1, DMACICR_1, DMACSR_1;
extern volatile uint16_t ioport DMACSSAL_1, DMACSSAU_1, DMACDSAL_1, DMACDSAU_1;
extern volatile uint16_t ioport DMACEN_1, DMACFN_1, DMACSEI_1, DMACSFI_1, DMACDEI_1, DMACDFI_1;
extern volatile uint16_t ioport DMACSAC_1, DMACDAC_1;

asm ("_DMAGCR .set 0x0e00");
asm ("_DMAGTCR .set 0x0e01");

asm ("_DMACSDP_0 .set 0x0c00");
asm ("_DMACCR_0 .set 0x0c01");
asm ("_DMACICR_0 .set 0x0c02");
asm ("_DMACSR_0 .set 0x0c03");
asm ("_DMACSSAL_0 .set 0x0c04");
asm ("_DMACSSAU_0 .set 0x0c05");
asm ("_DMACDSAL_0 .set 0x0c06");
asm ("_DMACDSAU_0 .set 0x0c07");
asm ("_DMACEN_0 .set 0x0c08");
asm ("_DMACFN_0 .set 0x0c09");
asm ("_DMACSEI_0 .set 0x0c0b");
asm ("_DMACSFI_0 .set 0x0c0a");
asm ("_DMACSDEI_0 .set 0x0c0e");
asm ("_DMACSDFI_0 .set 0x0c0f");
asm ("_DMACSAC_0 .set 0x0c0c");
asm ("_DMACDAC_0 .set 0x0c0d");

asm ("_DMACSDP_1 .set 0x0c20");
asm ("_DMACCR_1 .set 0x0c21");
asm ("_DMACICR_1 .set 0x0c22");
asm ("_DMACSR_1 .set 0x0c23");
asm ("_DMACSSAL_1 .set 0x0c24");
asm ("_DMACSSAU_1 .set 0x0c25");
asm ("_DMACDSAL_1 .set 0x0c26");
asm ("_DMACDSAU_1 .set 0x0c27");
asm ("_DMACEN_1 .set 0x0c28");
asm ("_DMACFN_1 .set 0x0c29");
asm ("_DMACSEI_1 .set 0x0c2b");
asm ("_DMACSFI_1 .set 0x0c2a");
asm ("_DMACSDEI_1 .set 0x0c2e");
asm ("_DMACSDFI_1 .set 0x0c2f");
asm ("_DMACSAC_1 .set 0x0c2c");
asm ("_DMACDAC_1 .set 0x0c2d");

#define REGVAL_DMAGCR_FREE	0x0004
#define REGVAL_DMAGTCR_PTE	0x0008
#define REGVAL_DMAGTCR_ETE	0x0004
#define REGVAL_DMAGTCR_ITE1	0x0002
#define REGVAL_DMAGTCR_ITE0	0x0001
#define REGVAL_DMACCR_ENDPROG	0x0800
#define REGVAL_DMACCR_WP	0x0400
#define REGVAL_DMACCR_REPEAT	0x0200
#define REGVAL_DMACCR_AUTOINIT	0x0100
#define REGVAL_DMACCR_EN	0x0080
#define REGVAL_DMACCR_PRIO	0x0040
#define REGVAL_DMACCR_FS	0x0020
#define REGVAL_DMACCR_DSTAMODE_CONST	0x0000
#define REGVAL_DMACCR_DSTAMODE_POSTINC	0x4000
#define REGVAL_DMACCR_DSTAMODE_1INDEX	0x8000
#define REGVAL_DMACCR_DSTAMODE_2INDEX	0xc000
#define REGVAL_DMACCR_SRCAMODE_CONST	0x0000
#define REGVAL_DMACCR_SRCAMODE_POSTINC	0x1000
#define REGVAL_DMACCR_SRCAMODE_1INDEX	0x2000
#define REGVAL_DMACCR_SRCAMODE_2INDEX	0x3000
#define REGVAL_DMACCR_SYNC_NOEVENT	0x0000
#define REGVAL_DMACCR_SYNC_MCBSP0_REV	0x0001
#define REGVAL_DMACCR_SYNC_MCBSP0_TEV	0x0002
#define REGVAL_DMACCR_SYNC_MCBSP1_REV	0x0005
#define REGVAL_DMACCR_SYNC_MCBSP1_TEV	0x0006
#define REGVAL_DMACCR_SYNC_MCBSP2_REV	0x0009
#define REGVAL_DMACCR_SYNC_MCBSP2_TEV	0x000a
#define REGVAL_DMACCR_SYNC_UART_REV	0x000b
#define REGVAL_DMACCR_SYNC_UART_TEV	0x000c
#define REGVAL_DMACCR_SYNC_TIMER0	0x000d
#define REGVAL_DMACCR_SYNC_TIMER1	0x000e
#define REGVAL_DMACCR_SYNC_INT0		0x0010
#define REGVAL_DMACCR_SYNC_INT1		0x0011
#define REGVAL_DMACCR_SYNC_INT2		0x0012
#define REGVAL_DMACCR_SYNC_INT3		0x0013
#define REGVAL_DMACCR_SYNC_I2C_REV	0x0013
#define REGVAL_DMACCR_SYNC_I2C_TEV	0x0014
#define REGVAL_DMACICR_AERRIE		0x0080
#define REGVAL_DMACICR_BLOCKIE		0x0020
#define REGVAL_DMACICR_LASTIE		0x0010
#define REGVAL_DMACICR_FRAMEIE		0x0008
#define REGVAL_DMACICR_HALFIE		0x0004
#define REGVAL_DMACICR_DROPIE		0x0002
#define REGVAL_DMACICR_TIMEOUTIE	0x0001
#define REGVAL_DMACSR_AERR		0x0080
#define REGVAL_DMACSR_SYNC		0x0040
#define REGVAL_DMACSR_BLOCK		0x0020
#define REGVAL_DMACSR_LAST		0x0010
#define REGVAL_DMACSR_FRAME		0x0008
#define REGVAL_DMACSR_HALF		0x0004
#define REGVAL_DMACSR_DROP		0x0002
#define REGVAL_DMACSR_TIMEOUT		0x0001
#define REGVAL_DMACSDP_DSTBEN_DISABLE	0x0000
#define REGVAL_DMACSDP_DSTBEN_BURST	0x8000
#define REGVAL_DMACSDP_DSTPACK		0x2000
#define REGVAL_DMACSDP_DST_DARAM0	0x0000
#define REGVAL_DMACSDP_DST_DARAM1	0x0200
#define REGVAL_DMACSDP_DST_EMIF		0x0400
#define REGVAL_DMACSDP_DST_PERIPH	0x0600
#define REGVAL_DMACSDP_SRCBEN_DISABLE	0x0000
#define REGVAL_DMACSDP_SRCB EN_ENABLE	0x0100
#define REGVAL_DMACSDP_SRCPACK		0x0040
#define REGVAL_DMACSDP_SRC_DARAM0	0x0000
#define REGVAL_DMACSDP_SRC_DARAM1	0x0004
#define REGVAL_DMACSDP_SRC_EMIF		0x0008
#define REGVAL_DMACSDP_SRC_PERIPH	0x000c
#define REGVAL_DMACSDP_DATATYPE_8BIT	0x0000
#define REGVAL_DMACSDP_DATATYPE_16BIT	0x0001
#define REGVAL_DMACSDP_DATATYPE_32BIT	0x0002

#endif
