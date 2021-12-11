#include "headers.h"
#include "Queue.h"
#include "LinkedList.h"
#include "PriorityQueue.h"

struct PCB * pcbPointer=NULL;
enum Status{Free, Busy};
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

    //Intialization
    int rec_val;
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
    FILE * logFile;
    FILE * prefFile;
    
    //open the output files for write
    logFile = fopen("scheduler.log", "w");
    prefFile = fopen("scheduler.pref", "w");
    
    fprintf(logFile, "\n#At time x process y state arr w total z remain y wait k\n");
    

    //HPF
    PQueue HPF_readyQueue;
    PQueueInit(&HPF_readyQueue);
    //SRTN
    PQueue SRTN_Queue; 
    PQueueInit(&SRTN_Queue);
    //RR
    Queue RR_readyQueue;
    queueInit(&RR_readyQueue, sizeof(struct processData));

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
        rec_val= msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
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
            rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            while (rec_val != -1)
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
                rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
                
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
        rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        printf("\nProcess with Pid = %d recieved\n",message.mData.id);
        pData = message.mData;
        push(&SRTN_Queue, pData.runningtime, pData); //enqueue the data in the ready queue
        if(rec_val!=-1)
        {
 
            //Creating PCB
            pcbBlock.state = 0;
            
            pcbBlock.executionTime = 0;
            
            pcbBlock.remainingTime = pData.runningtime;
            
            pcbBlock.waitingTime = 0;
            
            insertFirst(pData.id, pcbBlock);
            
            printf("\nPCB created for process with Pid = %d\n",pData.id);
        }

        while(getlength(&SRTN_Queue)!=0)
        {
            rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            while(rec_val!=-1)
            {
                printf("\n Current time is: %d\n", getClk());
                
                printf("\nProcess with Pid = %d recieved\n",message.mData.id);
                
                pData = message.mData;
                
                push(&SRTN_Queue, pData.runningtime, pData);
            
     
                 //Creating PCB
                pcbBlock.state = 0;
                
                pcbBlock.executionTime = 0;
                
                pcbBlock.remainingTime = pData.runningtime;
                
                pcbBlock.waitingTime = 0;
                
                insertFirst(pData.id, pcbBlock);
                
                printf("\nPCB created for process with Pid = %d\n",pData.id);
                rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            }

            pData = pop (&SRTN_Queue);
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
                    push(&SRTN_Queue,pcbPointer->remainingTime , pData);
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

                    push (&SRTN_Queue, pcbPointer->remainingTime, pData);

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

        //RR
        case 3:
        //RR
             rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, !IPC_NOWAIT);
        printf("\nRecieved rec val is: %d \n",rec_val);
        pData = message.mData;
        enqueue(&RR_readyQueue, &pData); //enqueue the data in the ready queue
        printf("\n%d %d %d %d\n",pData.id,pData.arrivaltime,pData.runningtime,pData.priority);
        if(rec_val!=-1)
            {

                //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
                
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created\n");
            }
        
        while(getQueueSize(&RR_readyQueue)!=0 || pcount!=0)
        {   
            rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
            printf("from the rec_val at the end of the while loop %d\n",rec_val);
            while(rec_val!=-1)
            {
                printf("\n Current time is: %d\n", getClk());
                printf("\nRecieved rec val is: %d \n",rec_val);
                pData = message.mData;
                enqueue(&RR_readyQueue, &pData);
                printf("\n%d %d %d %d\n",pData.id,pData.arrivaltime,pData.runningtime,pData.priority);
                
                
                 //Creating PCB
                pcbBlock.state = 0;
                pcbBlock.executionTime = 0;
                pcbBlock.remainingTime = pData.runningtime;
                pcbBlock.waitingTime = 0;
                
                insertFirst(pData.id, pcbBlock);
                printf("\nPCB created\n");
                rec_val = msgrcv(msgqid, &message, sizeof(message.mData), 0, IPC_NOWAIT);
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
                sleep(50);
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

                }
            }
           
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
