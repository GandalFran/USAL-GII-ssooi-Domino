#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>

//TIPOS
typedef struct{
	int p1,p2,p3;
}pids;
//VARIABLES GLOBALES
int descriptor;
int pidHijo=-1, pidHijo2=-1, pLectura;
pids*pb = NULL;
sigset_t conjuntoLimpiador;

//PROTOTIPOS
	//GENERAR ARBOL
	void generarProceso( void(*funcionCallBack)(int) );
	void rama1();
	void rama2();
	void rama3();
	void rama4();

	//MATAR EL ARBOL --> MATAR Y ESPERAR --> FUNCIONES DE CALLBACK
	void esperarMuerteHijo(int ss);
	void matarUnHijo(int ss);
	void matarDosHijos(int ss);
	void matarBastardo(int ss);

	//FICHEROS
	void escribirPid(int pid,int i);
	int leerPid(int i);
	void proyectarFichero();
	void desproyectarFichero();
	
		
	//SEÑALES
	void redefinirSennal(int sennal,void(*funcion)(int));
	void esperarSigSuspend(int sennal);
	void mandarSennal(int pidDestinatario,int sennal);
	void esperarPorHijo(int pid);
	void callBackUltimo(int s);
	
	//AUXILIARES
	void imprimirError(char*posicionError,char*orden,char*otros,int pidProceso);

//PROGRAMA PRINCIPAL
	int main (void){
		//Proyectar fichero y dar valores iniciales
		proyectarFichero();
		pLectura = 0;

		//Enmascarar
       	sigset_t conjunto; 
        if(-1 == sigfillset(&conjunto) ) imprimirError("main","sigfillset",NULL,getpid());
        if(-1 == sigprocmask(SIG_SETMASK, &conjunto, &conjuntoLimpiador) ) imprimirError("main","sigprocmask",NULL,getpid());

        //redefinir SIGTERM en el primer proceso
        redefinirSennal(SIGTERM,matarUnHijo);
		//generamos proceso 38
		generarProceso(matarUnHijo);
		//generamos proceso 39
		generarProceso(matarDosHijos);
		//GENERANDO 40 - 45
		switch(pidHijo=fork()){
				case(-1): //ERROR
					imprimirError("main","fork",NULL, getpid());
				break;
				case(0): //HIJO1 -- PROCESO 40
					fprintf(stderr, "\nPPID: %d|PID: %d",getppid(),getpid());
							//===========================================================
							switch(pidHijo=fork()){
								case(-1): //ERROR
									imprimirError("main","fork",NULL,getpid());
								break;
								case(0): //HIJO1 -- PROCESO 42
									fprintf(stderr, "\nRAMA 1 PPID: %d|PID: %d",getppid(),getpid());
									redefinirSennal(SIGTERM,matarUnHijo);
									rama1();
								break;
								default: //PADRE -- PROCESO 40
									switch(pidHijo2=fork()){
										case(-1): //ERROR
											imprimirError("main","fork",NULL,getpid());
											break;
										case(0): //HIJO2 -- PROCESO 43
											fprintf(stderr, "\nRAMA 2 PPID: %d|PID: %d",getppid(),getpid());
											redefinirSennal(SIGTERM,matarUnHijo);
											rama2();
										break;
										default: //PADRE --40
											redefinirSennal(SIGTERM,matarDosHijos);
											esperarSigSuspend(SIGTERM);
									}
							}
							//===========================================================
				break;
				default: //PADRE  -- PROCESO 39
					switch(pidHijo2=fork()){
						case(-1): //ERROR
							imprimirError("main","fork",NULL,getpid());
						break;
						case(0): //HIJO2 -- PROCESO 41
								fprintf(stderr, "\nPPID: %d|PID: %d",getppid(),getpid());
								//===========================================================
								switch(pidHijo=fork()){
									case(-1): //ERROR
										imprimirError("main","fork",NULL,getpid());
									break;
									case(0): //HIJO1 -- PROCESO 44
										fprintf(stderr, "\nRAMA 3 PPID: %d|PID: %d",getppid(),getpid());
										redefinirSennal(SIGTERM,matarUnHijo);
										rama3();
									break;
									default: //PADRE -- PROCESO 41
										switch(pidHijo2=fork()){
											case(-1): //ERROR
												imprimirError("main","fork",NULL,getpid());
											break;
											case(0): //HIJO2 -- PROCESO 45
												fprintf(stderr, "\nRAMA 4 PPID: %d|PID: %d",getppid(),getpid());
												redefinirSennal(SIGTERM,matarUnHijo);
												rama4();
											break;
											default: //PADRE -- PROCESO 41
												redefinirSennal(SIGTERM,matarDosHijos);
												esperarSigSuspend(SIGTERM);
										}
								}
								//===========================================================
						break;
						default: //PADRE -- PROCESO 39
							esperarSigSuspend(SIGTERM);
					}
			}

		return 0;
	}	

	///Ejecutado por proceso 42
	void rama1(){	
		//generamos el proceso 46
		generarProceso(matarUnHijo);
		//generamos el proceso 50
		generarProceso(esperarMuerteHijo);
		//generamos el proceso 54
		generarProceso(esperarMuerteHijo);
		//Ahora estamos en el proceso 54 y escribimos su pid en el fichero
			escribirPid(getpid(),1);
		//generamos el proceso 56
		generarProceso(matarUnHijo);
		//Ahora estamos en el proceso 56 y escribimos su pid en el fichero
			escribirPid(getpid(),3);
		//generamos el proceso 57
		generarProceso(matarUnHijo);
		//generamos el proceso 58
		generarProceso(callBackUltimo);	
		esperarSigSuspend(SIGTERM);	
	}
	//Ejecutado por proceso 43
	void rama2(){
		//Generamos el proceso 47
		generarProceso(matarUnHijo);
		//Generamos el proceso 51
		generarProceso(matarBastardo);
		//Ahora estamos en el proceso 51
		pLectura = 1;
		esperarSigSuspend(SIGTERM);
	}	
	//Ejecutado por proceso 44
	void rama3(){
		//generamos proceso 48
		generarProceso(matarUnHijo);
		//generamos procesos 52
		generarProceso(esperarMuerteHijo);
		//generamos el proceso 55
		generarProceso(matarBastardo);
		//Ahora estamos en el 55
		pLectura = 3;
		//escribimos el pid del 55 en el fichero
			escribirPid(getpid(),2);
		esperarSigSuspend(SIGTERM);
	}	
	//Ejecutado por proceso 45
	void rama4(){
		//Generamos el proceso 49
		generarProceso(matarUnHijo);
		//Generamos el proceso 53
		generarProceso(matarBastardo);
		//Ahora estamos en el proceso 53
		pLectura = 2;
		//Esperamos a sigterm para muerte
		esperarSigSuspend(SIGTERM);
	}


