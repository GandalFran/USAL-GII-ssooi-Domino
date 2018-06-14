/**
*	@autor: Francisco Pinto Santos
*
*	Sistemas Operativos I - Primera práctica evaluable:
*		http://avellano.usal.es/~ssooi/pract118.htm
*/

#include "header.h"

struct __global{
	int memorySem;
	int descriptor;
	typePidTable*pidTable;
}global;

void createTree(void);
	void branchA(void);
	void branchB(void);
	void branchC(void);
	void branchD(void);
int createChild(typePidTableEntry tableEntry, int stayBlocked);
void createTableEntry(typePidTableEntry tableEntry, int childPid);
void createSpecialEntry(int pos, familyPosition who);
void unlockSignalsAndWait(void);
void killHandler(int ss);
int lookForVictim(pid_t*victims);

int main(void){
	int i;
	int descriptor;
	sigset_t initialSet;

	//Communication file
	EXIT_IF_WRONG_VALUE( global.descriptor = open(SHARED_FILE_NAME, O_RDWR | O_CREAT, 0600), 0, FILE_ERROR );
	EXIT_IF_WRONG_VALUE( write(descriptor, &i, sizeof(int)), 0, FILE_ERROR );
	EXIT_IF_WRONG_VALUE( global.pidTable=(typePidTable*)mmap(0, sizeof(typePidTable), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0), 0, FILE_ERROR );

	memset(global.pidTable,0,sizeof(typePidTable));

	//Semaphore to control de acces to file
	EXIT_IF_WRONG_VALUE( global.memorySem = semget(IPC_PRIVATE, 1, IPC_CREAT|0600), 0, SEMAPHORE_ERROR );
	EXIT_IF_WRONG_VALUE( semctl(global.memorySem,0,SETVAL,1), 0, SEMAPHORE_ERROR );

	//Block all signals
	EXIT_IF_WRONG_VALUE( sigfillset(&initialSet), 0, SIGNAL_ERROR );
	EXIT_IF_WRONG_VALUE( sigprocmask(SIG_SETMASK,&initialSet,NULL), 0,SIGNAL_ERROR );

	createTree();
}

void createTree(void){
	int ret;
	global.pidTable->fatherPid = getpid();

	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	
	ret = createChild(TWO_CHILD,0);
	if(isFather(ret)){
		createChild(TWO_CHILD,1);
		/*Child 2*/
		if(isFather(ret)){
			createChild(TWO_CHILD,1);
			/*Child 2.2*/
			branchD();
		}else{/*Child 2.1*/
			branchC();
		}
	}else{/*Child 1*/
		if(isFather(ret)){
			createChild(TWO_CHILD,1);
			/*Child 1.2*/
			branchB();
		}else{/*Child 1.1*/
			branchA();
		}
	}
}

void branchA(void){
	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	createChild(SPECIAL_FATHER,1);
	//TODO->registrarEspecialVicitima -> 0
		createSpecialEntry(0,CHILD);
	createChild(SPECIAL_FATHER,1);
	//TODO->registrarEspecialVicitima -> 3
		createSpecialEntry(3,CHILD);
	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	unlockSignalsAndWait();
}
void branchB(void){
	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	//TODO->registrarEspecialAsesino -> 0; + esperar a que victima mande mensaje
		createSpecialEntry(0,FATHER);
	unlockSignalsAndWait();
}
void branchC(void){
	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	createChild(SPECIAL_FATHER,1);
	//TODO->registrarEspecialVicitima -> 2
		createSpecialEntry(2,CHILD);
	//TODO->registrarEspecialAsesino -> 3; + esperar a que victima mande mensaje
		createSpecialEntry(3,FATHER);
	unlockSignalsAndWait();
}
void branchD(void){
	createChild(ONE_CHILD,1);
	createChild(ONE_CHILD,1);
	//TODO->registrarEspecialAsesino -> 2; + esperar a que victima mande mensaje
		createSpecialEntry(2,FATHER);
	unlockSignalsAndWait();
}

int createChild(typePidTableEntry tableEntry, int stayBlocked){
	int childPid;

	EXIT_IF_WRONG_VALUE( childPid = fork(), 0, CHILD_CREATION_ERROR );

	if(isFather(childPid)){
		createTableEntry(tableEntry,childPid);
		if(stayBlocked)
			unlockSignalsAndWait();
		else
			return 1;
	}

	return 0;
}

