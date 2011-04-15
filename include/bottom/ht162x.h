/* HT162x includes
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* The following definitions are designed to clock bits
 * out starting at the high part of a 16-bit word.
 * Below these bits is a "1" bit and then fillup "0".
 */

#define HT162x_LCDPREFIX_CMD	(0x8000 | 0x1000)
#define HT162x_LCDPREFIX_WRITE	(0xa000 | 0x1000)

#define HT162x_LCD_ADDRESS6(x)	(((x) << 10) | 0x0200)
#define HT162x_LCD_NIBBLE(x)	(((x) << 12) | 0x0800)
#define HT162x_LCD_2NIBBLES(x)	(((x) <<  8) | 0x0080)
#define HT162x_LCD_CMD(x)	(((x) <<  8) | 0x0040)

#define HT162x_LCDCMD_SYSDIS	HT162x_LCD_CMD (0x00)
#define HT162x_LCDCMD_SYSEN	HT162x_LCD_CMD (0x01)
#define HT162x_LCDCMD_LCDOFF	HT162x_LCD_CMD (0x02)
#define HT162x_LCDCMD_LCDON	HT162x_LCD_CMD (0x03)
#define HT162x_LCDCMD_TIMERDIS	HT162x_LCD_CMD (0x04)
#define HT162x_LCDCMD_WDTDIS	HT162x_LCD_CMD (0x05)
#define HT162x_LCDCMD_TIMEREN	HT162x_LCD_CMD (0x06)
#define HT162x_LCDCMD_WDTEN	HT162x_LCD_CMD (0x07)
#define HT162x_LCDCMD_TONEOFF	HT162x_LCD_CMD (0x08)
#define HT162x_LCDCMD_TONEON	HT162x_LCD_CMD (0x09)
#define HT162x_LCDCMD_CLRTIMER	HT162x_LCD_CMD (0x0c)
#define HT162x_LCDCMD_CLRWDT	HT162x_LCD_CMD (0x0e)
#define HT162x_LCDCMD_XTAL32K	HT162x_LCD_CMD (0x14)
#define HT162x_LCDCMD_RC256K	HT162x_LCD_CMD (0x18)
#define HT162x_LCDCMD_EXT256K	HT162x_LCD_CMD (0x1c)
#define HT162x_LCDCMD_BIAS2_2	HT162x_LCD_CMD (0x20)
#define HT162x_LCDCMD_BIAS2_3	HT162x_LCD_CMD (0x24)
#define HT162x_LCDCMD_BIAS2_4	HT162x_LCD_CMD (0x28)
#define HT162x_LCDCMD_BIAS3_2	HT162x_LCD_CMD (0x21)
#define HT162x_LCDCMD_BIAS3_3	HT162x_LCD_CMD (0x25)
#define HT162x_LCDCMD_BIAS3_4	HT162x_LCD_CMD (0x29)
#define HT162x_LCDCMD_TONE4K	HT162x_LCD_CMD (0x40)
#define HT162x_LCDCMD_TONE2K	HT162x_LCD_CMD (0x60)
#define HT162x_LCDCMD_IRQDIS	HT162x_LCD_CMD (0x80)
#define HT162x_LCDCMD_IRQEN	HT162x_LCD_CMD (0x88)
#define HT162x_LCDCMD_F1	HT162x_LCD_CMD (0xa0)
#define HT162x_LCDCMD_F2	HT162x_LCD_CMD (0xa1)
#define HT162x_LCDCMD_F4	HT162x_LCD_CMD (0xa2)
#define HT162x_LCDCMD_F8	HT162x_LCD_CMD (0xa3)
#define HT162x_LCDCMD_F16	HT162x_LCD_CMD (0xa4)
#define HT162x_LCDCMD_F32	HT162x_LCD_CMD (0xa5)
#define HT162x_LCDCMD_F64	HT162x_LCD_CMD (0xa6)
#define HT162x_LCDCMD_F128	HT162x_LCD_CMD (0xa7)
//Note: HT162x_LCDCMD_TEST 	-> should not be used
#define HT162x_LCDCMD_NORMAL	HT162x_LCD_CMD (0xe3)


#define HT162x_LCDCMD_DONE	0x8000


/* The display data is stored in a shared array */
#ifndef HAVE_HT162x_DISPBYTES
#  define HAVE_HT162x_DISPBYTES 96
#endif
extern uint8_t ht162x_dispdata [HAVE_HT162x_DISPBYTES];

/* The program should define an array of LCDCMD_xxx to
 * initialise the HT162x chip.  The array should be
 * terminated with HT162x_LCDCMD_DONE.
 */
extern uint16_t ht162x_setup_cmdseq [];

/* The high-level driver offers a general setup routine
 * that runs over the command sequence above.
 */
void ht162x_setup_lcd (void);

/* Notify the HT162x driver of an update to a ht162x_dispdata range */
void ht162x_dispdata_notify (uint8_t minaddr, uint8_t maxaddr);

/* The low-level driver should provide a few functions that
 * enable toggling the CS, WR and DATA bits.
 */
void ht162x_chipselect (bool selected);
void ht162x_databit_prepare (bool databit);
void ht162x_databit_commit (void);