//GENERAR ARBOL
	void generarProceso(void(*funcionCallBack)(int)){

		switch(pidHijo=fork()){
			case(-1): //ERROR
				imprimirError("generarProceso","fork",NULL,getpid());
			break;
			case(0): //HIJO
				fprintf(stderr, "\nPPID: %d|PID: %d",getppid(),getpid());
				redefinirSennal(SIGTERM,funcionCallBack);
			break;
			default: //PADRE
				esperarSigSuspend(SIGTERM);
		}	
	}

//MATAR EL ARBOL --> FUNCIONES DE CALLBACK
	void esperarMuerteHijo(int s){
		esperarPorHijo(pidHijo);
	    sigprocmask(SIG_SETMASK, &conjuntoLimpiador, NULL);
		exit(0);
	}

	void matarUnHijo(int s){
		mandarSennal(pidHijo,SIGTERM);
		esperarPorHijo(pidHijo);

	    sigprocmask(SIG_SETMASK, &conjuntoLimpiador, NULL);
		exit(0);
	}

	void matarDosHijos(int s){	
		mandarSennal(pidHijo,SIGTERM);
		mandarSennal(pidHijo2,SIGTERM);

		esperarPorHijo(pidHijo2);
		esperarPorHijo(pidHijo);

	    sigprocmask(SIG_SETMASK, &conjuntoLimpiador, NULL);
		exit(0);
	}

	void matarBastardo(int s){
		int pidHijoBastardo = leerPid(pLectura);
		mandarSennal(pidHijoBastardo,SIGTERM);
		//Estos no esperan por pid porque no tienen hijos
	    sigprocmask(SIG_SETMASK, &conjuntoLimpiador, NULL);
		exit(0);
	}

	void callBackUltimo(int s){
		desproyectarFichero();
		
	    sigprocmask(SIG_SETMASK, &conjuntoLimpiador, NULL);
		exit(0);
	} 

