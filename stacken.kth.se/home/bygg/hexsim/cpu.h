/*
 *  Global routines and data from cpu.c:
 */

/*
 *  Data others need, i.e. cpu registers.
 */

extern hexaword ICT;		/* Instruction Counter. */

extern hexaword AR, ARX;	/* "A" (arithmetic) register with extension. */
extern hexaword BR, BRX;	/* "B" (buffer) register with extension. */

extern hexaword EBR;		/* The Exec Base Register. */
extern hexaword UBR;		/* The User Base Register. */
extern hexaword SPT;		/* SPT base address. */
extern hexaword CST;		/* CST base address. */

/*
 *  Globally known constants:
 */

extern const hexaword FWMASK;
extern const hexaword LHMASK;
extern const hexaword RHMASK;
extern const hexaword RHSIGN;
extern const hexaword SIGNBIT;
extern const hexaword ZERO;

/*
 *  Routines:
 */

extern void cpu_init(void);

extern int cpu_disass(hexaword data, char buf[]);
extern int cpu_execute(void);

extern void cpu_show_hw_regs(void);

extern void uart_input(char c);

/*
 *  Register access:
 */

extern hexaword reg_read(uint8_t reg);
extern void     reg_write(uint8_t reg, hexaword data);
extern void     reg_deposit(uint8_t reg, hexaword data);
extern uint16_t reg_rcmask(void);

/*
 *  PC access:
 */

extern hexaword pc_examine(void);
extern void     pc_deposit(hexaword val);

/*
 *  PSW bits etc.
 */

#define PSW_NII   0x00000001	/* Next Instruction Inhibit. */
#define PSW_CRY   0x00000010	/* Carry bit. */
#define PSW_OVF   0x00000020	/* Overflow bit. */

extern uint32_t psw_read(void);
extern void     psw_write(uint32_t val);
extern void     psw_setbit(uint32_t bit);
extern void     psw_clrbit(uint32_t bit);
extern int      psw_chkbit(uint32_t bit);
