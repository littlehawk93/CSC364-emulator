/*
 * CSC 364 Emulator
 * Written by: John Hawkins 
 * 		 Date: 5/7/13
 * Emulates a 16 bit microprocessor from CSC 364
 * Takes a file for initializing the ROM
 * Any comments, questions, concerns, suggestions or
 * bugs found, please contact me at jch101@latech.edu
 */
 
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// Register 15 (F) is the program counter
// Registers 13 (D) and 14 (E) are output registers
// Register 6 is the input register 
// Total ROM size is 2^16 - 2 because registers are 16 bit 
// and we need to be able to go over the max value (2^16 - 1)
// Total RAM size is all 16 bit addresses (2^16)
#define ROM_SIZE 65534 
#define RAM_SIZE 65536
#define SCREEN_WIDTH 16
#define PROG_COUNTER 15
#define INPUT 6
#define OUTPUT1 13
#define OUTPUT2 14

using namespace std;

void loadIn(unsigned short&,unsigned short&,char*);
void printAll(unsigned short*,int);
int processIn(unsigned short*,unsigned short);
void printReg(unsigned short);
void printScreen(unsigned char*,int);

/*
 * Main function. Handles all of the Microprocessor emulation.
 * Initializes the ROM from a provided file pointer, sets the 
 * registers, and counter and then begins reading the ROM and
 * executing the instructions written to it.
 */
int main(int argc,char** argv)
{
	int showScreen = 1;
	// Sleep time (ms) for each clock cycle
	int sleepTime = 1000;
	// Each instruction is 2 bytes, so total ROM is ROM_SIZE times 2
	// Initialize each ROM address to 0 before reading ROM file
	char rom[ROM_SIZE*2];
	for(int i=0;i<ROM_SIZE;i++) {
		rom[i] = 0;
	}
	// Initialize Registers & counter
	unsigned short in;
	int cycle;
	unsigned short reg[16];
	for(int i=0;i<16;i++) {
		reg[i] = 0;
	}
	// Initialize all RAM addresses to 0
	char ram[RAM_SIZE];
	for(int i=0;i<RAM_SIZE;i++) {
		ram[i] = 0;
	}
	
	unsigned char screen[SCREEN_WIDTH];
	for(int i=0;i<SCREEN_WIDTH;i++) {
		screen[i] = 0;
	}
	bool hasFile = false;
	for(int i=1;i<argc;i++) {
		// Read data from file provided into the ROM memory
		if(!strcmp(argv[i],"-f")) {
			if(i+1<argc) {
				i++;
				ifstream file;
				file.open(argv[i]);
				file.read(rom,ROM_SIZE);
				file.close();
				hasFile = true;
			}
		// Optional argument to change the delay between clock cycles
		}else if(!strcmp(argv[i],"-d")) {
			if(i+1<argc) {
				i++;
				sleepTime = atoi(argv[i]);
			}
		// Optional argument to hide the emulation screen
		}else if(!strcmp(argv[i],"-s")) {
			showScreen = 0;
		}
	}
	if(!hasFile) {
		cout << "No ROM File supplied" << endl;
		cout << "Usage:" << endl;
		cout << "\temu16 -f <file-path> -d <delay> -s" << endl << endl;
		cout << "\t -f : Input ROM file path" << endl;
		cout << "\t -d : Optional Delay between emulator clock cycles" << endl;
		cout << "\t -s : Optional turn off emulator display" << endl;
		return -1;
	}
	cycle = 1;
	// Continue executing until the counter exceeds total ROM size.
	while(reg[PROG_COUNTER] < ROM_SIZE) {
		loadIn(reg[PROG_COUNTER],in,rom);
		// If we're reading from RAM or Screen, we load the value into the input register
		if(!(reg[OUTPUT1] & 0x8000)) {
			reg[INPUT] &= 0xFF00;
			if(reg[OUTPUT1] & 0x4000) {
				reg[INPUT] |= (0xFF & screen[0xF & reg[OUTPUT2]]);
			}else {
				reg[INPUT] |= (0xFF & ram[reg[OUTPUT2]]);
			}
		}
		if(processIn(reg,in)) { // If processIn returns a non-zero number, something went wrong >.<
			cout << "FATAL ERROR - ";
			printReg(in);
			cout << endl;
			return 1;
		}
		// Write value to RAM or Screen if flag is on
		if(reg[OUTPUT1] & 0x8000) {
			if(reg[OUTPUT1] & 0x4000) {
				screen[0xF & reg[OUTPUT2]] = (0xFF & reg[OUTPUT1]);
			}else {
				ram[reg[OUTPUT2]] = (0xFF & reg[OUTPUT1]);
			}
		}
		// Print emulator information
		cout << "CLOCK CYCLE: " << cycle << endl;
		cout << "    COUNTER: ";
		printReg(reg[PROG_COUNTER]);
		cout << endl;
		cout << "INSTRUCTION: ";
		printReg(in);
		cout << endl << endl;
		// Print all of the emulator registers
		cout << "--------------- REGISTERS ---------------" << endl << endl;
		printAll(reg,16);
		// Print the emulated display if option is allowed
		if(showScreen) {
			cout << endl << "---------------- SCREEN -----------------" << endl << endl;
			printScreen(screen,SCREEN_WIDTH);
		}
		// Sleep, clear the console, and increment the clock cycle
		// if there will be another instruction to process
		if(reg[PROG_COUNTER] < ROM_SIZE) {
			usleep(sleepTime * 1000);
			system("clear");
			cycle++;
		}
	}
	return 0;
}

