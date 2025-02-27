# Makefile
# Author: aqrabwi, 13/02/2025 (modified)
# Description: Compiles two executables (oss and worker) from their respective source files.
#
# This Makefile uses gcc as the compiler with debugging (-g) and warning (-Wall) options.
# It defines rules for compiling source files into object files and then linking those object files
# to create the executables. A clean target is also provided to remove generated files.

# Set the C compiler to gcc.
CC = gcc

# Compiler flags:
#   -Wall: Enable all warning messages.
#   -g: Include debugging information.
CFLAGS = -Wall -g

# List of target executables to be built.
TARGETS = oss worker

# The default target "all" builds both executables.
all: $(TARGETS)
	@echo "Build complete: Executables $(TARGETS) have been created."

# Rule to build the "oss" executable from its object file oss.o.
oss: oss.o
	# Link oss.o using gcc and produce the executable 'oss'
	$(CC) $(CFLAGS) -o oss oss.o

# Rule to build the "worker" executable from its object file worker.o.
worker: worker.o
	# Link worker.o using gcc and produce the executable 'worker'
	$(CC) $(CFLAGS) -o worker worker.o

# Rule to compile oss.c into the object file oss.o.
oss.o: oss.c
	# Compile oss.c into an object file (oss.o) using the -c flag.
	$(CC) $(CFLAGS) -c oss.c

# Rule to compile worker.c into the object file worker.o.
worker.o: worker.c
	$(CC) $(CFLAGS) -c worker.c

# "clean" target to remove all generated object files and executables.
clean:
	# Remove all .o (object) files and the executables
	rm -f *.o $(TARGETS)
