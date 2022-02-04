#include "headers.h"
#include "Queue.h"
#include "LinkedList.h"
#include "PriorityQueue.h"

struct PCB * pcbPointer=NULL;
enum Status{Free, Busy};

void Merge(int address, int size);
void DeAllocate_memory(int address, int size);
int Allocate_memory(int size);

//memory management arrays creation 
//Initially the memory is divided into 4 partitions each have 256 bytes
//The values stored in the array refears to block's starting address in memory
int blocks_32[32];
int blocks_64[16];
int blocks_128[8];
int blocks_256[4] = {0, 256, 512, 768};

struct msgbuff
{
    long mtype;
    struct processData mData;
};

int quantum,t1,t2;
void Handler(int signum);

void ResumeProcess()
{
    printf("Procss with pid %d\n is resumed",pcbPointer->pid);
    kill(pcbPointer->pid,SIGCONT);
    pcbPointer->state=Running;
}

void Alarm_Handler(int signum)
{
    printf("From the alarm handler\n");
    kill(pcbPointer->pid,SIGSTOP);
}


int main(int argc, char * argv[])
{
    initClk();
    signal(SIGUSR1,Handler);
    signal(SIGALRM, Alarm_Handler);

    //Definitions
    int receive;
    int algorithmNo;
    int pcount;
    double sum_WTA=0;
    double sum_Waiting=0;
    double sum_RunningTime=0;
    key_t msgqid;
    struct msgbuff message;
    struct processData pData;
    struct PCB pcbBlock;
    struct node *pcbFind=NULL;
    
    //defining the output files for write
    FILE *logFile = fopen("scheduler.log", "w");
    FILE *prefFile = fopen("scheduler.pref", "w");
    FILE * sizeP = fopen("memory.log", "w");
    
    fprintf(logFile, "\n#At time x process y state arr w total z remain y wait k\n");

    //Initialize for memory blocks all other arrays with -1
    for (int i = 0; i < 32; i++)
        blocks_32[i]=-1;
    for (int i = 0; i < 16; i++)
        blocks_64[i]=-1;
    for (int i = 0; i < 8; i++)
        blocks_128[i]=-1;

    fprintf(sizeP, "\n#At time x allocated y bytes for process z from i to j\n");
    fprintf(logFile, "\n#At time x process y state arr w total z remain y wait k\n");
    

    //HPF
    PQueue HPF_readyQueue;
    PQueueInit(&HPF_readyQueue);
    //SRTN
    PQueue SRTN_readyQueue; 
    PQueueInit(&SRTN_readyQueue);
    //RR
    Queue RR_readyQueue;
    queueInit(&RR_readyQueue, sizeof(struct processData));
    //SJF
    PQueue SJF_readyQueue;
    PQueueInit(&SJF_readyQueue);

    //scan the count of the process
    sscanf(argv[2],"%d",&pcount);
    int Num_processes=pcount;
    double WTAArray[pcount];
    //Receive the processes from message queue and add to ready queue.
    msgqid = msgget(1000, 0644);
    sscanf(argv[1], "%d", &algorithmNo);
    printf("\nthe chosen algorithm is: %s\n",argv[1]);
    switch(algorithmNo)
    {
        //HPF
        case 1:
        receive= msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        pData = message.mData;
        printf("\nTime = %d process with id = %d recieved\n", getClk(), pData.id);
        
        push(&HPF_readyQueue, pData.priority, pData);
        
        pcbBlock.state = 0;
        pcbBlock.executionTime = 0;
        pcbBlock.remainingTime = pData.runningtime;
        pcbBlock.waitingTime = 0;
        insertFirst(pData.id, pcbBlock);
        printf("\nPCB created for process with id = %d\n", pData.id);

        while (pcount!=0)
        {
            printf("\nCurrent time = %d", getClk());
            receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            while (receive != -1)
            {
                pData = message.mData;
                printf("\nTime = %d process with id = %d recieved\n", getClk(), pData.id);
                push(&HPF_readyQueue, pData.priority, pData); //enqueue the data in the ready queue
                
                //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
            
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created for process with id = %d\n", pData.id);
                receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
                
            }
            pData=pop(&HPF_readyQueue);
            //find the pcb of the deueued process
            pcbFind = find(pData.id);
            pcbPointer = &(pcbFind->data);
            pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
            // write in the output file the process data
            fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);

            //forkprocess
            printf("\nIam forking a new process time = %d\n", getClk());
            int pid=fork();
            pcount--;
            pcbPointer->pid=pid;
            if (pid == -1)
            perror("error in fork");

            else if (pid == 0)
            {
                printf("\ntest the forking\n");
                char buf[20];
                sprintf(buf,"%d",pData.runningtime);
                char *argv[] = { "./process.out",buf, 0 };
                execve(argv[0], &argv[0], NULL);
            }
            sleep(1000);
            //Update pcb and calculate the process data after finishing it
            pcbPointer->executionTime=pData.runningtime;    
            pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
            pcbPointer->state=Finished;
            pcbPointer->remainingTime=0;
            int TA = getClk()-(pData.arrivaltime);
            double WTA=(double)TA/pData.runningtime;
            fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA, WTA);
            sum_WTA+=WTA;
            WTAArray[pData.id] = WTA;
            sum_Waiting+=pcbPointer->waitingTime;
            sum_RunningTime+=pcbPointer->executionTime;
        }

        break;




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////        
        
        


        //SRTN
        case 2:

        //Recieving the processes sent by the process generator
        receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        printf("\nProcess with Pid = %d recieved\n",message.mData.id);
        pData = message.mData;
        push(&SRTN_readyQueue, pData.runningtime, pData); //enqueue the data in the ready queue
        if(receive!=-1)
        {
            //Creating PCB
            pcbBlock.state = 0;
            pcbBlock.executionTime = 0;
            pcbBlock.remainingTime = pData.runningtime;
            pcbBlock.waitingTime = 0;
            
            insertFirst(pData.id, pcbBlock);
            printf("\nPCB created for process with Pid = %d\n",pData.id);
        }

        while(getlength(&SRTN_readyQueue)!=0)
        {
            receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            while(receive!=-1)
            {
                printf("\n Current time is: %d\n", getClk());
                printf("\nProcess with Pid = %d recieved\n",message.mData.id);
                pData = message.mData;
                push(&SRTN_readyQueue, pData.runningtime, pData);
            
                //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;   
                insertFirst(pData.id, pcbBlock);
                
                printf("\nPCB created for process with Pid = %d\n",pData.id);
                receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            }

            pData = pop (&SRTN_readyQueue);
            if(pData.id==-1){  //if the process is finished skip this iteration 
                continue;
            }
            //find the PCB of the poped process
            pcbFind = find(pData.id);
            pcbPointer = &(pcbFind->data);
            
            if(pcbPointer->state == Waiting)
            {
                fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                ResumeProcess();
                printf("\nCurrent time is: %d\n", getClk());
                alarm(1); 
                sleep(50);

                if(pcbPointer->state!=Finished)
                {
                    printf("\nThe process with Pid = %d is not finished yet \n",pcbPointer->pid);
                    push(&SRTN_readyQueue,pcbPointer->remainingTime , pData);
                    //Update PCB
                    pcbPointer->executionTime+=1;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->state=Waiting;
                    pcbPointer->remainingTime-=1;
                   
                    fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                }
                else
                {
                    pcbPointer->executionTime=pData.runningtime;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->remainingTime=0;                    
                    int TA = getClk()-(pData.arrivaltime);                    
                    double WTA=(double)TA/pData.runningtime;

                    fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", 
                        getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA,WTA);
                    
                    sum_WTA+=WTA;
                    sum_Waiting+=pcbPointer->waitingTime;
                    sum_RunningTime+=pcbPointer->executionTime;                            
                    pData.id=-1;
                }
            }    
            else
            {
                //Fork the process
                printf("\nIam forking a new process \n");
                int pid=fork();
                pcbPointer->pid=pid;
                if (pid == -1)
                    perror("error in forking");
                else if (pid == 0)
                {
                    printf("\ntesting the fork\n");
                    char buf[100];
                    sprintf(buf,"%d",pData.runningtime);
                    char *argv[] = { "./process.out",buf, 0 };
                    execve(argv[0], &argv[0], NULL);
                }

                pcount--;
                pcbPointer->waitingTime=getClk()-(pData.arrivaltime);
                printf("\nAt time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                alarm(1);
                sleep(50);  

                if(pcbPointer->state!=Finished)
                {
                    printf("Not Finished\n");
                    push (&SRTN_readyQueue, pcbPointer->remainingTime, pData);

                    pcbPointer->executionTime+=1;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->state=Waiting;
                    pcbPointer->remainingTime-=1;

                    fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                }
                else
                {
                    pcbPointer->executionTime=pData.runningtime;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->remainingTime=0;
                    int TA = getClk()-(pData.arrivaltime);
                    double WTA=(double)TA/pData.runningtime;

                    fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA, WTA);
 
                    sum_WTA+=WTA;
                    WTAArray[pData.id] = WTA;
                    sum_Waiting+=pcbPointer->waitingTime;
                    sum_RunningTime+=pcbPointer->executionTime;      
                    pData.id=-1;
                        
                }
            }   
        } 

        break;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




        //RR
        case 3:
        receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        printf("\nRecieved rec val is: %d \n",receive);
        pData = message.mData;
        enqueue(&RR_readyQueue, &pData); //enqueue the data in the ready queue
        printf("\n%d %d %d %d\n",pData.id,pData.arrivaltime,pData.runningtime,pData.priority);
        if(receive!=-1)
            {
                //Allocate Memory
                //Get the power of 2 value of size
                int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                int address = Allocate_memory(size);
                fprintf(sizeP, "\nAt time %d allocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, address,address+size-1);


                //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
                pcbBlock.mem_address=address;
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created\n");
            }
        
        while(getQueueSize(&RR_readyQueue)!=0 || pcount!=0)
        {   
            receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            printf("from the receive at the end of the while loop %d\n",receive);
            while(receive!=-1)
            {
                printf("\n Current time is: %d\n", getClk());
                printf("\nRecieved rec val is: %d \n",receive);
                pData = message.mData;
                enqueue(&RR_readyQueue, &pData);
                printf("\n%d %d %d %d\n",pData.id,pData.arrivaltime,pData.runningtime,pData.priority);
                
                //Allocate Memory
                //Get the power of 2 value of size
                int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                int address = Allocate_memory(size);
                fprintf(sizeP, "\nAt time %d allocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, address,address+size-1);

                 //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
                pcbBlock.mem_address=address;
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created\n");
                receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            }
            
            sscanf(argv[3], "%d", &quantum);
            dequeue(&RR_readyQueue, &pData); //dequeue a process to run
            if(pData.id==-1){  //if the process is finished skip this iteration 
                continue;
            }
            printf("id is %d\n",pData.id);
            //find the PCB of the dequeued process
            pcbFind = find(pData.id);
            pcbPointer = &(pcbFind->data);
            printf("the state of the pcb i found is %d\n",pcbPointer->state);
            
            if(pcbPointer->state==Waiting)
            {

                pcbPointer->waitingTime = getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                sum_Waiting+=pcbPointer->waitingTime;
                fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                ResumeProcess();
                printf("\nCurrent time before alarm is: %d\n", getClk());
                alarm(quantum);
                sleep(50);
                printf("\nCurrent time after alarm is: %d\n", getClk());

                
                if(pcbPointer->state!=Finished)
                {
                    enqueue(&RR_readyQueue,&pData);
                    //Update PCB
                    
                    pcbPointer->executionTime+=quantum;
                    pcbPointer->state=Waiting;
                    pcbPointer->remainingTime-=quantum;
                   
                    fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                    
                }
                else
                {
                    pcbPointer->executionTime=pData.runningtime;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->remainingTime=0;
                    int TA = getClk()-(pData.arrivaltime);
                    double WTA=(double)TA/pData.runningtime;
                    fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", 
                        getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA,WTA);
                    sum_WTA+=WTA;
                    WTAArray[pData.id] = WTA;
                    sum_Waiting+=pcbPointer->waitingTime;
                    sum_RunningTime+=pcbPointer->executionTime;

                   //Deallocate memory for the finished process
                    int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                    DeAllocate_memory(pcbPointer->mem_address,size);
                    fprintf(sizeP, "\nAt time %d deallocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, pcbPointer->mem_address, pcbPointer->mem_address+size-1);
                    pData.id=-1;

                }
                
            }
            else
            {
                //Fork the process
                printf("\n I am forking a new process now\n");
                int pid=fork();
                pcount--;
                pcbPointer->pid=pid;
                if (pid == -1)
                perror("error in fork");

                else if (pid == 0)
                {
                    printf("\ntesting the fork\n");
                    char buf[100];
                    sprintf(buf,"%d",pData.runningtime);
                    char *argv[] = { "./process.out",buf, 0 };
                    execve(argv[0], &argv[0], NULL);
                    
                }
                printf("I am setting the alarm for: %d \n",quantum);
                t1=getClk();
                printf("\nCurrent time before alarm is: %d\n", getClk());
                if (pData.arrivaltime==getClk()) pcbPointer->waitingTime=0;
                else pcbPointer->waitingTime = getClk()-pData.arrivaltime;

                fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                
                
                alarm(quantum);
                sleep(500);
                printf("\nCurrent time after alarm is: %d\n", getClk());
                
                //Update PCB
                
                if(pcbPointer->state!=Finished)
                {
                    printf("Not Finished\n");
                    enqueue(&RR_readyQueue,&pData);
                    if (pcbPointer->remainingTime>quantum)
                    {
                        
                        pcbPointer->executionTime+=quantum;
                        pcbPointer->state=Waiting;
                        pcbPointer->remainingTime-=quantum;
                        fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);
                        
                    }
                    else 
                    {
                        pcbPointer->executionTime += pcbPointer->remainingTime;
                        pcbPointer->remainingTime=0;
                        int TA = getClk()-(pData.arrivaltime);
                        double WTA=(double)TA/pData.runningtime;
                        fprintf(logFile, "2At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", 
                            getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA, WTA);
                        sum_WTA+=WTA;
                        WTAArray[pData.id] = WTA;
                        sum_Waiting+=pcbPointer->waitingTime;
                        sum_RunningTime+=pcbPointer->executionTime;

                        //Deallocate memory for the finished process
                        int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                        DeAllocate_memory(pcbPointer->mem_address,size);
                        fprintf(sizeP, "\nAt time %d deallocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, pcbPointer->mem_address, pcbPointer->mem_address+size-1);
                        pData.id=-1;


                    }
                    
                }
                else
                {
                    pcbPointer->executionTime=pData.runningtime;
                    pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
                    pcbPointer->remainingTime=0;
                    int TA = getClk()-(pData.arrivaltime);
                    double WTA=(double)TA/pData.runningtime;
                    fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", 
                        getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA, WTA);
                    sum_WTA+=WTA;
                    WTAArray[pData.id] = WTA;
                    sum_Waiting+=pcbPointer->waitingTime;
                    sum_RunningTime+=pcbPointer->executionTime;

                   //Deallocate memory for the finished process
                    int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                    DeAllocate_memory(pcbPointer->mem_address,size);
                    fprintf(sizeP, "\nAt time %d deallocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, pcbPointer->mem_address, pcbPointer->mem_address+size-1);
                    pData.id=-1;

                }
            }
           
        }
    
       
        break;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //SJF
        case 4:
        receive= msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        pData = message.mData;
        printf("\nTime = %d process with id = %d recieved\n", getClk(), pData.id);
        

        //enqueue the process in the ready queue
        push(&SJF_readyQueue, pData.runningtime, pData);
       
        //Allocate Memory
        //Get the power of 2 value of size
        int size = pow(2,ceil(log(pData.sizeP)/log(2)));
        int address = Allocate_memory(size);
        fprintf(sizeP, "\nAt time %d allocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, address,address+size);

        //Create PCB
        pcbBlock.state = 0;
        pcbBlock.executionTime = 0;
        pcbBlock.remainingTime = pData.runningtime;
        pcbBlock.waitingTime = 0;
        pcbBlock.mem_address = address;
        insertFirst(pData.id, pcbBlock);
        printf("\nPCB created for process with id = %d\n", pData.id);

        while (pcount!=0)
        {
            printf("\nThe current time = %d\n", getClk());
            receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            while (receive != -1)
            {
                pData = message.mData;
                printf("\nTime = %d process with id = %d recieved\n", getClk(), pData.id);
                push(&SJF_readyQueue, pData.runningtime, pData); //enqueue the data in the ready queue
                //Allocate Memory
                //Get the power of 2 value of size
                int size = pow(2,ceil(log(pData.sizeP)/log(2)));
                int address = Allocate_memory(size);
                fprintf(sizeP, "\nAt time %d allocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, address,address+size);

                //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
                pcbBlock.mem_address = address;
            
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created for process with id = %d\n", pData.id);
                receive = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
                
            }
            pData=pop(&SJF_readyQueue);
            //find the pcb of the poped process
            pcbFind = find(pData.id);
            pcbPointer = &(pcbFind->data);
            pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
            //write in the Log file the process data
            fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime);

            //fork the process
            printf("\nIam forking a new process time = %d\n", getClk());
            int pid=fork();
            pcount--;
            pcbPointer->pid=pid;
            if (pid == -1)
            perror("error in forking");

            else if (pid == 0)
            {
                printf("\ntest the forking\n");
                char buf[20];
                sprintf(buf,"%d",pData.runningtime);
                char *argv[] = { "./process.out",buf, 0 };
                execve(argv[0], &argv[0], NULL);
            }
            sleep(50);
            //Update pcb and calculate the process data after finishing it
            pcbPointer->executionTime=pData.runningtime;
            pcbPointer->waitingTime=getClk()-(pData.arrivaltime)-(pcbPointer->executionTime);
            pcbPointer->state=Finished;
            pcbPointer->remainingTime=0;
            int TA = getClk()-(pData.arrivaltime);
            double WTA=(double)TA/pData.runningtime;
            fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), pData.id, pData.arrivaltime, pData.runningtime, pcbPointer->remainingTime, pcbPointer->waitingTime, TA, WTA);
            sum_WTA+=WTA;
            WTAArray[pData.id] = WTA;
            sum_Waiting+=pcbPointer->waitingTime;
            sum_RunningTime+=pcbPointer->executionTime;
            //Deallocate the memory 
            size = pow(2,ceil(log(pData.sizeP)/log(2)));
            DeAllocate_memory(pcbPointer->mem_address, size);
            fprintf(sizeP, "\nAt time %d deallocated %d bytes for process %d from %d to %d\n", getClk(), pData.sizeP, pData.id, pcbPointer->mem_address,pcbPointer->mem_address+size);

        }
        break;

    }
    fclose(logFile);
    double stdWTA = 0.00;
    double avgWTA =(sum_WTA/Num_processes);
    for(int i = 0; i < Num_processes; i++){
        stdWTA += pow((WTAArray[i]-avgWTA),2);
    }
    fprintf(prefFile,"CPU Utilization = %.2f %%\n",(sum_RunningTime/getClk())*100);
    fprintf(prefFile,"Avg WTA = %.2f\n",(sum_WTA/Num_processes));
    fprintf(prefFile,"Avg Waiting = %.2f\n",sum_Waiting/Num_processes);
    fprintf(prefFile,"Std WTA = %.2f\n",sqrt(stdWTA/Num_processes));
    fclose(prefFile);
 
    //upon termination release the clock resources
    
    destroyClk(false);

    

}


