// uba.c - KS10 Processor: Unibus emulation routines
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS-10 Emulator.
// See README for copyright notice.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "pdp10/defs.h"
#include "pdp10/uba.h"
#include "pdp10/proto.h"
#include "dev/rh.h"
#include "dev/proto.h"

extern DEVICE rh_Device;

DEVICE *ks10uba_Devices[] =
{
	&rh_Device
};

DEVICE ks10uba_Device =
{
	"UBA",              /* Device Name */
	"KS10 - Unibus System",
	"v0.8 (Alpha)",  /* Version */
	NULL,            /* Drive Type - Not Used */
	NULL,            /* Unit table - Not used */
	ks10uba_Devices, /* Listing of devices */
	NULL,            // Listing of Commands
	NULL,            // Listing of Set Commands
	NULL,            // Listing of Show Commands

	1,               /* Number of Devices */
	UBA_MAXUNITS,    /* Number of Units */

	ks10uba_Initialize, /* Initialize Routine */
	ks10uba_Reset,      /* Reset Routine */
	ks10uba_Create,     /* Create Routine */
	ks10uba_Delete,     /* Delete Routine */
	ks10uba_Configure,  /* Configure Routine */
	NULL,               /* Enable Routine */
	NULL,               /* Disable Routine */
	NULL,               /* Attach/Mount Routine - Not Used */
	NULL,               /* Detach/Unmount Routine - Not Used */
	NULL,               /* Format Routine */
	NULL,               /* Read I/O Routine */
	NULL,               /* Write I/O Routine */
	NULL,               /* Process Routine */
	NULL,               /* Boot Routine */
	NULL,               /* Execute Routine */

	NULL                /* SetUnit Routine - Not Used */
};

/* Unibus Adapters */
UNIT *uba_Units = NULL;

void ks10uba_Initialize(UNIT *ubaUnits, int32 maxUnits)
{
	int idx;

	for (idx = 0; idx < maxUnits; idx++) {
		ubaUnits[idx].Device = &ks10uba_Device;
		ubaUnits[idx].idUnit = idx;
	}
}

void ks10uba_Reset(UNIT *ubaUnit)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	UNIT    *sUnits  = ubaUnit->sUnits;
	int     idx;

	// Initialize all UBA registers
	for (idx = 0; idx < 077; idx++)
		ubaData->Map[idx] = 0;
	ubaData->sr = 0;
	ubaData->mr = 0;

	// Initialize all devices and controllers.
	for (idx = 0; idx < UBA_MAXUNITS; idx++) {
		if (sUnits[idx].Flags & UNIT_PRESENT) {
#ifdef DEBUG
			dbg_Printf("UBA %d: Initializing unit #%d\n", ubaUnit->idUnit, idx);
#endif DEBUG
			sUnits[idx].Device->Reset(&sUnits[idx]);
		}
	}
}

// Create UBA (Unibus Adapter) controller
int ks10uba_Create(UNIT *pUnit, char *devName, int argc, char **argv)
{
	int unit;
	int len, idx1, idx2;

	len = strlen(ks10uba_Device.Name);
	if (devName[len] == ':') {
		uba_Units = (UNIT *)calloc(UBA_MAX, sizeof(UNIT));

		if (uba_Units) {
			ks10uba_Initialize(uba_Units, UBA_MAX);
			if (pUnit) {
				pUnit->sUnits = uba_Units;
				pUnit->nUnits = UBA_MAX;
			}
			printf("Unit %s had been created.\n", devName);
		} else
			return EMU_MEMERR;

		return EMU_OK;
	}

	unit = toupper(devName[len]) - '0';

//	printf("Device: %s Unit: %d\n", devName, unit);

	if (uba_Units && (unit < UBA_MAX)) {
		UNIT *uptr = &uba_Units[unit];

		if (!(uptr->Flags & UNIT_PRESENT)) {
			UNIT    *ubaUnits = (UNIT *)calloc(UBA_MAXUNITS, sizeof(UNIT));
			UBAUNIT *ubaData  = (UBAUNIT *)calloc(1, sizeof(UBAUNIT));

			if (ubaUnits && ubaData) {
				int idx;

				// Initialize internal Unibus I/O page map
				for (idx = 0763000; idx < 0763102; idx += 2) {
					int32 idx2 = (idx & 017777) >> 1;

					ubaData->IOPage[idx2].pUnit = (UNIT *)UBA_INTERNAL;
				}

				// This unit is a Unibus interface.
				uptr->tFlags = UT_INTERFACE;
				uptr->dType  = NULL;
				uptr->Device = &ks10uba_Device;
				uptr->sUnits = ubaUnits;
				uptr->nUnits = UBA_MAXUNITS;
				uptr->Flags  = UNIT_PRESENT;
				uptr->uData  = (void *)ubaData;
			} else
				return EMU_MEMERR;
		} else {
			DEVICE *dptr;
			char   *pdevName = StrChar(devName, ':')+1;
			int    st;

			if ((dptr = unit_FindDevice(ks10uba_Devices, pdevName)) == NULL)
				return EMU_ARG;

			if (st = dptr->Create(uptr, pdevName, argc, argv))
				return st;

//			printf("Unit %s had been added.\n", pdevName);
		}
	} else
		return EMU_ARG;
	return EMU_OK;
}

