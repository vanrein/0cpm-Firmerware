/* Micrel KSZ8842 includes.
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


#ifndef HEADER_KSZ8842
#define HEADER_KSZ8842


typedef uint16_t kszint16_t;
typedef uint32_t kszint32_t;


#define KSZ8842_BANKx_BSR	kszreg16(14)

#define KSZ8842_BANK0_BAR	kszreg16(0)
#define KSZ8842_BANK2_MARL	kszreg16(0)
#define KSZ8842_BANK2_MARM	kszreg16(2)
#define KSZ8842_BANK2_MARH	kszreg16(4)
#define KSZ8842_BANK0_BESR	kszreg16(6)

#define KSZ8842_BANK16_TXCR	kszreg16(0)
#define KSZ8842_BANK16_TXSR	kszreg16(2)
#define KSZ8842_BANK16_RXCR	kszreg16(4)
#define KSZ8842_BANK16_TXMIR	kszreg16(8)
#define KSZ8842_BANK16_RXMIR	kszreg16(10)
#define KSZ8842_BANK17_TXQCR	kszreg16(0)
#define KSZ8842_BANK17_RXQCR	kszreg16(2)
#define KSZ8842_BANK17_TXFDPR	kszreg16(4)
#define KSZ8842_BANK17_RXFDPR	kszreg16(6)
#define KSZ8842_BANK17_QDRL	kszreg16(8)
#define KSZ8842_BANK17_QDRH	kszreg16(10)
#define KSZ8842_BANK18_IER	kszreg16(0)
#define KSZ8842_BANK18_ISR	kszreg16(2)
#define KSZ8842_BANK18_RXSR	kszreg16(4)
#define KSZ8842_BANK18_RXBC	kszreg16(6)

#define KSZ8842_BANK32_SIDER	kszreg16(0)

#define KSZ8842_BANK45_P1MBSR	kszreg16(2)
#define KSZ8842_BANK46_P2MBSR	kszreg16(2)

#define KSZ8842_BANK_SET(x)	(KSZ8842_BANKx_BSR = kszmap16 (x))


#define REGVAL_KSZ8842_ISR_LCIS		0x8000
#define REGVAL_KSZ8842_ISR_TXIS		0x4000
#define REGVAL_KSZ8842_ISR_RXIS		0x2000
#define REGVAL_KSZ8842_ISR_RXOIS	0x0800
#define REGVAL_KSZ8842_ISR_TXPSIE	0x0200
#define REGVAL_KSZ8842_ISR_RXPSIE	0x0100
#define REGVAL_KSZ8842_ISR_RXEFIE	0x0080

#define REGVAL_KSZ8842_TXCR_TXFCE	0x0008
#define REGVAL_KSZ8842_TXCR_TXPE	0x0004
#define REGVAL_KSZ8842_TXCR_TXCE	0x0002
#define REGVAL_KSZ8842_TXCR_TXE		0x0001

#define REGVAL_KSZ8842_RXCR_RXFCE	0x0400
#define REGVAL_KSZ8842_RXCR_RXEFE	0x0200
#define REGVAL_KSZ8842_RXCR_RXBE	0x0080
#define REGVAL_KSZ8842_RXCR_RXME	0x0040
#define REGVAL_KSZ8842_RXCR_RXUE	0x0020
#define REGVAL_KSZ8842_RXCR_RXRA	0x0010
#define REGVAL_KSZ8842_RXCR_RXSCE	0x0008
#define REGVAL_KSZ8842_RXCR_RXE		0x0001

#define REGVAL_KSZ8842_RXSR_RXFV	0x8000
#define REGVAL_KSZ8842_RXSR_RXBF	0x0080
#define REGVAL_KSZ8842_RXSR_RXMF	0x0040
#define REGVAL_KSZ8842_RXSR_RXUF	0x0020
#define REGVAL_KSZ8842_RXSR_RXFT	0x0008
#define REGVAL_KSZ8842_RXSR_RXTL	0x0004
#define REGVAL_KSZ8842_RXSR_RXRF	0x0002
#define REGVAL_KSZ8842_RXSR_RXCE	0x0001

#define REGVAL_KSZ8842_TXQCR_TXETF	0x0001
#define REGVAL_KSZ8842_RXQCR_RXRRF	0x0001

#define REGVAL_KSZ8842_PxMBSR_ONLINE	0x0004

#define CTLVAL_KSZ8842_TXIC		0x8000


#endif


/* Data is read 4 bytes at a time, and the buffer
 * is a multiple of 2 bytes, so reserve 2 bytes
 * extra for overflow.
 */
#define HAVE_NET_TRAILER 2


/* Map KSZ8842 network data to or from general purpose memory */
void ksz_memset16 (kszint16_t ksz, uint8_t *mem);
kszint16_t ksz_memget16 (uint8_t *mem);


/* Setup the KSZ8842 chip */
void ksz8842_setup_network (void);

/* KSZ8842 interrupt handler, _but_ compiled as a plain function.
 * Retrieve the ISR register and handle each interrupt reason.
 */
void ksz8842_interrupt_handler (void);
