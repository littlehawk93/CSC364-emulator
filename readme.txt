Written by: John Hawkins
Date:       4/4/13

To compile the source code, simply run the makefile using the command:

	make

If you want to keep the object files for faster compilation later, use:

	make keep

To compile the zip folder of all the source code and libraries, use:

	make zip

To delete all compiled files and zip file, use:

	make clean

To run the assembler, pipe the input file of the assembly code to the program
and then pipe its output to the desired file.

	./asm16 < input.file > output.file

To run the emulator, use the -f flag followed by a space and then the name of the file.
The -d flag will set the clock speed in milliseconds, but is optional (1000ms is default).

	./emu16 -f rom.file -d 2000

Any comments, questions, bugs, or suggestions, let me know at jch101@latech.edu.


Some Sample Code Snippets

# this is a comment
# To invert the sign of a register (make positive -> negative or negative -> postive)

not r0, r0
addi r0, r0, 1

# To halt execution of the emulator

set r14, 255
seth r14, 255
move r15, r14

# Handy Dandy Macros Included in the Assmebler:
# PC - stands for program counter, which is register 15 or r15 (rF)
# OUT0, OUTPUT0 - stands for the OUTPUT0 register, which is register 13 or r13 (rD)
# OUT1, OUTPUT1 - stands for the OUTPUT1 register, which is register 14 or r14 (rE)
# IN, INPUT - stands for the INPUT register, which is register 6 or r6

# EXAMPLES

addi PC, PC, 4

# Is the same as 

addi r15, r15, 4

# Which is the same as

addi rF, rF, 4

# The lower 8 bits of the input register come from either the screen or the RAM
# OUTPUT1 is the address for RAM or the Screen to read / write to
# OUTPUT0 has the lower 8 bits reserved for writing to the RAM or Screen
# OUTPUT0 highest bit is to toggle between read / write mode (1 is write, 0 is read)
# OUTPUT0 second highest bit is to toggle between the screen and RAM (1 is screen, 0 is RAM)

# New feature Added!
# To include code or raw machine code assembled from other files, try the following commands:
#	include
#	includebin
# These commands will make the assembler stop parsing the current file, open the new file and add its
# instructions to your file. Once all of the code from the external file has been parsed and copied 
# into the rom, the assembler picks up where it left off on the remainder of your code.
# These commands work recursively as well. Meaning you can include files with include statements in them
# DO NOT try to infintely include files! In other words, don't make fileA include fileB, if fileB already
# includes fileA. The assembler will infintely recurse through the files until something breaks.

# Included in the zip folder is a lib folder, which contains premade code snippets to include in
# order to save some time in programming. lib/bin is a pre-compiled library so you can use
# includebin if you want to save a little time. the binaries also take up significantly less space.
# Feel free to add / remove / change any files in the lib folder

include lib/NEG0
includebin lib/bin/HALT
