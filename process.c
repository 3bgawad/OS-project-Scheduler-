#include "headers.h"

int main(int agrc, char * argv[])
{
    initClk();
    int runningtime;
    printf("\n from process.c file a new process is created\n");

    //TODO it needs to get the remaining time from somewhere

    // Getting Runningtime and store it on its variable
    sscanf(argv[1],"%d",&runningtime);
    printf("Runtime of process is: %d and the clock is %d\n",runningtime,getClk());
    //returns the number of clock ticks elapsed since the program was launched. 
    //Number of Seconds used by CPU = clock() divided by CLOCKS_PER_SEC.
    // more info through: https://www.tutorialspoint.com/c_standard_library/c_function_clock.htm
    while (clock()/CLOCKS_PER_SEC <  runningtime);
    // If it ended its job 
    printf("\n Process is done. Current clock is: %d \n",getClk());
    destroyClk(false);
    kill(getppid(),SIGUSR1); //send signal to the scheduler when finished
    return 0;
}
