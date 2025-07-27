#pragma once

#include <cstdint>

// Basic type definitions from PCSX2's Pcsx2Defs.h
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

// Basic vector types that GPRs can be viewed as.
union u128 {
    u64 UD[2];
    u32 UL[4];
    u16 US[8];
    u8 UC[16];
};

union s128 {
    s64 SD[2];
    s32 SL[4];
    s16 SS[8];
    s8 SC[16];
};


// --- Structs from R5900.h, simplified for recompilation ---
union PERFregs {
	struct
	{
		union
		{
			struct
			{
				u32 pad0:1;			// LSB should always be zero (or undefined)
				u32 EXL0:1;			// enable PCR0 during Level 1 exception handling
				u32 K0:1;			// enable PCR0 during Kernel Mode execution
				u32 S0:1;			// enable PCR0 during Supervisor mode execution
				u32 U0:1;			// enable PCR0 during User-mode execution
				u32 Event0:5;		// PCR0 event counter (all values except 1 ignored at this time)

				u32 pad1:1;			// more zero/undefined padding [bit 10]

				u32 EXL1:1;			// enable PCR1 during Level 1 exception handling
				u32 K1:1;			// enable PCR1 during Kernel Mode execution
				u32 S1:1;			// enable PCR1 during Supervisor mode execution
				u32 U1:1;			// enable PCR1 during User-mode execution
				u32 Event1:5;		// PCR1 event counter (all values except 1 ignored at this time)

				u32 Reserved:11;
				u32 CTE:1;			// Counter enable bit, no counting if set to zero.
			} b;

			u32 val;
		} pccr;

		u32 pcr0, pcr1, pad;
	} n;
	u32 r[4];
};

// Represents a single 128-bit General Purpose Register (GPR).
union GPR_reg {
    u128 UQ;
    s128 SQ;
    u64 UD[2];
    s64 SD[2];
    u32 UL[4];
    s32 SL[4];
    u16 US[8];
    s16 SS[8];
    u8  UC[16];
    s8  SC[16];
};

// Represents all 32 GPRs.
union GPRregs {
    struct {
        GPR_reg r0, at, v0, v1, a0, a1, a2, a3,
                t0, t1, t2, t3, t4, t5, t6, t7,
                s0, s1, s2, s3, s4, s5, s6, s7,
                t8, t9, k0, k1, gp, sp, s8, ra;
    } n;
    GPR_reg r[32];
};

// Represents the 32 registers of Coprocessor 0 (the System Control Coprocessor).
// These handle exceptions, memory management, and other system-level tasks.
union CP0regs {
	struct {
		u32	Index,    Random,    EntryLo0,  EntryLo1,
			Context,  PageMask,  Wired,     Reserved0,
			BadVAddr, Count,     EntryHi,   Compare;
		union {
			struct {
				u32 IE:1;		// Bit 0: Interrupt Enable flag.
				u32 EXL:1;		// Bit 1: Exception Level, set on any exception not covered by ERL.
				u32 ERL:1;		// Bit 2: Error level, set on Resetm NMI, perf/debug exceptions.
				u32 KSU:2;		// Bits 3-4: Kernel [clear] / Supervisor [set] mode
				u32 unused0:3;
				u32 IM:8;		// Bits 10-15: Interrupt mask (bits 12,13,14 are unused)
				u32 EIE:1;		// Bit 16: IE bit enabler.  When cleared, ints are disabled regardless of IE status.
				u32 _EDI:1;		// Bit 17: Interrupt Enable (set enables ints in all modes, clear enables ints in kernel mode only)
				u32 CH:1;		// Bit 18: Status of most recent cache instruction (set for hit, clear for miss)
				u32 unused1:3;
				u32 BEV:1;		// Bit 22: if set, use bootstrap for TLB/general exceptions
				u32 DEV:1;		// Bit 23: if set, use bootstrap for perf/debug exceptions
				u32 unused2:2;
				u32 FR:1;		// (?)
				u32 unused3:1;
				u32 CU:4;		// Bits 28-31: Co-processor Usable flag
			} b;
			u32 val;
		} Status;
		u32   Cause,    EPC,       PRid,
			Config,   LLAddr,    WatchLO,   WatchHI,
			XContext, Reserved1, Reserved2, Debug,
			DEPC,     PerfCnt,   ErrCtl,    CacheErr,
			TagLo,    TagHi,     ErrorEPC,  DESAVE;
	} n;
	u32 r[32];
};
// A container for the core CPU registers.
struct cpuRegisters {
	GPRregs GPR;		// GPR regs
	// NOTE: don't change order since recompiler uses it
	GPR_reg HI;
	GPR_reg LO;			// hi & log 128bit wide
	CP0regs CP0;		// is COP0 32bit?
	u32 sa;				// shift amount (32bit), needs to be 16 byte aligned
	u32 IsDelaySlot;	// set true when the current instruction is a delay slot.
	u32 pc;				// Program counter, when changing offset in struct, check iR5900-X.S to make sure offset is correct
	u32 code;			// current instruction
	PERFregs PERF;
	u32 eCycle[32];
	u32 sCycle[32];		// for internal counters
	u32 cycle;			// calculate cpucycles..
	u32 interrupt;
	int branch;
	int opmode;			// operating mode
	u32 tempcycles;
	u32 dmastall;
	u32 pcWriteback;

	// if cpuRegs.cycle is greater than this cycle, should check cpuEventTest for updates
	u32 nextEventCycle;
	u32 lastEventCycle;
	u32 lastCOP0Cycle;
	u32 lastPERFCycle[2];
};

// Represents a single 32-bit Floating Point Register (FPR).
union FPRreg {
    float f;
    u32 UL;
    s32 SL;
};

// Represents the state of the Floating Point Unit (FPU).
struct fpuRegisters {
    FPRreg fpr[32];
    u32 fprc[32]; // FPU control registers
    FPRreg ACC;   // Accumulator register
};

// This is the main, complete state of the Emotion Engine that our
// recompiled functions will operate on. It contains the core CPU
// registers and the FPU registers.
struct EmotionEngineState {
    cpuRegisters cpuRegs;
    fpuRegisters fpuRegs;
};