void Handler(int signum)
{
    printf("Handler started\n");
    printf("from sig child pid is %d\n",pcbPointer->pid);
    int pid,stat_loc;
    pid = wait(&stat_loc);
    t2=getClk();
    if(WIFEXITED(stat_loc)){
        if(WEXITSTATUS(stat_loc)==0)
        {
            printf("\nProcess Finished\n");
            pcbPointer->state=Finished;
            pcbPointer->executionTime+=(t2-t1);
            pcbPointer->remainingTime=0;
        }
    }
}

int Allocate_memory(int size)
{
    switch (size)
    {
    case 32:
        for (int i = 0; i < 32; i++)
        {
            if(blocks_32[i]!=-1)
            {
                blocks_32[i]=-1;
                return i*size;
            }

        }


        for (int i = 0; i < 16; i++)
        {
            if(blocks_64[i]!=-1)
            {
                blocks_32[2*i+1]=(2*i+1)*size;
                blocks_64[i]=-1;
                return (2*i)*size;
            }

        }

        for (int i = 0; i < 8; i++)
        {
            if(blocks_128[i]!=-1)
            {
                blocks_64[2*i+1]=(2*i+1)*2*size;
                blocks_32[4*i+1]=(4*i+1)*size;
                blocks_128[i]=-1;
                return (4*i)*size;
            }

        }
        for (int i = 0; i < 4; i++)
        {
            if(blocks_256[i]!=-1)
            {
                blocks_128[2*i+1]=(2*i+1)*4*size;
                blocks_64[4*i+1]=(4*i+1)*2*size;
                blocks_32[8*i+1]=(8*i+1)*size;
                blocks_256[i]=-1;
                return (8*i)*size;
            }
        }

        printf("\n\n Warning! There's no enough space in memory for this process\n");
        
        break;

    case 64:
        for (int i = 0; i < 16; i++)
        {
            if(blocks_64[i]!=-1)
            {

                blocks_64[i]=-1;
                return (i)*size;
            }
        }
        for (int i = 0; i < 8; i++)
        {
            if(blocks_128[i]!=-1)
            {

                blocks_64[2*i+1]=(2*i+1)*size;
                blocks_128[i]=-1;
                return (2*i)*size;
            }
        }
        for (int i = 0; i < 4; i++)
        {
            if(blocks_256[i]!=-1)
            {

                blocks_128[2*i+1]=(2*i+1)*2*size;
                blocks_64[4*i+1]=(4*i+1)*size;
                blocks_256[i]=-1;
                return (4*i)*size;
            }
        }
        printf("\n\n Warning! There's no enough space in memory for this process\n");
        break;

    case 128:
        for (int i = 0; i < 8; i++)
        {

            if(blocks_128[i]!=-1)
            {
                blocks_128[i]=-1;
                return (i)*size;
            }
        }
        for (int i = 0; i < 4; i++)
        {

            if(blocks_256[i]!=-1)
            {
                blocks_128[2*i+1]=(2*i+1)*size;
                blocks_256[i]=-1;
                return (2*i)*size;
            }
        }
        printf("\n\n Warning! There's no enough space in memory for this process\n");
        break;

    case 256:
        for (int i = 0; i < 4; i++)
        {

            if(blocks_256[i]!=-1)
            {
                blocks_256[i]=-1;
                return (i)*size;
            }
        }
        printf("\n\n Warning! There's no enough space in memory for this process\n");
        break;

    }
}