// Delete the desired UBA controller
int ks10uba_Delete(int32 Unit)
{
	if (Unit < UBA_MAX) {
		UNIT *uptr = &uba_Units[Unit];

		if (uptr->Flags & UNIT_DISABLED)
			return EMU_ARG;

		free(uptr->sUnits);
		free(uptr->uData);
		uptr->Flags |= UNIT_DISABLED;
	} else
		return EMU_ARG;
	return EMU_OK;
}

int ks10uba_Configure(UNIT *ubaUnit, UNIT *Unit, int32 sAddress, int32 Mask)
{
	UNIT    *sUnits  = ubaUnit->sUnits;
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	UBAMAP  *ubaMap  = ubaData->IOPage;
	int32   eAddress = sAddress + (Mask + 1);
	int     idx;

	switch ((int)Unit) {
		case UBA_DELETE:
			for (idx = sAddress; idx < eAddress; idx += 2) {
				int32 idx2 = (idx & 017777) >> 1;

				ubaMap[idx2].pUnit   = NULL;
				ubaMap[idx2].ReadIO  = NULL;
				ubaMap[idx2].WriteIO = NULL;
			}
			break;

		case UBA_CHECK:
			for (idx = sAddress; idx < eAddress; idx += 2) {
				int32 idx2 = (idx & 017777) >> 1;

				if (ubaMap[idx2].pUnit)
					return EMU_UBA_BADADDR;
			}
			break;

		default:
			for (idx = sAddress; idx < eAddress; idx += 2) {
				int32 idx2 = (idx & 017777) >> 1;

//				printf("Assign %06o to %s\n", idx2, Unit->dType->Name);
				ubaMap[idx2].pUnit   = Unit;
				ubaMap[idx2].ReadIO  = Unit->Device->ReadIO;
				ubaMap[idx2].WriteIO = Unit->Device->WriteIO;
			}
			break;
	}

	return EMU_OK;
}

int18 ks10uba_ReadData18(UNIT *ubaUnit, int18 vaddr)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	int36 paddr = (ubaData->Map[(vaddr >> 11) & 077] & UBA_MAP_PAGE) |
		((vaddr >> 2) & 0777);

	if (vaddr & 2)
		return mem_prhRead(paddr);
	else
		return mem_plhRead(paddr);
}

int36 ks10uba_ReadData36(UNIT *ubaUnit, int18 vaddr)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	int36 paddr = (ubaData->Map[(vaddr >> 11) & 077] & UBA_MAP_PAGE) |
		((vaddr >> 2) & 0777);
	int36 data;

	data = p10_pRead(paddr, 0);

#ifdef DEBUG
	if (dbg_Check(DBG_IODATA))
		dbg_Printf("UBA %d: %06o (%06llo) => %s\n",
			ubaUnit->idUnit, vaddr, paddr, pdp10_DisplayData(data));
#endif DEBUG

	return data;
}

void ks10uba_WriteData18(UNIT *ubaUnit, int18 vaddr, int18 data)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	int36 paddr = (ubaData->Map[(vaddr >> 11) & 077] & UBA_MAP_PAGE) |
		((vaddr >> 2) & 0777);

	if (vaddr & 2)
		mem_prhWrite(paddr, data);
	else
		mem_plhWrite(paddr, data);
}

void ks10uba_WriteData36(UNIT *ubaUnit, int18 vaddr, int36 data)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;
	int36 paddr = (ubaData->Map[(vaddr >> 11) & 077] & UBA_MAP_PAGE) |
		((vaddr >> 2) & 0777);

#ifdef DEBUG
	if (dbg_Check(DBG_IODATA))
		dbg_Printf("UBA %d: %06o (%06llo) <= %s\n",
			ubaUnit->idUnit, vaddr, paddr, pdp10_DisplayData(SXT36(data)));
