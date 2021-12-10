#include <time.h>
#include <unistd.h>
#include "headers.h"
#include "Queue.h"
#define null 0

key_t msgqid;
struct processData //define process data
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int memsize;
};

struct msgbuff
{
    long mtype;
    struct processData mData;
};

void clearResources(int); //function to clear ipc and clock recources


int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    //Initialization

    FILE * pFile; //define a pointer to the file
    struct processData pData;
    int send_val;
    struct msgbuff message;
    msgqid = msgget(1000, IPC_CREAT | 0644); //create a message queue
    //define the input queue
    Queue Input_Queue;
    queueInit(&Input_Queue,sizeof(struct processData));

    // 1. Read the input files.
    pFile = fopen("processes.txt", "r");
    while (!feof(pFile))
    {
        fscanf(pFile,"%d\t%d\t%d\t%d\n", &pData.id, &pData.arrivaltime, &pData.runningtime, &pData.priority);
        enqueue(&Input_Queue,&pData);
        
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters.

    int algorithmNo;
    int quantum;
    int error;
    do
    {
        printf("What scheduling algorithm do you want?\n 1- HPF\n 2- SRTN\n 3- RR\n ");
        scanf("%d", &algorithmNo);
        char pCount[100];
        sprintf(pCount,"%d",getQueueSize(&Input_Queue));
        error = 0;
        int pid1 = fork(); //fork the scheduler process
        printf("\nPID1 is: %d\n",pid1);
        if(algorithmNo == 3)
        { 
            printf("Please Enter the quantum duration: \n");
            scanf("%d", &quantum);
        }
        switch (algorithmNo)
        {
            //HPF
            case 1:
            {
                if (pid1 == -1)
                    perror("error in fork");

                else if (pid1 == 0)
                {
                    char *argv[] = { "./scheduler.out","1", pCount, 0 };
                    execve(argv[0], &argv[0], NULL); //run the scheduler
                }

                break;
            }
            //SRTN
            case 2:
            {
                if (pid1 == -1)
                    perror("error in fork");

                else if (pid1 == 0)
                {
                    char *argv[] = { "./scheduler.out","2", pCount, 0 };
                    execve(argv[0], &argv[0], NULL); //run the scheduler
                }
                break;
            }
            //RR
            case 3:
            {
                if (pid1 == -1)
                perror("error in fork");

                else if (pid1 == 0)
                {
                    printf("\ntest case 2\n");
                    char buf[100];
                    sprintf(buf,"%d",quantum);
                    char *argv[] = { "./scheduler.out", "3",pCount,buf, 0 };
                    execve(argv[0], &argv[0], NULL);
                }
                break;
            }
            default:
            {
                error = 1;
                printf("Input error, please try again...");
            }
        }
    } while (error == 1);
    

    // 3. Initiate and create the scheduler and clock processes.
   
    int pid2 = fork(); //fork the clock process
    if (pid2 == -1)
        perror("error in fork");
       
    else if (pid2 == 0)
    {
        char *argv[] = { "./clk.out", 0 };
        execve(argv[0], &argv[0], NULL); //run the clock
    }

    //4. Initializing clock
    initClk();
    
    if(msgqid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    while(getQueueSize(&Input_Queue))
    {
        int x = getClk();
        queuePeek(&Input_Queue,&pData); //peek the first entry in the input queue
        //send the process at the appropriate time.
        while ((pData.arrivaltime<x)||(pData.arrivaltime==x))
        {
        dequeue(&Input_Queue,&pData);
        printf("current time is %d\n", x);
        message.mtype = 7;     	/* arbitrary value */
        //message.mData = pData;
        message.mData.arrivaltime=pData.arrivaltime;
        message.mData.id=pData.id;
        message.mData.priority=pData.priority;
        message.mData.runningtime=pData.runningtime;
        message.mData.memsize=pData.memsize;
        send_val = msgsnd(msgqid, &message, sizeof(message.mData), IPC_NOWAIT);
        
        //printf("\n%d %d %d %d\n",pData.id,pData.arrivaltime,pData.runningtime,pData.priority);
        printf("\nprocess with id = %d sent\n", pData.id);
        if(send_val == -1)
            perror("Errror in send");
        pData.arrivaltime=x+5;
        queuePeek(&Input_Queue,&pData);
        }
    }
    // 7. Wait for the scheduler to exit
    int status;
    int pid = wait(&status);
    if(!(status & 0x00FF))
        printf("\nA scheduler with pid %d terminated with exit code %d\n", pid, status>>8);

    destroyClk(true);
}

void clearResources(int signum)
{
    //Clears all resources in case of interruption
    msgctl(msgqid, IPC_RMID, (struct msqid_ds *) 0); 
    exit(0);
}
