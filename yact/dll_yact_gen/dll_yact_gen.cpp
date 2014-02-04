// dll_stub_gen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>

#define NUMBADFUNCS 1024

static char*BadFuncs[NUMBADFUNCS];
int BadCount=0;


int _tmain(int argc, _TCHAR* argv[])
{
	char Buff[1024];

	if(argc<3)
	{
		puts("you should specify def and output file names.");
		return 1;
	}

	FILE *def=fopen(argv[1],"r");
	FILE *cpp=fopen(argv[2],"w");
	FILE *bad=0;
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

	if(!def || !cpp)
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

	fputs("#include \"dllyact_def.h\"\n\n",cpp);

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

		if(NParams%4 || Ptr1==0)
		{
			printf("Warning, incorrect number of params for function %s\n",Ptr);
			fprintf(cpp,"DEFINE_FUNC_(%s)\n",Ptr);
		} else
		{
			fprintf(cpp,"DEFINE_FUNC%d(%s)\n",NParams/4,Ptr);
		}
	}

	fclose(cpp);
	fclose(def);

	return 0;
}