#endif DEBUG

	p10_pWrite(paddr, SXT36(data), 0);
}

void ks10uba_DoInterrupt(UNIT *ubaUnit, int32 IntBR, int32 IntVec)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("UBA: Interrupt - Unit %d, BR %d, Vector %04o\n",
			ubaUnit->idUnit, IntBR, IntVec);
#endif DEBUG

	if ((IntBR == 6) || (IntBR == 7)) {
		ubaData->sr |= UBA_SR_INTH;
		if (ubaData->sr & UBA_SR_PIH) {
#ifdef DEBUG
			if (dbg_Check(DBG_INTERRUPT))
				dbg_Printf("UBA: ** High Interrupt **\n");
#endif DEBUG
			ubaData->inth_Vector  = IntVec;
			ubaData->inth_Channel = (ubaData->sr & UBA_SR_PIH) >> 3;
			p10_piRequestIO(ubaData->inth_Channel);
		}
	}

	if ((IntBR == 4) || (IntBR == 5)) {
		ubaData->sr |= UBA_SR_INTL;
		if (ubaData->sr & UBA_SR_PIL) {
#ifdef DEBUG
			if (dbg_Check(DBG_INTERRUPT))
				dbg_Printf("UBA: ** Low Interrupt **\n");
#endif DEBUG
			ubaData->intl_Vector  = IntVec;
			ubaData->intl_Channel = ubaData->sr & UBA_SR_PIL;
			p10_piRequestIO(ubaData->intl_Channel);
		}
	}
}

void ks10uba_DisableInterrupt(UNIT *ubaUnit, int32 IntBR)
{
	UBAUNIT *ubaData = (UBAUNIT *)ubaUnit->uData;

	if ((IntBR == 6) || (IntBR == 7)) {
		ubaData->sr &= ~UBA_SR_INTH;
	}

	if ((IntBR == 4) || (IntBR == 5)) {
		ubaData->sr &= ~UBA_SR_INTL;
	}
}

int32 ks10uba_CheckInterrupt(int32 *Vector, int32 Channel)
{
	int Device;
	int Unit;

	for (Unit = 1; Unit < UBA_MAX; Unit += 2) {
		UBAUNIT *ubaData = (UBAUNIT *)uba_Units[Unit].uData;

		if (ubaData->sr & UBA_SR_INT) {
			if ((ubaData->sr & UBA_SR_INTH) &&
			    (ubaData->inth_Channel == Channel)) {
				Device = Unit;
				*Vector = ubaData->inth_Vector;
				ubaData->inth_Channel = 0;
				return Device;
			} else if ((ubaData->sr & UBA_SR_INTL) &&
			           (ubaData->intl_Channel == Channel)) {
				Device = Unit;
				*Vector = ubaData->intl_Vector;
				ubaData->intl_Channel = 0;
				return Device;
			}
		}
	}
	return 0;
}

void ks10uba_PageFailTrap(int30 ioAddr, int mode)
{

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("UBA: (%c) Unknown I/O Address: %o,,%06o\n",
			((mode & PTF_WRITE) ? 'W' : 'R'), LHSR(ioAddr), RH(ioAddr));
#endif DEBUG

	// Create a page fail word for bad I/O address
	// and perform 'page fail trap' routine.
	PFW = ((FLAGS & PC_USER) ? PFW1_USER : 0) |
		((mode & PTF_IOBYTE) ? PFW1_BYTE : 0) |
		PFW1_HARD | PFW1_PAGED | PFW1_IO | ioAddr;
	PC = RH(PC - 1);
	emu_Abort(p10_SetJump, PAGE_FAIL);
}

int36 ks10uba_mapRead(int32 Unit, int32 ioAddr)
{
	UBAUNIT *ubaData = (UBAUNIT *)uba_Units[Unit].uData;
	int36 Data = 0;
	int32 Reg;

	if ((ioAddr & ~UBA_MAP_MASK) == UBA_MAP_ADDR) {
		Reg = ioAddr & UBA_MAP_MASK;
		Data = ubaData->Map[Reg];
#ifdef DEBUG
		if (dbg_Check(DBG_IOREGS))
			dbg_Printf("UBA %d: PR %02o => %012llo\n", Unit, Reg, Data);
#endif DEBUG
	} else if (ioAddr == UBA_SR_ADDR) {
		Data = (ubaData->sr & UBA_SR_RDMASK);
#ifdef DEBUG
		if (dbg_Check(DBG_IOREGS))
			dbg_Printf("UBA %d: SR => %06llo\n", Unit, Data);
#endif DEBUG
	}

	return Data;
}

