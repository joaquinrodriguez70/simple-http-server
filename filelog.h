void startfilelog(int filelog,char* filename);
void filelog(char *fmt,...);
void closefilelog();
void flushfilelog();
void starttty12();
void clrscrtty12();
void gotoxy(int x,int y);
void mensajetty12(char *fmt,...);
void mensajetty12xy(int x,int y,char *fmt,...);
void cierratty12();
void ventanatty12();
void scrolltty12();
extern int statusalterno;
extern int *numclientes;
extern int puertoatencion;
extern char versionSistema[10]; /*Numero del la version del sistema de taquilla*/



