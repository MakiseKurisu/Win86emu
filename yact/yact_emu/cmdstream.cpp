#include "stdafx.h"
#include <stdio.h>
#include "yact_emu.h"

bool FASTCALL ProcessCommand(x86opcode *Op)
{
	bool Ret;

	do
	{
#ifdef _DEBUG
		DWORD Addr=(DWORD)Op->CurrentPtr;
#endif
		Op->Current=FetchB(Op);
		Ret=(OneByteCmds[Op->Current])(Op);
#ifdef _DEBUG
		printf("%08X: ",Addr);
		for(DWORD i=Addr; i<(DWORD)Op->CurrentPtr; i++)
			printf("%02X ",*(BYTE*)i);
		printf("\t%s",Op->Mnemonics);
#endif
	} while(Ret);

	return !Op->Finished;
}