//SEÑALES
	void redefinirSennal(int sennal,void(*funcion)(int)){
		struct sigaction ss;	

		ss.sa_handler=funcion;
		ss.sa_flags=0;
		sigemptyset(&ss.sa_mask);
	    sigaddset(&ss.sa_mask,sennal);

		if(-1==sigaction(sennal,&ss,NULL)){
			char otros[15];
	    	sprintf(otros,"SEÑAL: %d",sennal);
			imprimirError("redefinirSennal","sigaction",otros,getpid());
		}
	}
	
	void esperarSigSuspend(int sennal){
	    sigset_t conjunto;

	    if( -1 == sigfillset(&conjunto) ) imprimirError("esperarSigSuspend","sigfillset",NULL,getpid());
	    if( -1 == sigdelset(&conjunto,sennal) ) imprimirError("esperarSigSuspend","sigdelset",NULL,getpid());

	    if( -1 == sigsuspend(&conjunto) ){
	    	char otros[15];
	    	sprintf(otros,"SEÑAL: %d",sennal);
			imprimirError("esperarSigSuspend","sigsuspend",otros,getpid());
	    }
	}

	void mandarSennal(int pidDestinatario,int sennal){
		fprintf(stderr,"\n%d --> %d  -- %d",getpid(),pidDestinatario,sennal);
		if(-1 == kill(pidDestinatario, sennal) ){
	    	char otros[50];
	    	sprintf(otros,"SEÑAL: %d y PID DESTINO: %d",sennal,pidDestinatario);
			imprimirError("esperarSigSuspend","sigsuspend",otros,getpid());
	    }
	}

	void esperarPorHijo(int pid){
		if(-1 == waitpid( pid,NULL,0)){
			imprimirError("esperarPorHijo","waitpid",NULL,getpid());
		}
		fprintf(stderr,"\nProceso %d ha terminado la espera por su hijo",getpid());
	}

//FICHEROS
	void escribirPid(int pid,int i){
		fprintf(stderr,"\n%d escribiendo %d en %d",getpid(),pid,i);
		switch(i){
			case(1): pb->p1 = pid; break;
			case(2): pb->p2 = pid; break;
			case(3): pb->p3 = pid; break;
		}
	}

	int leerPid(int i){
		fprintf(stderr,"\n%d leyendo %d : %d %d %d",getpid(),i,pb->p1,pb->p2,pb->p3);
		switch(i){
			case(1): return pb->p1; break;
			case(2): return pb->p2; break;
			case(3): return pb->p3; break;
		}
		
	}

	void proyectarFichero(){
		if( -1 == (descriptor = open("fichero", O_RDWR | O_CREAT, 0600)) ) imprimirError("poryectarFichero","open","fichero",getpid());
		

		//Escribir para que no de error
		int i = 0;
		write(descriptor,&i,sizeof(int));

		if( MAP_FAILED == (pb=(pids*)mmap(0, sizeof(pids), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0)) ){
			imprimirError("proyectarFichero","mmap",NULL,getpid());
		}

		//Dar valores iniciales
		pb->p1 = pb->p2 = pb->p3 = 0;
	}
	

	void desproyectarFichero(){
		if(-1 == munmap((void*)pb, sizeof(pids)) ) imprimirError("desPoryectarFichero","munmap",NULL,getpid());
		if(-1 == close(descriptor) ) imprimirError("desPoryectarFichero","close",NULL,getpid());
	}	


//AUXILIAR
	void imprimirError(char*posicionError,char*orden,char*otros,int pidProceso){
		char etiqueta [500];

		if( NULL==posicionError || NULL==orden ){
			sprintf(etiqueta,"\nPID: %d |ERROR: ",pidProceso);
		}else if( NULL==otros ){
	    	sprintf(etiqueta,"\nPID: %d |ERROR: IN %s | FOR ORDER: %s | OTHER: NULL | ",pidProceso,posicionError,orden);
		}else{
	    	sprintf(etiqueta,"\nPID: %d |ERROR: IN %s | FOR ORDER: %s | OTHER: %s |",pidProceso,posicionError,orden,otros);
		}

		perror(etiqueta);
		desPoryectarFichero();
		exit(-1);
	}
