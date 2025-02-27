/*
 * worker.c
 * Author: aqrabwi, 13/02/2025 (modified)
 * Description: Worker process that attaches to the shared simulated clock,
 *              computes a target termination time based on command-line arguments,
 *              and busy-loops (without sleep) until the simulated clock passes that target.
 *
 * Usage: worker <secondsToStay> <nanoToStay>
 */

 #include <stdio.h>      
 #include <stdlib.h>     
 #include <unistd.h>       
 #include <signal.h>     
 #include <stdbool.h>    
 
 // Define the shared memory key for the simulated clock.
 #define SHMKEY 9876
 // Define the value representing one billion for nanosecond calculations.
 #define ONE_BILLION 1000000000
 
 // Global variable to hold the shared memory ID.
 int shmid;
 // Pointer to the shared memory segment representing the simulated clock.
 // shmClock[0] holds seconds, and shmClock[1] holds nanoseconds.
 int *shmClock;
 
 /*
  * cleanupWorker - Signal handler for cleaning up shared memory and exiting.
  * @signum: The signal number that triggered this handler.
  *
  * This function detaches the shared memory segment if it is attached,
  * and then exits the process.
  */
 void cleanupWorker(int signum) {
     // Check if the shared memory pointer is valid.
     if (shmClock != (void *) -1) {
         // Detach the shared memory segment from this process's address space.
         shmdt(shmClock);
     }
     // Exit the process with a status of 1 (indicating abnormal termination).
     exit(1);
 }
 
 int main(int argc, char *argv[]) {
     // Verify that the required command-line arguments are provided.
     // The program expects two arguments: secondsToStay and nanoToStay.
     if (argc < 3) {
         fprintf(stderr, "Usage: %s <secondsToStay> <nanoToStay>\n", argv[0]);
         exit(1);
     }
 
     // Convert command-line arguments from strings to integers.
     int secondsToStay = atoi(argv[1]);
     int nanoToStay = atoi(argv[2]);
 
     // Set up a signal handler for SIGINT (e.g., when the user presses Ctrl-C)
     // to ensure proper cleanup of shared memory.
     signal(SIGINT, cleanupWorker);
 
     // Attach to the existing shared memory segment that holds the simulated clock.
     // The segment is expected to be created by the oss process.
     shmid = shmget(SHMKEY, 2 * sizeof(int), 0666);
     if (shmid == -1) {
         perror("worker: shmget");
         exit(1);
     }
 
     // Attach the shared memory segment to our process's address space.
     shmClock = (int *) shmat(shmid, NULL, 0);
     if (shmClock == (int *) -1) {
         perror("worker: shmat");
         exit(1);
     }
 
     // Capture the starting simulated time from the shared memory.
     int startSec = shmClock[0];
     int startNano = shmClock[1];
 
     // Calculate the target termination time by adding the desired duration

     int targetSec = startSec + secondsToStay;
     int targetNano = startNano + nanoToStay;
     // Normalize the target time if nanoseconds exceed one billion.
     if (targetNano >= ONE_BILLION) {
         targetSec += targetNano / ONE_BILLION;
         targetNano %= ONE_BILLION;
     }
 
     // Output initial status information including process IDs,
     // current simulated clock, and target termination time.
     printf("WORKER PID: %d PPID: %d | SysClock: %d s, %d ns | Target Termination: %d s, %d ns -- Just Starting\n",
            getpid(), getppid(), startSec, startNano, targetSec, targetNano);
 
     // Variable to track the last second printed for periodic updates.
     int lastPrintedSec = startSec;
 
     // Enter a busy-loop: the worker will continuously check the simulated clock
     // until the current time meets or exceeds the target termination time.
     while (true) {
         // Check if the simulated clock has reached or passed the target termination time.
         // The condition checks if the seconds part is greater than the target seconds,
         // or if equal, whether the nanoseconds part is greater than or equal to the target nanoseconds.
         if ((shmClock[0] > targetSec) ||
             (shmClock[0] == targetSec && shmClock[1] >= targetNano)) {
             // If the target is reached, output a termination message with current time.
             printf("WORKER PID: %d PPID: %d | SysClock: %d s, %d ns | Target Termination: %d s, %d ns -- Terminating\n",
                    getpid(), getppid(), shmClock[0], shmClock[1], targetSec, targetNano);
             break;
         }
         // Every time the simulated seconds change, print a status update.
         if (shmClock[0] != lastPrintedSec) {
             printf("WORKER PID: %d PPID: %d | SysClock: %d s, %d ns | Target Termination: %d s, %d ns -- %d seconds have passed since starting\n",
                    getpid(), getppid(), shmClock[0], shmClock[1], targetSec, targetNano, shmClock[0] - startSec);
             // Update the last printed second to avoid duplicate messages.
             lastPrintedSec = shmClock[0];
         }
         // The busy-loop does not call sleep() or usleep() because the simulation
     }
 
     // Once the loop exits (i.e., the worker's time has expired), detach the shared memory.
     shmdt(shmClock);
 
     // Return 0 to indicate normal termination.
     return 0;
 }
 