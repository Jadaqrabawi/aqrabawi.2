/*
 * oss.c
 * Author: aqrabwi, 13/02/2025 (modified)
 * Description: Launches worker processes using a simulated system clock in shared memory.
 *              Maintains a process table and launches workers based on command-line parameters.
 *
 * Usage: oss [-h] [-n totalProcs] [-s simulLimit] [-t childTimeLimit] [-i launchIntervalMs]
 *   -n totalProcs        Total number of worker processes to launch (default: 20)
 *   -s simulLimit        Maximum number of workers running concurrently (default: 5)
 *   -t childTimeLimit    Upper bound (in seconds) for a worker's run time (default: 5)
 *   -i launchIntervalMs  Interval (in simulated milliseconds) between launches (default: 100)
 */

 #include <stdio.h>      
 #include <stdlib.h>     
 #include <unistd.h>     
 #include <sys/shm.h>    
 #include <sys/ipc.h>    
 #include <sys/types.h>  
 #include <sys/wait.h>   
 #include <signal.h>     
 #include <time.h>       
 #include <string.h>     
 #include <errno.h>      
 #include <stdbool.h>    
 #include <getopt.h>     
 
 // Defining the key for shared memory segment.
 #define SHMKEY 9876
 
 // Maximum number of child processes to track in the process table.
 #define MAX_CHILDREN 20
 
 // Nanosecond conversion.
 #define ONE_BILLION 1000000000ULL
 
 // Default command-line parameters if not provided by the user.
 #define DEFAULT_TOTAL_PROCS 20
 #define DEFAULT_SIMUL_LIMIT 5
 #define DEFAULT_CHILD_TIME_LIMIT 5      // seconds each worker runs, upper bound
 #define DEFAULT_LAUNCH_INTERVAL_MS 100    // simulated milliseconds between launches
 
 // Structure representing a Process Control Block (PCB) for each worker.
 typedef struct {
     int occupied;        // Flag: 0 if free, 1 if this entry is occupied
     pid_t pid;           // Process ID of the worker process
     int startSeconds;    // Simulated clock seconds at which the worker was launched
     int startNano;       // Simulated clock nanoseconds at which the worker was launched
 } PCB;
 
 PCB processTable[MAX_CHILDREN];
 
 // Global variables for shared memory management.
 int shmid;       // Shared memory identifier.
 int *shmClock;   // Pointer to the shared memory segment storing the simulated clock:
 // shmClock[0] holds seconds, shmClock[1] holds nanoseconds.
 
 // Global parameters, which may be overridden by command-line options.
 int totalProcs = DEFAULT_TOTAL_PROCS;        // Total number of workers to launch.
 int simulLimit = DEFAULT_SIMUL_LIMIT;          // Maximum workers running concurrently.
 int childTimeLimit = DEFAULT_CHILD_TIME_LIMIT; // Upper bound for worker run time (in seconds).
 int launchIntervalMs = DEFAULT_LAUNCH_INTERVAL_MS; // Interval (in simulated ms) between launching workers.
 
 // Volatile flag for safe termination in signal handlers.
 volatile sig_atomic_t terminateFlag = 0;
 
 // Cleanup function to detach and remove shared memory and terminate child processes.
 // This function is called when SIGINT (Ctrl-C) or SIGALRM (timeout) is received.
 void cleanup(int signum) {
     // If the shared memory is attached, detach it.
     if (shmClock != (void *) -1) {
         shmdt(shmClock);
     }
     // Remove the shared memory segment from the system.
     shmctl(shmid, IPC_RMID, NULL);
     // Send SIGTERM to all processes in the current process group (to kill all children).
     kill(0, SIGTERM);
     exit(1);
 }
 
 // Alarm handler for timeout after 60 real-life seconds.
 // When the alarm triggers, notify and perform cleanup.
 void alarmHandler(int signum) {
     printf("Real time limit reached. Terminating oss and all children.\n");
     cleanup(signum);
 }
 
 // Function to increment the simulated system clock.
 // It adds the given seconds and nanoseconds to the current clock stored in shared memory.
 void incrementClock(int secIncrement, int nanoIncrement) {
     // Add nanosecond increment.
     shmClock[1] += nanoIncrement;
     // Add second increment.
     shmClock[0] += secIncrement;
     // Normalize the clock: if nanoseconds exceed one billion, convert them to seconds.
     if (shmClock[1] >= ONE_BILLION) {
         shmClock[0] += shmClock[1] / ONE_BILLION;
         shmClock[1] %= ONE_BILLION;
     }
 }
 
 // Function to display the current simulated clock and the process table.
 // This is useful for debugging and tracking simulation progress.
 void displayTime() {
     // Print the OSS process ID and the current simulated clock time.
     printf("OSS PID: %d | SysClock: %d s, %d ns\n", getpid(), shmClock[0], shmClock[1]);
     printf("Process Table:\n");
     printf("Entry  Occupied  PID     StartSec  StartNano\n");
     // Loop over each entry in the process table and print its status.
     for (int i = 0; i < MAX_CHILDREN; i++) {
         printf("%-6d %-9d %-7d %-9d %-9d\n", i, processTable[i].occupied, processTable[i].pid,
                processTable[i].startSeconds, processTable[i].startNano);
     }
     printf("\n");
 }
 
 int main(int argc, char *argv[]) {
     int opt;
     // Parse command-line options using getopt.
     // Options:
     //  -h: help
     //  -n: total number of worker processes to launch
     //  -s: maximum number of simultaneous workers
     //  -t: upper bound for worker run time (in seconds)
     //  -i: simulated interval (ms) between launching workers
     while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
         switch (opt) {
             case 'h':
                 // Display help/usage information.
                 printf("Usage: %s [-n totalProcs] [-s simulLimit] [-t childTimeLimit] [-i launchIntervalMs]\n", argv[0]);
                 exit(0);
             case 'n':
                 // Set total number of worker processes.
                 totalProcs = atoi(optarg);
                 break;
             case 's':
                 // Set maximum number of simultaneous workers.
                 simulLimit = atoi(optarg);
                 break;
             case 't':
                 // Set the upper bound for each worker's runtime (in seconds).
                 childTimeLimit = atoi(optarg);
                 break;
             case 'i':
                 // Set the launch interval in simulated milliseconds.
                 launchIntervalMs = atoi(optarg);
                 break;
             default:
                 // Handle unknown options.
                 fprintf(stderr, "Unknown option: %c\n", opt);
                 exit(1);
         }
     }
  
     // Set up signal handlers for SIGINT (e.g., Ctrl-C) and SIGALRM (timeout).
     signal(SIGINT, cleanup);
     signal(SIGALRM, alarmHandler);
     alarm(60);  // Automatically terminate after 60 real-life seconds.
  
     // Create a shared memory segment for the simulated clock (2 integers: seconds and nanoseconds).
     shmid = shmget(SHMKEY, 2 * sizeof(int), IPC_CREAT | 0666);
     if (shmid == -1) {
         perror("oss: shmget");
         exit(1);
     }
     // Attach the shared memory segment to our address space.
     shmClock = (int *) shmat(shmid, NULL, 0);
     if (shmClock == (int *) -1) {
         perror("oss: shmat");
         exit(1);
     }
     // Initialize the simulated clock to 0 seconds and 0 nanoseconds.
     shmClock[0] = 0;  // seconds
     shmClock[1] = 0;  // nanoseconds
  
     // Initialize the process table by marking all entries as free.
     for (int i = 0; i < MAX_CHILDREN; i++) {
         processTable[i].occupied = 0;
     }
  
     int launchedCount = 0; // Number of worker processes launched so far.
     int runningCount = 0;  // Number of worker processes currently running.
     // Record the last launch time (in simulated nanoseconds) to enforce the launch interval.
     unsigned long long lastLaunchTime = 0;
  
     // Main loop: continue until all workers have been launched and all have terminated.
     while (launchedCount < totalProcs || runningCount > 0) {
         // Increment the simulated clock by 1 millisecond (1,000,000 ns).
         incrementClock(0, 1000000);
  
         // Display the process table periodically when the nanosecond counter resets (roughly every second).
         if (shmClock[1] < 1000000) {
             displayTime();
         }
  
         // Check for any terminated children using a nonblocking wait.
         int status;
         pid_t pidTerm = waitpid(-1, &status, WNOHANG);
         if (pidTerm > 0) {
             // Search for the terminated child's entry in the process table.
             for (int i = 0; i < MAX_CHILDREN; i++) {
                 if (processTable[i].occupied && processTable[i].pid == pidTerm) {
                     // Mark the entry as free and decrease the count of running workers.
                     processTable[i].occupied = 0;
                     runningCount--;
                     printf("Child PID %d terminated.\n", pidTerm);
                     break;
                 }
             }
         }
  
         // Compute the current simulated time in nanoseconds.
         unsigned long long currentSimTime = ((unsigned long long) shmClock[0]) * ONE_BILLION + shmClock[1];
  
         // Conditions to launch a new worker:
         // 1. Not all required workers have been launched.
         // 2. Running workers are below the simultaneous limit.
         // 3. Sufficient simulated time has passed since the last launch.
         if (launchedCount < totalProcs && runningCount < simulLimit &&
             (currentSimTime - lastLaunchTime) >= ((unsigned long long) launchIntervalMs) * 1000000) {
  
             // Find a free slot in the process table.
             int slot = -1;
             for (int i = 0; i < MAX_CHILDREN; i++) {
                 if (!processTable[i].occupied) {
                     slot = i;
                     break;
                 }
             }
             if (slot != -1) {
                 // Generate a random runtime for the worker:
                 // Random seconds between 1 and childTimeLimit, and random nanoseconds between 0 and 1e9-1.
                 int randSec = (rand() % childTimeLimit) + 1;
                 int randNano = rand() % ONE_BILLION;
  
                 // Fork a new worker process.
                 pid_t pid = fork();
                 if (pid < 0) {
                     perror("oss: fork");
                     cleanup(0);
                 } else if (pid == 0) {
                     // Child process: Prepare arguments and execute the worker.
                     char secArg[16], nanoArg[16];
                     sprintf(secArg, "%d", randSec);
                     sprintf(nanoArg, "%d", randNano);
                     execl("./worker", "worker", secArg, nanoArg, (char *)NULL);
                     // If execl returns, an error occurred.
                     perror("oss: execl");
                     exit(1);
                 } else {
                     // Parent process: Record the new worker in the process table.
                     processTable[slot].occupied = 1;
                     processTable[slot].pid = pid;
                     processTable[slot].startSeconds = shmClock[0];
                     processTable[slot].startNano = shmClock[1];
                     launchedCount++;   // Increment the count of launched workers.
                     runningCount++;    // Increment the count of currently running workers.
                     // Update the last launch time to the current simulated time.
                     lastLaunchTime = currentSimTime;
                     printf("Launched worker PID %d at simulated time %d s, %d ns. (Worker will run for %d s and %d ns)\n",
                            pid, shmClock[0], shmClock[1], randSec, randNano);
                 }
             }
         }
         // Busy-loop: In a production system, a short usleep() might yield CPU time.
         // However, we cannot sleep because we simulate time using our own clock.
     }
  
     // Cleanup: detach and remove shared memory before exiting.
     shmdt(shmClock);
     shmctl(shmid, IPC_RMID, NULL);
     return 0;
 }
 