void DeAllocate_memory(int address, int size)
{
    int ratio=address/size;
    switch (size)
    {
    case 32:


        blocks_32[ratio]=address;

        break;
    
    case 64:
        blocks_64[ratio]=address;

        break;
    
    case 128:

        blocks_128[ratio]=address;

        break;

    case 256:

        blocks_256[ratio]=address;

        break;
    }

    printf("\n [256] Memory Blocks: \n");
    for (int i = 0; i < 4 ; i++) printf("%d\t", blocks_256[i]);

    printf("\n[128] Memory Blocks:\n");
    for (int i = 0; i < 8; i++) printf("%d\t", blocks_128[i]);

    printf("\n[64] Memory Blocks:\n");
    for (int i = 0; i < 16; i++) printf("%d\t", blocks_64[i]);

    printf("\n[32] Memory Blocks:\n");
    for (int i = 0; i < 32; i++) printf("%d\t", blocks_32[i]);

    printf("\n\n");


    Merge(address, size);


    printf("\n [256] Memory Blocks: \n");
    for (int i = 0; i < 4 ; i++) printf("%d\t", blocks_256[i]);

    printf("\n[128] Memory Blocks:\n");
    for (int i = 0; i < 8; i++) printf("%d\t", blocks_128[i]);

    printf("\n[64] Memory Blocks:\n");
    for (int i = 0; i < 16; i++) printf("%d\t", blocks_64[i]);

    printf("\n[32] Memory Blocks:\n");
    for (int i = 0; i < 32; i++) printf("%d\t", blocks_32[i]);

    printf("\n\n");
}

