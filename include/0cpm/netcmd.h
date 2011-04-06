/*
 * bpfcmd.h -- Berkeley Packet Filter (ish) command set.
 *
 * This defines a virtual machine which is based on the BPF
 * machine, as that is very suitable for taking apart packets,
 * storing juicy parts on a scratchpad, and finally deciding
 * what to do with it.
 *
 * The implementation, although inspired on the original
 * BPF code, is a full reimplementation, mainly because the
 * purpose is different: We want to generate code that is
 * as efficient as possible, and that can be overridden
 * locally by code that generates real assembler instructions
 * optimised for a particular target platform.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */



/* Enable overriding per target, if so desired.  This supports
 * direct translation of these instructions into real assembler,
 * fit for very fast execution of these vital bits of network
 * code.  The utility of this code is that it is simple and is
 * a delay in the actual handling of the network packet.  The
 * BPF setup is geared towards efficiency, although the added
 * value of this implementation is even better -- compile-time
 * translation into real machine instructions at the expense of
 * the flexibility that BPF needs but that we can live without.
 *
 * All data is stored in network byte order.  For some machines,
 * that implies conversions.  The commands below do as much of
 * these conversions on constants, counting on a clever compiler
 * to precalculate it before it turns into code.
 */

#ifdef HAVE_BPF_ASM_INLINES
#include <asm/bpfcmd.h>
#else


/* The following functions mark the structure of a program:
 *
 * BPF_BEGIN(selhdr)
 * 	...insn...
 * BPF_LABEL(skip)
 * 	...insn...
 * BPF_END()
 *
 * main () {
 * 	uint32_t memspace [20];
 * 	uint8_t *pkt = ...;
 * 	uint32_t pktlen = ...;
 * 	uint32_t outcome = selhdr (pkt, pktlen, memspace);
 * 	...
 * }
 */

#define BPF_BEGIN(fname) intptr_t fname (uint8_t *P, uint32_t L, uint32_t *M) { register uint32_t A, X=htonl ((uint32_t) P), S; register int k;
#define BPF_LABEL(l) l:
#define BPF_END() }
#define BPF_TAIL(fname) return (fname) (P, L, M);


/* Opcodes and their meaning:
 *
 * LDB_	load byte and expand into accu A, unsigned
 * LDH_	load halfword and expand into accu A, unsigned
 * LDW_ load word into accu A
 * LDX_ load word into indexreg X
 * STB_	store byte from accu A
 * STH_	store halfword from accu A
 * STW_	store accu A
 * STX_	store indexreg X
 * JMP_	jump always
 * JEQ_	jump on equal
 * JNE_	jump on not equal
 * JGT_	jump on greater
 * JGE_	jump on greater or equal
 * JLT_	jump on less
 * JLE_	jump on less or equal
 * JMN_	jump on nonzero after masking
 * JMZ_	jump on zero after masking
 * ADD_ add to accu A
 * SUB_ subtract from accu A
 * MUL_ multiply into accu A
 * DIV_ divide accu A by
 * AND_ bitwise and with accu A
 * IOR_ bitwise inclusive or with accu A
 * LSH_ left shift accu A
 * RSH_ right shift accu A
 * RET_ return value (finish program)
 * 
 * Note: TAX is renamed to LDX_ACC, TXA is renamed to LDW_REX
 *
 * Addressing modes
 *
 * _PKT k	[k]
 * _IDX k	[X+k]	(X not shown as an operand)
 * _LIT k	#k
 * _LEN		#len	(not shown as an operand)
 * _MEM k	M[k]	(M not shown as an operand)
 * _ACC		A	(not shown as an operand)
 * _TOP		A	(not shown, top half is used)
 * _REX		X	(not shown as an operand)
 * _ALW			(no operand)
 * _IP4		4*([k]&0x0f)	(not used in this phone)
 * _SUM		S	(not shown as an operand)
 *
 * Local extensions:
 * Commands	XOR_ STH_ STB_ SUM_ CLR_
 * Modes	_TOP _SUM
 * Combinations	ST*_PKT ST*_IDX 
 */