/*TODO --> de alguna manera hay que hacer que cuando se aun nodo especial registre su pid y sus mierdas en la tabla antes de continuar*/
void createTableEntry(typePidTableEntry tableEntry, int childPid){
	int i;

	WAIT(global.memorySem);

	switch(tableEntry){
		case ONE_CHILD:
			for(i=0; i<NUM_ONE_CHILD; i++){
				if(0 == global.pidTable->oneChild[i].father){
					global.pidTable->oneChild[i].father = getpid();
					global.pidTable->oneChild[i].child = childPid;
					break;
				}
			}
		break;
		case TWO_CHILD:
			for(i=0; i<NUM_TWO_CHILD; i++){
				if( getpid() == global.pidTable->twoChild[i].father ){
						global.pidTable->twoChild[i].child2 = childPid;
						break;
				}else if( 0 == global.pidTable->twoChild[i].father ){
					global.pidTable->twoChild[i].father = getpid();
					global.pidTable->twoChild[i].child = childPid;
					break;
				}
			}
		break;
		case SPECIAL_FATHER: /*For the father of the specials*/
			for(i=0; i<NUM_ONE_CHILD; i++){
				if(0 == global.pidTable->oneChild[i].father){
					global.pidTable->oneChild[i].father = getpid();
					global.pidTable->oneChild[i].child = -1;
					break;
				}
			}
		break;
	}

	SIGNAL(global.memorySem);
}

void createSpecialEntry(int pos, familyPosition who){
	sigset_t set;

	sigfillset(&set);
	sigdelset(&set,SIGUSR1);

	switch(who){
		case CHILD:
			WAIT(global.memorySem);
				global.pidTable->special[pos].victim = getpid(); 
				if( 0 == global.pidTable->special[pos].killer ){
					sigsuspend(&set);
					kill(global.pidTable->special[pos].victim,SIGUSR1);
				}else{
					kill(global.pidTable->special[pos].killer,SIGUSR1);
					sigsuspend(&set);
				}
			SIGNAL(global.memorySem);
		break;
		case FATHER: 
			WAIT(global.memorySem); 
				global.pidTable->special[pos].killer = getpid();  
				if( 0 == global.pidTable->special[pos].victim ){
					sigsuspend(&set);
					kill(global.pidTable->special[pos].victim,SIGUSR1);
				}else{
					kill(global.pidTable->special[pos].victim,SIGUSR1);
					sigsuspend(&set);
				}
			SIGNAL(global.memorySem);
		break;
	}
}

void unlockSignalsAndWait(void){
	sigset_t set;
	struct sigaction ss;

	ss.sa_handler=killHandler;
	ss.sa_flags=0;
	EXIT_IF_WRONG_VALUE( sigfillset(&ss.sa_mask), 0, SIGNAL_ERROR );
	EXIT_IF_WRONG_VALUE( sigaction(SIGTERM,&ss,NULL), 0, SIGNAL_ERROR );

	EXIT_IF_WRONG_VALUE( sigfillset(&set), 0, SIGNAL_ERROR );
	EXIT_IF_WRONG_VALUE( sigdelset(&set,SIGTERM), 0, SIGNAL_ERROR );
	EXIT_IF_WRONG_VALUE( sigsuspend(&set), 0, SIGNAL_ERROR );

}

void killHandler(int ss){
	int numChilds, i;
	pid_t toKill[2] = {-1, -1};

	numChilds = lookForVictim(toKill);

	for(i=0; i<numChilds; i++){
		EXIT_IF_WRONG_VALUE( kill(toKill[i],SIGTERM), 0, SIGNAL_ERROR );
		EXIT_IF_WRONG_VALUE( waitpid(toKill[i],NULL,0), 0, SIGNAL_ERROR );
	}

	if( getpid() == global.pidTable->fatherPid ){
		EXIT_IF_WRONG_VALUE( semctl(global.memorySem,0,IPC_RMID), 0, SEMAPHORE_ERROR);
		EXIT_IF_WRONG_VALUE( munmap((void*)global.pidTable, sizeof(typePidTable)), 0, FILE_ERROR );
		EXIT_IF_WRONG_VALUE( close(global.descriptor), 0,  FILE_ERROR );
	}

	exit(EXIT_SUCCESS);
}

int lookForVictim(pid_t*victims){
	int i;

	for(i=0; i<NUM_SPECIAL; i++){
		if( getpid() == global.pidTable->special[i].killer){
			victims[0] = global.pidTable->special[i].victim;
			return 1;
		}
	}

	for(i=0; i<NUM_ONE_CHILD; i++){
		if( getpid() == global.pidTable->oneChild[i].father ){
			if( -1 != global.pidTable->oneChild[i].child ){
				victims[0] = global.pidTable->oneChild[i].child;
				return 1;
			}else{
				return 0;
			}
		}
	}

	for(i=0; i<NUM_TWO_CHILD; i++){
		if( getpid() == global.pidTable->twoChild[i].father ){
			victims[0] = global.pidTable->twoChild[i].child;
			victims[1] = global.pidTable->twoChild[i].child2;
			return 2;
		}
	}

	return 0;
}