void Merge(int address, int size)
{
    //Implementation of merging memory blocks goes here
    switch (size)
    {
    case 32:
        for (int i = 0; i< 32; i+=2)
        {
          if (blocks_32[i] != -1 && blocks_32[i+1] != -1)
          {
           blocks_64[i/2]= i*size;
           blocks_32[i] = -1;
           blocks_32[i+1]=-1;
           printf("\nmemory 32 merged successfully\n");
          }
        } 

        for (int i = 0; i < 16; i+=2)
        {
            if (blocks_64[i] != -1 && blocks_64[i+1] != -1)
            {
                blocks_128[i/2] = 2*i*size;
                blocks_64[i] = -1;
                blocks_64[i+1] = -1;
                printf("\nmemory 64 merged successfully\n");
            }
        }

        for (int i = 0; i < 8; i +=2)
        {
            if (blocks_128[i] != -1 && blocks_128[i+1] != -1)
            {
                blocks_256[i/2] = 4*i*size;
                blocks_128[i] = -1;
                blocks_128[i+1] = -1;
                printf("\nmemory 128 merged successfully\n");
            }
        }
        break;
    
    case 64:
        for (int i = 0; i < 16; i+= 2)
        {
            if (blocks_64[i] != -1 && blocks_64[i+1] != -1)
            {
                blocks_128[i/2] = i*size;
                blocks_64[i] = -1;
                blocks_64[i+1] = -1;
                printf("\nmemory 64 merged successfully\n");
            }
        }

          for (int i = 0; i < 8; i +=2)
        {
            if (blocks_128[i] != -1 && blocks_128[i+1] != -1)
            {
                blocks_256[i/2] = 2*i*size;
                blocks_128[i] = -1;
                blocks_128[i+1] = -1;
                printf("\nmemory 128 merged successfully\n");
            }
        }

        break;

    case 128:
        for (int i = 0; i < 8; i +=2)
        {
            if (blocks_128[i] != -1 && blocks_128[i+1] != -1)
            {
                blocks_256[i/2] = i*size;
                blocks_128[i] = -1;
                blocks_128[i+1] = -1;
                printf("\nmemory 128 merged successfully\n");
            }
        }
        break;
    }

}
