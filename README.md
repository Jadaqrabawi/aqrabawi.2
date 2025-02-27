
# Simulated Clock Multi-Process Project (aqrabawi.1)

## Quick Start

1. **Download or Clone the Project**  
   Clone the repository using:
   ```bash
   git clone https://github.com/Jadaqrabawi/aqrabawi.1.git
   ```
   Or download and extract the project archive.

2. **Navigate to the Project Directory**
   ```bash
   cd aqrabawi.1
   ```

3. **Clean the Project (Optional)**
   ```bash
   make clean
   ```

4. **Compile the Project**
   ```bash
   make all
   ```

5. **Run the Project**
   ```bash
   ./oss -n 10 -s 3 -t 7 -i 100
   ```


## Overview

This project simulates a simple multi-process system that uses a shared, simulated clock to coordinate process lifetimes. The project includes two executables:

- **oss**:  
  The parent process that:
  - Creates a shared system clock in shared memory (two integers representing seconds and nanoseconds).
  - Maintains a process table (an array of Process Control Blocks) that tracks each worker process.
  - Launches worker processes based on specified command-line parameters.
  - Monitors active worker processes using nonblocking waits.
  - Launches new workers at fixed intervals (based on the simulated clock) while enforcing a simultaneous process limit.
  - Automatically terminates after 60 real-life seconds, cleaning up shared memory and terminating any remaining workers.

- **worker**:  
  The child process that:
  - Attaches to the shared simulated clock created by **oss**.
  - Computes its target termination time by adding a specified duration (in simulated seconds and nanoseconds) to the current clock.
  - Busy-loops (without using any sleep functions) until the simulated clock exceeds its target time.
  - Outputs periodic status updates (each time the simulated seconds change) and a final termination message when its time has elapsed.

> **Note:** The **worker** executable is intended to be launched by **oss**. Running **worker** directly is only recommended for testing purposes.



## Detailed Instructions

### Compilation

1. **Open a Terminal**  
   Navigate to your project directory:
   ```bash
   cd aqrabawi.1
   ```

2. **Compile the Project**  
   Use the provided Makefile to compile both executables:
   ```bash
   make all
   ```
   This command compiles the source files for both **oss** and **worker** and creates their respective executables.

### Running the Project

#### Running the `oss` Executable

The **oss** process launches **worker** processes based on command-line options. Its usage is as follows:
```bash
./oss [-h] [-n totalProcs] [-s simulLimit] [-t childTimeLimit] [-i launchIntervalMs]
```
- **-h**: Displays help and usage information.
- **-n totalProcs**: Total number of worker processes to launch (default: 20).
- **-s simulLimit**: Maximum number of workers running concurrently (default: 5).
- **-t childTimeLimit**: Upper bound (in simulated seconds) for how long each worker runs (default: 5).
- **-i launchIntervalMs**: Interval (in simulated milliseconds) between launching new workers (default: 100).

**Example:**  
To launch 10 worker processes with a simultaneous limit of 3, each set to run for up to 7 simulated seconds, with a launch interval of 100 simulated milliseconds:
```bash
./oss -n 10 -s 3 -t 7 -i 100
```

#### Testing the `worker` Executable Directly (Optional)

For testing and debugging, you can run the **worker** executable directly:
```bash
./worker <secondsToStay> <nanoToStay>
```
**Example:**
```bash
./worker 5 500000
```
This command will run a worker process for 5 simulated seconds and 500,000 simulated nanoseconds.

### Cleaning Up

To remove all compiled object files and executables, run:
```bash
make clean
```



## Additional Information

- **Shared Memory and Simulated Clock**  
  The **oss** process creates a shared memory segment that holds the simulated clock (with seconds and nanoseconds). Worker processes attach to this shared memory to read the clock and determine their termination time.

- **Process Table**  
  **oss** maintains a process table that tracks each worker's PID and the simulated time at which it was launched. This table is used to monitor active processes and to free slots when workers terminate.

- **Version Control**  
  This project is managed using Git. To access the repository:
  ```bash
  git clone https://github.com/Jadaqrabawi/aqrabawi.1.git
  ```

- **Project Purpose**  
  This assignment is designed to enhance your skills in process management, shared memory usage, and simulated time control in a Linux environment. It demonstrates the use of system calls such as `fork()`, `exec()`, `waitpid()`, and shared memory functions to create a simple operating system simulation.

- **Usage Note**  
  The **worker** executable is typically invoked by **oss**. Running **worker** directly is recommended only for testing and debugging purposes.
