#include <stdio.h>
#include <stdarg.h>
#include "filelog.h"
#include <sys/sem.h>
#include <sys/shm.h>


FILE *fplog;
int hayfilelog;


// ABRE ARCHIVO DE RASTEO
void startfilelog(int filelog, char * filename)
{
	hayfilelog=filelog;
	if(hayfilelog)
	{
		fplog=fopen(filename,"w");
		printf("DEBUG ACTIVADO (%s)\n",filename);
	}
}

// ESCRIBE EN ARCHIVO DE RASTEO
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

// GUARDA EN DISCO BUFFER DE ARCHIVO
void flushfilelog()
{
	fflush(fplog);
}


