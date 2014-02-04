// dll_stub_gen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <string>
#include <map>

using namespace std;

#define NUMBADFUNCS 1024

static char*BadFuncs[NUMBADFUNCS];
int BadCount=0;

struct FuncDef
{
	int NParams;
	int Ordinal;
	FuncDef() : NParams(0), Ordinal(0) {}
};

map<string,FuncDef> Functions;

int _tmain(int argc, _TCHAR* argv[])
{
	char Buff[1024];
//	DebugBreak();

	if(argc<3)
	{
		puts("you should specify def and spec file names.");
		return 1;
	}

	FILE *def=fopen(argv[1],"r");
	FILE *spec=0;
	FILE *bad=0;
	if(argc>2)
	{
		spec=fopen(argv[2],"r");
		while(spec && !feof(spec))
		{
			Buff[0]=0;
			fgets(Buff,1024,spec);
			char *Ptr=strrchr(Buff,'#');
			if(Ptr)
				*Ptr=0;
			Ptr=Buff;
			while(*Ptr<=' ' && *Ptr)
				Ptr++;
			int ord=atoi(Ptr);
			char* Ob=strchr(Ptr,'(');
			char* Cb=strchr(Ptr,')');
			if(Ob==0 || Cb==0)
				continue;
			char *TmpOB=Ob-1;
			*Ob=0; Ob++;
			*Cb=0; Cb--;
			while(*TmpOB==' ')
			{
				*TmpOB=0;
				TmpOB--;
			}
			while(*Cb==' ')
			{
				*Cb=0;
				Cb--;
			}

			Ptr=strrchr(Ptr,' ');
			if(Ptr==0)
				continue;
			else 
				Ptr++;

			int NParams=0;
			if(Ob[0]!=0)
				NParams++;
			for(int i=0; Ob[i]; i++)
			{
				if(Ob[i]==' '&&Ob[i-1]!=' ')
					NParams++;
			}
			char *At=strchr(Ptr,'@');
			if(At)
				*At=0;
			FuncDef f;
			string Name(Ptr);
			f.NParams=NParams;
			f.Ordinal=ord;
			if(Functions[Name].Ordinal==0)
				Functions[Name]=f;
		}
	}
	if(argc>3)
	{
		bad=fopen(argv[3],"r");
		while(bad && !feof(bad) && BadCount<NUMBADFUNCS)
		{
			Buff[0]=0;
			fgets(Buff,1024,bad);
			char *Ptr=Buff;
			while(*Ptr<=' ' && *Ptr)
				Ptr++;
			for(int i=0; i<strlen(Ptr); i++)
				if(Ptr[i]<=' ' || Ptr[i]=='=' || Ptr[i]=='@')
					Ptr[i]=0;
			if(Buff[0] && Buff[0]!=';')
				BadFuncs[BadCount++]=strdup(Buff);
		}
	}

	if(!def)
	{
		puts("error opening files.");
		return 1;
	}

	while(!feof(def))
	{
		Buff[0]=0;
		fgets(Buff,1024,def);
		if(memcmp(Buff,"EXPORTS",5)==0)
			break;
	}
	if(feof(def))
	{
		puts("bad def file.");
		return 1;
	}

	char Tmp[_MAX_PATH];
	char *P=strrchr(argv[1],'\\');
	if(P==0)
		P=argv[1];
	else 
		P++;
	strcpy(Tmp,P);
	P=strrchr(Tmp,'.');
	if(P)
		*P=0;

	char CppFN[_MAX_PATH];
	char F1[_MAX_PATH];
	char F2[_MAX_PATH];
	char F3[_MAX_PATH];
	char F4[_MAX_PATH];
	char F5[_MAX_PATH];
	char DirStub[_MAX_PATH];
	char DirYact[_MAX_PATH];
	sprintf(DirStub,"%s_stub",Tmp);
	sprintf(DirYact,"%s_yact",Tmp);
	CreateDirectory(DirStub,0);
	CreateDirectory(DirYact,0);
	sprintf(CppFN,"%s\\%s.cpp",DirStub,Tmp);
	sprintf(F1,"%s\\dllmain.cpp",DirStub);
	sprintf(F2,"%s\\stdafx.cpp",DirStub);
	sprintf(F3,"%s\\stdafx.h",DirStub);
	sprintf(F4,"%s\\targetver.h",DirStub);
	sprintf(F5,"%s\\%s.86.vcxproj",DirStub,Tmp);
	CopyFile("template\\dllmain.cpp",F1,FALSE);
	CopyFile("template\\stdafx.cpp",F2,FALSE);
	CopyFile("template\\stdafx.h",F3,FALSE);
	CopyFile("template\\targetver.h",F4,FALSE);

	FILE *cpp=fopen(CppFN,"w");

	FILE *prj=fopen("template\\template.86.vcxproj","r");
	static char VcPrj[65536];
	static char VcPrj1[65536];
	fread(VcPrj,65536,1,prj);
	fclose(prj);
	prj=fopen(F5,"w");
	sprintf(VcPrj1,VcPrj,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp);
	*strchr(VcPrj1,155)=0;
	fputs(VcPrj1,prj);
	fclose(prj);

	fputs("#include \"dllstub_def.h\"\n\n",cpp);
	fprintf(cpp,"static const char DLL_NAME[]=\"%s.nt.dll\";\n\n",Tmp);

	while(!feof(def))
	{
		Buff[0]=0;
		fgets(Buff,1024,def);
		if(memcmp(Buff,"IMPORTS",5)==0)
			break;
		char *Ptr=Buff;
		while(*Ptr<=' ' && *Ptr)
			Ptr++;
		char *Ptr1=0;

		while(strlen(Ptr))
			if(Ptr[strlen(Ptr)-1]<=' ')
				Ptr[strlen(Ptr)-1]=0;
			else
				break;

		char *Ord=strrchr(Ptr,' ');
		int Ordinal=0;
		if(Ord && Ord[1]=='@')
			Ordinal=atoi(Ord+2);
		for(int i=0; i<strlen(Ptr); i++)
		{
			if(Ptr[i]<=' ' || Ptr[i]=='=')
				Ptr[i]=0;
			if(Ptr[i]=='@')
			{
				Ptr1=Ptr+i+1;
				Ptr[i]=0;
			}
		}

		for(int i=0; i<BadCount; i++)
			if(strcmp(BadFuncs[i],Ptr)==0)
			{
				*Ptr=0;
				break;
			}

		if(strlen(Ptr)==0 || *Ptr==';')
			continue;

		int NParams=0;
		if(Ptr1)
			NParams=atoi(Ptr1);

/*		if(NParams%4 || Ptr1==0)
		{
			printf("Warning, incorrect number of params for function %s\n",Ptr);
			if(Ordinal)
				fprintf(cpp,"DEFINE_FUNC_ORD_(%s,%d)\n",Ptr,Ordinal);
			else
				fprintf(cpp,"DEFINE_FUNC_(%s)\n",Ptr);
		} else
		{
			if(Ordinal)
				fprintf(cpp,"DEFINE_FUNC_ORD%d(%s,%d)\n",NParams/4,Ptr,Ordinal);
			else
				fprintf(cpp,"DEFINE_FUNC%d(%s)\n",NParams/4,Ptr);
		}*/
		FuncDef f;
		string Name(Ptr);
		f.NParams=NParams/4;
		f.Ordinal=Ordinal;
		if(Functions[Name].Ordinal==0)
			Functions[Name]=f;
	}

	for(auto it = Functions.begin(); it != Functions.end() ; ++it)
	{
		if(it->second.Ordinal)
			fprintf(cpp,"DEFINE_FUNC_ORD%d(%s,%d)\n",it->second.NParams,it->first.c_str(),it->second.Ordinal);
		else
			fprintf(cpp,"DEFINE_FUNC%d(%s)\n",it->second.NParams,it->first.c_str());
	}

	fclose(cpp);
	fclose(def);

	sprintf(CppFN,"%s\\%s.cpp",DirYact,Tmp);
	sprintf(F1,"%s\\dllmain.cpp",DirYact);
	sprintf(F2,"%s\\stdafx.cpp",DirYact);
	sprintf(F3,"%s\\stdafx.h",DirYact);
	sprintf(F4,"%s\\targetver.h",DirYact);
	sprintf(F5,"%s\\%s.nt.vcxproj",DirYact,Tmp);
	CopyFile("template\\dllmain.cpp",F1,FALSE);
	CopyFile("template\\stdafx.cpp",F2,FALSE);
	CopyFile("template\\stdafx.h",F3,FALSE);
	CopyFile("template\\targetver.h",F4,FALSE);
	cpp=fopen(CppFN,"w");

	prj=fopen("template\\template.nt.vcxproj","r");
	fread(VcPrj,65536,1,prj);
	fclose(prj);
	prj=fopen(F5,"w");
	sprintf(VcPrj1,VcPrj,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp,Tmp);
	*strchr(VcPrj1,155)=0;
	fputs(VcPrj1,prj);
	fclose(prj);

	fputs("#include \"dllyact_def.h\"\n\n",cpp);
	for(auto it = Functions.begin(); it != Functions.end() ; ++it)
	{
		fprintf(cpp,"DEFINE_FUNC%d(%s)\n",it->second.NParams,it->first.c_str());
	}

	fclose(cpp);

	return 0;
}

