# Makefile for CSC 364 Emulator & Assembler
# Compiles into the following executables:
#	emu16 : The emulator
#	asm16 : The assembler
# Will also compile all source code and libraries into zip folder
# All commands are executed silently
# Written by: John Hawkins
#	Date: 3/28/13
#
# Any comments, questions, concerns, suggestions or
# bugs found, please contact me at jch101@latech.edu

# Compile object files into executables and delete object files
all: assembler.o emu16.o
	@gcc assembler.o -o asm16
	@g++ emu16.o -o emu16
	@rm -f *.o

# Compile object files into executables and keep them after compiling
keep: assembler.o emu16.o
	@gcc assembler.o -o asm16
	@g++ emu16.o -o emu16

# zip components into single zip package for sharing
zip: assembler.c emu16.cpp makefile readme.txt lib/
	@zip -r csc364_emulator.zip lib/ assembler.c emu16.cpp makefile readme.txt 1 > /dev/null

# Compile assembler into object file
assembler.o: assembler.c
	@gcc -c assembler.c

# Compile emulator into object file
emu16.o: emu16.cpp
	@g++ -c emu16.cpp

# Clean up everything
clean:
	@rm -f *.o
	@rm -f emu16
	@rm -f asm16
	@rm -f csc364_emulator.zip

# Delete any ROM files 
cleanrom:
	@rm -f *.rom

