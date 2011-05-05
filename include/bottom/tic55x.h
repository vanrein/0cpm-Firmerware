/*
 * tic55x bottom support definitions
 *
 * Below:
 *  - definitions for this architecture to service the top half
 *  - definitions to bind bottom half files together
 *
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* The timer is the top 32-bit half of timer0 */
typedef uint32_t timing_t;
#define TIME_BEFORE(x,y)	(((x)-(y)) >> 31)

#define TIMER_NULL 0

#define TIME_MSEC(x)	((x))
#define TIME_SEC(x)	((x)*1000)
#define TIME_MIN(x)	((x)*1000*60)
#define TIME_HOUR(x)	((x)*1000*60*60)
#define TIME_DAY(x)	((x)*1000*60*60*24)


/* Critical region definitions */
#define bottom_critical_region_begin() _disable_interrupts()
#define bottom_critical_region_end()    _enable_interrupts()


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
#define REGBIT_IER1_TINT1	6

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

/* Pin Control Registers for McBSP0, McBSP1 */
extern volatile uint16_t ioport PCR0, PCR1;
asm ("_PCR0 .set 0x2812");
asm ("_PCR1 .set 0x2c12");

#define REGBIT_PCR_DXSTAT	5
#define REGBIT_PCR_RIOEN	12
#define REGBIT_PCR_XIOEN	13

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
extern volatile uint16_t ioport PLLCSR, PLLM, PLLDIV1, PLLDIV2, PLLDIV3;
asm ("_PLLCSR .set 0x1c80");
asm ("_PLLM .set 0x1c88");
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
#define REGVAL_PLLCSR_STABLE		0x0040

#define REGVAL_PLLDIVx_DxEN		0x8000
#define REGVAL_PLLDIVx_PLLDIVx_1	0x0000
#define REGVAL_PLLDIVx_PLLDIVx_2	0x0001
#define REGVAL_PLLDIVx_PLLDIVx_4	0x0003


#endif
