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
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX 10000

#define NUM_ONE_CHILD 16
#define NUM_TWO_CHILD 3
#define NUM_SPECIAL 3

#define SHARED_FILE_NAME "communication_file.bin"
#define FILE_ERROR "Error related to communication file treatement."
#define CHILD_CREATION_ERROR "Error related to child creation."
#define SEMAPHORE_ERROR "Error related to semaphore."
#define SIGNAL_ERROR "Error related to signal treatement."

#define isChild(value) ((value) == 0)
#define isFather(value) ((value) != 0)

#define EXIT_IF_WRONG_VALUE(value, wrongValue, errorMsg)										\
	do{																							\
		char errorTag[MAX];																		\
		if( (value) == (wrongValue) ){															\
			sprintf(errorTag,"\n[%s:%d:%s] # %s # ",__FILE__,__LINE__,__FUNCTION__,errorMsg);	\
			perror(errorTag);																	\
		}																						\
	}while(0) 


#define SEMOP(sem, quantity) 											\
	do{																	\
	    struct sembuf sops;												\
	    sops.sem_op = quantity;											\
	    sops.sem_num = sops.sem_flg = 0;								\
	    EXIT_IF_WRONG_VALUE( semop(sem,&sops,1), 0, SEMAPHORE_ERROR );	\
	}while(0)

#define WAIT(sem) SEMOP(sem,-1)
#define SIGNAL(sem) SEMOP(sem,1)

struct oneChildEntry{
	pid_t father;
	pid_t child;
};
struct twoChildEntry{
	pid_t father;
	pid_t child;
	pid_t child2;
};
struct specialEntry{
	pid_t killer;
	pid_t victim;
};
typedef struct __typePidTable{
	struct oneChildEntry oneChild[NUM_ONE_CHILD];
	struct twoChildEntry twoChild[NUM_TWO_CHILD];
	struct specialEntry special[NUM_SPECIAL];
	pid_t fatherPid;
}typePidTable;


typedef enum __typePidTableEntry { ONE_CHILD, TWO_CHILD, SPECIAL_CHILD, SPECIAL_FATHER } typePidTableEntry;
typedef enum __familyPosition {CHILD, FATHER} familyPosition;