/*
 * Loads the next instruction into the variable address &in.
 * Increments &counter by the number of bytes read from rom into &in.
 * (Increments by 2)
 */
void loadIn(unsigned short &counter,unsigned short &in, char* rom)
{
	in = rom[counter * 2] & 0xFF;
	in <<= 8;
	in |= (rom[counter * 2 + 1] & 0xFF);
}

/*
 * Process the command stored in variable 'in'.
 * The command is executed on the provided array of regsiters.
 * If any operation is done on the program counter, the program counter
 * is not incremented, otherwise, it is.
 * 
 * Returns 0 if execution was correct, otherwise returns a non-zero number.
 */
int processIn(unsigned short* reg,unsigned short in)
{
	int opcd = ((in & 0xF000) >> 12) & 0xFF;
	int regD = ((in & 0x0F00) >> 8) & 0xFF;
	int regA = ((in & 0x00F0) >> 4) & 0xFF;
	int regB = (in & 0x000F) & 0xFF;
	// Flag variable to determine whether or not to increment the PC
	int flag = (regD == PROG_COUNTER) ? 0 : 1;
	
	if(regD != INPUT) {
		switch(opcd) {
			
			case 0: // MOVE
				reg[regD] = reg[regA];
				break;
				
			case 1: // NOT
				reg[regD] = ~reg[regA];
				break;
				
			case 2: // AND
				reg[regD] = reg[regA] & reg[regB];
				break;
				
			case 3: // OR
				reg[regD] = reg[regA] | reg[regB];
				break;
				
			case 4: // ADD
				reg[regD] = reg[regA] + reg[regB];
				break;
				
			case 5: // SUB
				reg[regD] = reg[regA] - reg[regB];
				break;
				
			case 6: // ADDI
				reg[regD] = reg[regA] + regB;
				break;
				
			case 7: // SUBI
				reg[regD] = reg[regA] - regB;
				break;
				
			case 8: // SET
				reg[regD] = (0xF0 & (regA << 4)) | (0xF & regB);
				break;
				
			case 9: // SETH
				reg[regD] &= 0x00FF;
				reg[regD] |= (((0xF0 & (regA << 4)) | (0xF & regB)) << 8);
				break;
				
			case 10: // INCIZ
				if(!reg[regB]) {
					reg[regD] += regA;
				}else {
					flag = 1;
				}
				break;
				
			case 11: // DECIN
				if((reg[regB] & 0x8000)) {
					reg[regD] -= regA;
				}else {
					flag = 1;
				}
				break;
				
			case 12: // MOVEZ
				if(!reg[regB]) {
					reg[regD] = reg[regA];
				}else {
					flag = 1;
				}
				break;
				
			case 13: // MOVEX
				if(reg[regB]) {
					reg[regD] = reg[regA];
				}else {
					flag = 1;
				}
				break;
				
			case 14: // MOVEP
				if(!(reg[regB] & 0x8000)) {
					reg[regD] = reg[regA];
				}else {
					flag = 1;
				}
				break;
				
			case 15: // MOVEN
				if((reg[regB] & 0x8000)) {
					reg[regD] = reg[regA];
				}else {
					flag = 1;
				}
				break;
			
			default: // CATCH-ALL
				return opcd;
		}
	}
	// Check flag if any operations were done on the PC
	if(flag) {
		reg[PROG_COUNTER]++;
	}
	return 0;
}

/*
 * Prints all of the binary register values to cout.
 * Also provides hexadecimal labels for each register.
 */
void printAll(unsigned short* reg,int len)
{
	char letters[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for(int i=0;i<len;i+=2) {
		cout << letters[i] << " ";
		printReg(reg[i]);
		cout << " - ";
		printReg(reg[i+1]);
		cout << " " << letters[i+1] << endl;
	}
}

/*
 * Prints the binary value of the provided register.
 * Puts a space (gap) between each byte of the register.
 */
void printReg(unsigned short num) 
{
	const int len = sizeof(num) * 8;
	unsigned int mask = (1 << (len - 1));
	for(int i=0;i<len;i++) {
		if(!(i % 8) && i) {
			cout << " ";
		}
		cout << ((num & mask) ? '1' : '0');
		mask >>= 1;
	}
}

/*
 * Prints the 16 x 8 pixel display used by the CPU
 */
void printScreen(unsigned char* screen,int len)
{
	// Top screen border
	for(int i=0;i<len+1;i++) {
		if(len - 1 - i) {
			cout << "-";
		}
		cout << "-";
	}
	cout << endl;
	// Scan each of the 8 bits of each memory address
	for(int i=0;i<8;i++) {
		// Mask deterimes which bit of the memory address we are reading
		int mask = 1 << (7 - i);
		// Left side border
		cout << "|";
		// Loop through all the memory addresses
		for(int j=len-1;j>=0;j--) {
			cout << ((screen[j] & mask) ? "*" : " ");
			if(j) {
				cout << " ";
			}
		}
		// Right side border
		cout << "|" << endl;
	}
	// Bottom border
	for(int i=0;i<len+1;i++) {
		if(len - 1 - i) {
			cout << "-";
		}
		cout << "-";
	}
	cout << endl;
}