/* A few utility macros */

#define BPF_GET32(a,o) htonl ( (((uint8_t *)a)[o+0] << 24) | (((uint8_t *)a)[o+1] << 16) | (((uint8_t *)a)[o+2] << 8) | ((uint8_t *)a)[o+3] )
#define BPF_GET16(a,o) htonl (                                   (((uint8_t *)a)[o+0] << 8) | ((uint8_t *)a)[o+1] )
#define BPF_GET08(a,o) htonl (                                                   ((uint8_t *)a)[o+0] )

#define BPF_PUT32(a,o,v) ((uint8_t *)a)[o+0] = ((v) >> 24) & 0xff; \
                         ((uint8_t *)a)[o+1] = ((v) >> 16) & 0xff; \
                         ((uint8_t *)a)[o+2] = ((v) >>  8) & 0xff; \
                         ((uint8_t *)a)[o+3] = ((v)      ) & 0xff;
#define BPF_PUT16(a,o,v) ((uint8_t *)a)[o+0] = ((v) >>  8) & 0xff; \
                         ((uint8_t *)a)[o+1] = ((v)      ) & 0xff;
#define BPF_PUT08(a,o,v) ((uint8_t *)a)[o+0] = ((v)      ) & 0xff;


/* Instruction macros.  These can be overridden by
 * real assembly instructions for a particular target
 * platform if HAVE_BPF_ASM_INLINES is defined for it.
 *
 * TODO: Handle _PKT mode boundaries?  How about _IDX?
 */

#ifdef DEBUG
#  include <stdio.h>
#  define DEBUG_REGS printf ("%d: A=%08X X=%08X\n", __LINE__, htonl (A), htonl (X))
#  define DEBUG_REGS_PLUS printf ("%d: A=%08X X=%08X, S=%08X\n", __LINE__, htonl (A), htonl (X), S)
#else
#  define DEBUG_REGS
#  define DEBUG_REGS_PLUS
#endif

#define RET_LIT(k)	return (uint32_t) (k);			DEBUG_REGS;
#define RET_ACC()	return A;				DEBUG_REGS;
#define LDW_PKT(k)	A = BPF_GET32 (P, (k));			DEBUG_REGS;
#define LDH_PKT(k)	A = BPF_GET16 (P, (k));			DEBUG_REGS;
#define LDB_PKT(k)	A = BPF_GET08 (P, (k));			DEBUG_REGS;
#define LDW_LEN()	A = htonl (L);				DEBUG_REGS;
#define LDX_LIT(k)	X = htonl (k);				DEBUG_REGS;
#define LDX_LEN()	X = htonl (L);				DEBUG_REGS;
#define LDW_IDX(k)	A = BPF_GET32 (ntohl(X), (k));		DEBUG_REGS;
#define STW_IDX(k)	BPF_PUT32 (ntohl(X), (k), ntohl (A));	DEBUG_REGS;
#define LDH_IDX(k)	A = BPF_GET16 (ntohl(X), (k));		DEBUG_REGS;
#define STH_IDX(k)	BPF_PUT16 (ntohl(X), (k), ntohl (A));	DEBUG_REGS;
#define LDB_IDX(k)	A = BPF_GET08 (ntohl(X), (k));		DEBUG_REGS;
#define STB_IDX(k)	BPF_PUT08 (ntohl(X), (k), ntohl (A));	DEBUG_REGS;
#define LDX_IP4(k)	A = (P [k] & 0x0f) << 2;		DEBUG_REGS;
#define LDW_LIT(k)	A = htonl ((k) & 0xffffffff);		DEBUG_REGS;
#define LDH_LIT(k)	A = htonl ((k) & 0x0000ffff);		DEBUG_REGS;
#define LDB_LIT(k)	A = htonl ((k) & 0x000000ff);		DEBUG_REGS;
#define LDW_MEM(k)	A = htonl (M[k]);			DEBUG_REGS;
#define LDX_MEM(k)	X = htonl (M[k]);			DEBUG_REGS;
#define STW_MEM(k)	M[k] = ntohl (A);			DEBUG_REGS;
#define STX_MEM(k)	M[k] = ntohl (X);			DEBUG_REGS;
#define JMP_ALW(t)	goto t;		DEBUG_REGS;
#define JGT_LIT(k,t)	if (ntohl(A) >  (k)) goto t;		DEBUG_REGS;
#define JGE_LIT(k,t)	if (ntohl(A) >= (k)) goto t;		DEBUG_REGS;
#define JLT_LIT(k,t)	if (ntohl(A) <  (k)) goto t;		DEBUG_REGS;
#define JLE_LIT(k,t)	if (ntohl(A) <= (k)) goto t;		DEBUG_REGS;
#define JEQ_LIT(k,t)	if (ntohl(A) == (k)) goto t;		DEBUG_REGS;
#define JNE_LIT(k,t)	if (ntohl(A) != (k)) goto t;		DEBUG_REGS;
#define JMN_LIT(k,t)	if ((ntohl(A) &  (k)) != 0) goto t;	DEBUG_REGS;
#define JMZ_LIT(k,t)	if ((ntohl(A) &  (k)) == 0) goto t;	DEBUG_REGS;
#define ADD_REX()	A = htonl (ntohl (A) + ntohl (X));	DEBUG_REGS;
#define SUB_REX()	A = htonl (ntohl (A) - ntohl (X));	DEBUG_REGS;
#define MUL_REX()	A = htonl (ntohl (A) * ntohl (X));	DEBUG_REGS;
#define DIV_REX()	if (X == 0) return 0; A = htonl (ntohl (A) / ntohl (X)); \
								DEBUG_REGS;