void ks10uba_mapWrite(int32 Unit, int32 ioAddr, int36 Data)
{
	UBAUNIT *ubaData = (UBAUNIT *)uba_Units[Unit].uData;
	int32 Reg;

	if ((ioAddr & ~UBA_MAP_MASK) == UBA_MAP_ADDR) {
		Reg = ioAddr & UBA_MAP_MASK;
		ubaData->Map[Reg] = (Data & UBAW_MAP_PAGE) << 9;
		ubaData->Map[Reg] |= (Data & UBAW_MAP_FLAGS) << 13;
#ifdef DEBUG
		if (dbg_Check(DBG_IOREGS))
			dbg_Printf("UBA %d: PR %02o <= %06llo (now: %012llo)\n",
				Unit, ioAddr & UBA_MAP_MASK, Data, ubaData->Map[Reg]);
#endif DEBUG
	} else if (ioAddr == UBA_SR_ADDR) {
		if (Data & UBA_SR_UINIT) {
#ifdef DEBUG
			if (dbg_Check(DBG_IOREGS))
				dbg_Printf("UBA %d: Unibus Initialization\n", Unit);
#endif DEBUG
			ks10uba_Reset(&uba_Units[Unit]);
		}
		ubaData->sr
			= (Data & UBA_SR_WRMASK) | (ubaData->sr & ~UBA_SR_WRMASK);
		ubaData->sr
			&= ~(Data & UBA_SR_CLMASK);
#ifdef DEBUG
		if (dbg_Check(DBG_IOREGS))
		dbg_Printf("UBA %d: SR <= %06llo (now: %06o)\n",
				Unit, Data, ubaData->sr);
#endif DEBUG
	} else if (ioAddr == UBA_MR_ADDR) {
		ubaData->mr = Data;
#ifdef DEBUG
		if (dbg_Check(DBG_IOREGS))
			dbg_Printf("UBA %d: MR <= %06llo\n", Unit, Data);
#endif DEBUG
	}
}

// **************************************************************

int36 ks10uba_ReadIO(int30 ioAddr, int mode)
{
	int32 Unit = IO_CONTROLLER(ioAddr);
	int32 Addr = IO_REG_ADDR(ioAddr);
	int32 data32;

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("UBA: (R) Controller: %o  Register Address: %06o\n",
			Unit, Addr);
#endif DEBUG

	if ((Unit < UBA_MAX) && uba_Units[Unit].uData) {
		UBAUNIT *ubaData = (UBAUNIT *)uba_Units[Unit].uData;
		UBAMAP  *ubaMap  = &ubaData->IOPage[(Addr & 017777) >> 1];

		switch ((int)ubaMap->pUnit) {
			case NULL:
				ubaData->sr |= (UBA_SR_TIM|UBA_SR_NED);
				break;

			case UBA_INTERNAL:
				return ks10uba_mapRead(Unit, Addr);

			default:
				ubaMap->ReadIO(ubaMap->pUnit, Addr, &data32);
				return data32;
		}
	}
 
	if ((Unit == 0) && (Addr == 0100000))
		return 0;

	ks10uba_PageFailTrap(ioAddr, mode | PTF_READ);

	return 0;
}

void ks10uba_WriteIO(int30 ioAddr, int36 Data, int mode)
{
	int32 Unit = IO_CONTROLLER(ioAddr);
	int32 Addr = IO_REG_ADDR(ioAddr);

#ifdef DEBUG
	if (dbg_Check(DBG_IOREGS))
		dbg_Printf("UBA: (W) Controller: %o  Register Address: %06o\n",
			Unit, Addr);
#endif DEBUG

	if ((Unit < UBA_MAX) && uba_Units[Unit].uData) {
		UBAUNIT *ubaData = (UBAUNIT *)uba_Units[Unit].uData;
		UBAMAP  *ubaMap  = &ubaData->IOPage[(Addr & 017777) >> 1];

		switch ((int)ubaMap->pUnit) {
			case NULL:
				ubaData->sr |= (UBA_SR_TIM|UBA_SR_NED);
				break;

			case UBA_INTERNAL:
				ks10uba_mapWrite(Unit, Addr, Data);
				return;

			default:
				ubaMap->WriteIO(ubaMap->pUnit, Addr, Data);
				return;
		}
	} 

	if ((Unit == 0) && (Addr == 0100000))
		return;

	ks10uba_PageFailTrap(ioAddr, mode | PTF_WRITE);
}
