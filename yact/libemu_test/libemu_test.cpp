// libemu_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <classes.h>

int MyFunc()
{
	return 0x12345678;
}

extern "C" void __declspec(dllimport) Emulate(DWORD addr);


int _tmain(int argc, _TCHAR* argv[])
{
//	Emulate((DWORD)MyFunc);

	CSet Set;

	Set.Set(1234,"lalala");
	puts((char*)Set.Get(1234));


	getchar();
	return 0;
}