#define AND_REX()	A = A & X;				DEBUG_REGS;
#define IOR_REX()	A = A | X;				DEBUG_REGS;
#define XOR_REX()	A = A ^ X;				DEBUG_REGS;
#define LSH_REX()	A = htonl (ntohl (A) << ntohl (X));	DEBUG_REGS;
#define RSH_REX()	A = htonl (ntohl (A) >> ntohl (X));	DEBUG_REGS;
#define ADD_LIT(k)	A = htonl (ntohl (A) + (k));		DEBUG_REGS;
#define SUB_LIT(k)	A = htonl (ntohl (A) - (k));		DEBUG_REGS;
#define MUL_LIT(k)	A = htonl (ntohl (A) * (k));		DEBUG_REGS;
#define DIV_LIT(k)	if ((k) == 0) return 0; A = htonl (ntohl (A) / (k)); \
								DEBUG_REGS;
#define AND_LIT(k)	A = A & htonl ((k));			DEBUG_REGS;
#define IOR_LIT(k)	A = A | htonl ((k));			DEBUG_REGS;
#define XOR_LIT(k)	A = A ^ htonl ((k));			DEBUG_REGS;
#define LSH_LIT(k)	A = htonl (ntohl (A) << (k))		DEBUG_REGS;
#define RSH_LIT(k)	A = htonl (ntohl (A) >> (k))		DEBUG_REGS;
#define NEG_ACC()	A = htonl (-ntohl (A))			DEBUG_REGS;
#define LDX_ACC()	X = A;					DEBUG_REGS;
#define LDW_REX()	A = A;					DEBUG_REGS;
#define SUM_PKT(k)	S += ntohl (BPF_GET16 (P, (k)));	DEBUG_REGS_PLUS;
#define SUM_IDX(k)	S += ntohl (BPF_GET16 (ntohl(X), (k)));	DEBUG_REGS_PLUS;
#define SUM_ACC()	S += ntohl (A) & 0xffff;		DEBUG_REGS_PLUS;
#define SUM_TOP()	S += ntohl (A) >> 16;			DEBUG_REGS_PLUS;
#define LDH_SUM()	printf ("S_pre = %08x\n", S); S = ((S & 0xffff) + (S >> 16)) & 0xffff; A = htonl (S) ^ 0xffff; printf ("S_post= %08x\n", S); \
								DEBUG_REGS_PLUS;
#define CLR_SUM()	S = 0;					DEBUG_REGS_PLUS;


#endif // HAVE_BPF_ASM_INLINES

