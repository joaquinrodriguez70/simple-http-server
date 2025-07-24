#include <stdio.h>
#include <stdarg.h>
#include "filelog.h"



FILE *fplog;
int hayfilelog;

void startfilelog(int filelog, char * filename)
{
	hayfilelog=filelog;
	if(hayfilelog)
	{
		fplog=fopen(filename,"w");
		printf("DEBUG ACTIVE(%s)\n",filename);
	}
}

void filelog(char *fmt,...)
{
	va_list args;
	
	va_start(args,fmt);
	if(hayfilelog) 
		vfprintf(fplog,fmt,args);
		flushfilelog();
	va_end(args);
}


// CIERRA ARCHIVO DE RASTREO
void closefilelog()
{
	flushfilelog();
	fclose(fplog);
}


void flushfilelog()
{
	fflush(fplog);
}


