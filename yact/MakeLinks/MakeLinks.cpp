// MakeLinks.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <Shlobj.h>

HMODULE HM=0;

bool MakeLink(int csidl, int ResId)
{
	char Target[MAX_PATH];
	size_t tmp;
	BOOL Ret=SHGetSpecialFolderPathA(0,Target,csidl,FALSE);
	if(Ret==0)
		return false;

	char SymLink[MAX_PATH];
	if(getenv_s(&tmp,SymLink,"ALLUSERSPROFILE"))
		return false;

	strcat_s(SymLink,"\\");
	if(LoadStringA(HM,ResId,SymLink+strlen(SymLink),MAX_PATH-strlen(SymLink))==0)
		return false;

	printf("Linking directory '%s' to '%s' - ",SymLink,Target);

	return CreateSymbolicLinkA(SymLink,Target,SYMBOLIC_LINK_FLAG_DIRECTORY)!=0;
}

#define IDS_ADMINTOOLS              67
#define IDS_APPDATA                 57
#define IDS_DESKTOP                 20
#define IDS_COMMON_DOCUMENTS        66
#define IDS_COMMON_MUSIC            68
#define IDS_COMMON_PICTURES         69
#define IDS_COMMON_VIDEO            70
#define IDS_STARTMENU               51

int Ids[][2]={
	{CSIDL_COMMON_APPDATA, IDS_APPDATA},
	{CSIDL_COMMON_DESKTOPDIRECTORY, IDS_DESKTOP},
	{CSIDL_COMMON_DOCUMENTS, IDS_COMMON_DOCUMENTS},
	{CSIDL_COMMON_MUSIC, IDS_COMMON_MUSIC},
	{CSIDL_COMMON_PICTURES, IDS_COMMON_PICTURES},
	{CSIDL_COMMON_VIDEO, IDS_COMMON_VIDEO},
	{CSIDL_COMMON_STARTMENU, IDS_STARTMENU}, 
	0,0
};

int _tmain(int argc, _TCHAR* argv[])
{
	HM=LoadLibraryExA("c:\\x86node\\Windows\\System32\\shell32.dll",0, LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_LIBRARY_AS_DATAFILE);
	if(HM==0)
	{
		puts("Unable to load x86 shell32.dll");
		return 1;
	}
	
	for(int i=0; Ids[i][0]; i++)
	{
		if(MakeLink(Ids[i][0],Ids[i][1]))
			puts("ok");
		else
			puts("failed");
	}
	
	return 0;
}

