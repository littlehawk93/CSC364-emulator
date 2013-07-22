/*
 * CSC 364 Assembler
 * Written by: John Hawkins 
 * 		 Date: 5/2/13
 * Compiles Assembly language from CSC 364 into machine code for emulator
 * Any comments, questions, concerns, suggestions or
 * bugs found, please contact me at jch101@latech.edu
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

// Total number of bytes that can be written
#define ROM_SIZE 131068

// Function prototypes
int getComNum(char*);
int getRegNum(char*);
int getNormNum(char*);

/*
 * Main function. The total parsing of the assembly code is converted to machine code
 * for the emulator. Input is read from stdin and output is printed to stdout. Piping
 * is recommended. Any and all problems with the inputted code are printed at the end of the
 * process before printing the resulting code to stderr.
 */
int main()
{
	// Assign main variables rom is the total memory to be used by the emulator
	unsigned char rom[ROM_SIZE];
	int lineNum = 1, romIndex = 0, err = 0;
	char line[64];
	// Read input until no more is provided
	while(fgets(line,64,stdin) != NULL) {
		
		// First, we tokenize the line and count the total number of tokens provided (up to 5)
		int tokSize = 1;
		char *toks[6];
		toks[0] = strtok(line," ,\n\t\r");
		while((toks[tokSize] = strtok(NULL," ,\n\t\r")) != NULL || tokSize > 5) {
			// If we reach a comment we are finished loading tokens for the line
			if(toks[tokSize][0] == '#') {
				break;
			}
			tokSize++;
		}
		// Check that the first token is not null (empty) and that there are tokens to parse
		if(tokSize > 0 && toks[0] != NULL) {
			// Check if the line is a comment
			// If so, ignore line and continue
			if(toks[0][0] == '#') {
				continue;
			}
			
			int tI;
			// Convert all characters in first token to lower case
			for(tI=0;tI<strlen(toks[0]);tI++) {
				if(isupper(toks[0][tI])) {
					toks[0][tI] += 32;
				}
			}
			// Check for include statement to import existing code
			if(!strcmp(toks[0],"include")) {
				if(tokSize < 2) {
					fprintf(stderr,"line %d - Assembler Error: include statement requires file pointer\n",lineNum);
					err++;
				}else {
					// Create new files for input and output for child process
					FILE* file = fopen(toks[1],"r");
					char name[32];
					sprintf(name,"TMPBIN_%s.tmp~",toks[1]);
					int i;
					for(i=0;i<strlen(name);i++) {
						name[i] = (name[i] == '/' || isspace(name[i])) ? '_' : name[i];
					}
					FILE* out = fopen(name,"w");
					// Check to make sure input file exists
					if(file == NULL) {
						fprintf(stderr,"line %d - Assembler Error: File pointer \'%s\' not valid\n",lineNum,toks[1]);
						err++;
					}else {
						// fork the process to assemble the new code file 
						int child = fork();
						if(fork < 0) {
							fprintf(stderr,"line %d - Assembler Error: Unable to fork child process\n",lineNum);
							err++;
						}else {
							if(child) { // Parent process
								int status = 0;
								// Wait for Child process to finish
								while(waitpid(child,&status,WUNTRACED) > 0);
								FILE* temp = fopen(name,"r");
								if(temp == NULL) {
									fprintf(stderr,"line %d - Assembler Error: Failed to read assembled code from \'%s\'\n",lineNum,toks[1]);
									err++;
								}else {
									// Read in the entire machine code from the temporary file and save into the ROM
									int c;
									while((c = fgetc(temp)) != EOF) {
										if(romIndex + 1 < ROM_SIZE) {
											rom[romIndex++] = (unsigned char) c;
										}else {
											fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
											err++;
										}
									}
									// Close the temporary file and delete it
									fclose(temp);
									if(remove(name)) {
										fprintf(stderr,"line %d - Warning: Unable to delete temporary file\n",lineNum);
									}
								}
							}else { // Child process
								// Redirect stdin and stdout
								dup2(fileno(file),STDIN_FILENO);
								dup2(fileno(out),STDOUT_FILENO);
								// Call a fresh copy of the assembler process with redirected I/O
								execlp("./asm16","asm16",NULL);
								fprintf(stderr,"line %d - Assembler Error: Unable to create new process\n",lineNum);
								err++;
							}
						}
					}
				}
				continue;
			// Check for includebin statement to import existing binary machine code
			}else if(!strcmp(toks[0],"includebin")) {
				if(tokSize < 2) {
					fprintf(stderr,"line %d - Assembler Error: includebin statement requires file pointer\n",lineNum);
					err++;
				}else {
					FILE* file = fopen(toks[1],"r");
					if(file == NULL) {
						fprintf(stderr,"line %d - Assembler Error: File pointer \'%s\' not valid\n",lineNum,toks[1]);
						err++;
					}else {
						// Read in the entire machine code file and save into the ROM
						int c;
						while((c = fgetc(file)) != EOF) {
							if(romIndex + 1 < ROM_SIZE) {
								rom[romIndex++] = (unsigned char) c;
							}else {
								fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
								err++;
							}
						}
						fclose(file);
					}
				}
				continue;
			}
			
			// Get command number of the first token 
			switch(getComNum(toks[0])) {
				
			case 0: // MOV
				if(tokSize != 3) {
					fprintf(stderr,"line %d - Syntax Error: MOVE (MOV) command takes 2 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						if(r1 < 0 || r2 < 0 || r1 > 15 || r2 > 15) {
							fprintf(stderr,"line %d - Syntax Error: MOVE (MOV) command takes 2 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xF & r1);
							rom[romIndex++] = (unsigned char) (0xF0 & (r2 <<= 4));
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 1: // NOT
				if(tokSize != 3) {
					fprintf(stderr,"line %d - Syntax Error: NOT command takes 2 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						if(r1 < 0 || r2 < 0 || r1 > 15 || r2 > 15) {
							fprintf(stderr,"line %d - Syntax Error: NOT command takes 2 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x10 | (0xF & r1));
							rom[romIndex++] = (unsigned char) (0xF0 & (r2 <<= 4));
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 2: // AND
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: AND command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: AND command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x20 | (0xF & r1));
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3));
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 3: // ORR
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: OR (ORR) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: OR (ORR) command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x30 | (0xF & r1));
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3));
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 4: // ADD
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: ADD command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: ADD command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x40 | (0xF & r1));
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3));
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 5: // SUB
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: SUB command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: SUB command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) 0x50 | (0xF & r1);
							rom[romIndex++] = (unsigned char) (0xF0 & (r2 <<= 4)) | (0xF & r3);
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 6: // ADI
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: ADDI (ADI) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getNormNum(toks[3]);
						if(r1 == -1 || r2 == -1 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: ADDI (ADI) command takes 2 Registers and a Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x60 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 7: // SBI
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: SUBI (SBI) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getNormNum(toks[3]);
						if(r1 == -1 || r2 == -1 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15 || r3 < 0) {
							fprintf(stderr,"line %d - Syntax Error: SUBI (SBI) command takes 2 Registers and a Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x70 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 8: // SET
				if(tokSize != 3) {
					fprintf(stderr,"line %d - Syntax Error: SET command takes 2 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getNormNum(toks[2]);
						if(r1 < 0 || r2 < 0 || r1 > 15 || r2 > 255) {
							fprintf(stderr,"line %d - Syntax Error: SET command takes 1 Register and 1 Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x80 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) (0xFF & r2) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 9: // STH
				if(tokSize != 3) {
					fprintf(stderr,"line %d - Syntax Error: SETH (STH) command takes 2 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getNormNum(toks[2]);
						if(r1 < 0 || r2 < 0 || r1 > 15 || r2 > 255) {
							fprintf(stderr,"line %d - Syntax Error: SETH (STH) command takes 1 Register and 1 Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0x90 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) (0xFF & r2) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 10: // INC
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: INCIZ (INC) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getNormNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 == -1 || r2 == -1 || r3 == -1 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: INCIZ (INC) command takes 2 Registers and a Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xA0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 11: // DEC
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: DECIN (DEC) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getNormNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 == -1 || r3 == -1 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: DECIN (DEC) command takes 2 Registers and a Constant\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xB0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 12: // MVZ
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: MOVEZ (MVZ) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: MOVEZ (MVZ) command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xC0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 13: // MVX
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: MOVEX (MVX) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: MOVEX (MVX) command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xD0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 14: // MVP
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: MOVEP (MVP) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: MOVEP (MVP) command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xE0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			case 15: // MVN
				if(tokSize != 4) {
					fprintf(stderr,"line %d - Syntax Error: MOVEN (MVN) command takes 3 arguments\n",lineNum);
					err++;
				}else {
					if(romIndex + 1 < ROM_SIZE) {
						int r1 = getRegNum(toks[1]);
						int r2 = getRegNum(toks[2]);
						int r3 = getRegNum(toks[3]);
						if(r1 < 0 || r2 < 0 || r3 < 0 || r1 > 15 || r2 > 15 || r3 > 15) {
							fprintf(stderr,"line %d - Syntax Error: MOVEN (MVN) command takes 3 Registers\n",lineNum);
							err++;
						}else {
							rom[romIndex++] = (unsigned char) (0xF0 | (0xF & r1)) & 0xFF;
							rom[romIndex++] = (unsigned char) ((0xF0 & (r2 <<= 4)) | (0xF & r3)) & 0xFF;
						}
					}else {
						fprintf(stderr,"line %d - Out of Memory Error\n",lineNum);
						err++;
					}
				}
				break;
				
			default: // ERROR
				fprintf(stderr,"line %d - Unrecognized Command: \'%s\'\n",lineNum,toks[0]);
				err++;
				break;
			}
		}
		lineNum++;
	}
	// If no errors were found, the rom data can be printed to stdout.
	if(!err) {
		fprintf(stderr,"Total Bytes Written: %d\n",romIndex);
		fwrite(rom,sizeof(char),romIndex,stdout);
	}
	return 0;
}

/*
 * Converts the provided string into the command number.
 * 
 * Returns: The decimal equivalent of the machine code number for the
 * 			provided commmand, or -1 if an invalid string is provided.
 */
int getComNum(char *str)
{
	// Short 3 character commands
	const char *coms[] = {
		"mov", "not", "and", "orr", 
		"add", "sub", "adi", "sbi", 
		"set", "sth", "inc", "dec", 
		"mvz", "mvx", "mvp", "mvn"
	};
	
	// Long commands (specified by professor in class)
	const char *lComs[] = {
		"move", "not", "and", "or", 
		"add", "sub", "addi", "subi", 
		"set", "seth", "inciz", "decin", 
		"movez", "movex", "movep", "moven"
	};
	
	int i;
	// Check short command names first
	for(i=0;i<16;i++) {
		if(!strcmp(str,coms[i])) {
			return i;
		}
	}
	
	// Check long command names
	for(i=0;i<16;i++) {
		if(!strcmp(str,lComs[i])) {
			return i;
		}
	}
	return -1;
}

/*
 * Parses a string to find its corresponding register number.
 * String must be 2 characters long. The first character being the letter r (or R).
 * The second character must be a hexadecimal number (0 - F) or a decimal number (0 - 15)
 * Also will parse PC (Program Counter) to the decimal value 15 (hex F)
 * Parses the macros out0 or output0, out1 or output1, in or input to their respective registers
 * 
 * Returns: The decimal equivalent of the hexadecimal register number provided
 * 			or -1 if non-valid string provided
 */
int getRegNum(char *str)
{
	// Convert letters to lowercase
	int i;
	for(i=0;i<strlen(str);i++) {
		if(isupper(str[i])){
			str[i] += 32; 
		}
	}
	// Macro for program counter register
	if(!strcmp(str,"pc")) {
		return 15;
	// Macro for output 0 register
	}else if(!strcmp(str,"out0") || !strcmp(str,"output0")) {
		return 13;
	// Macro for output 1 register
	}else if(!strcmp(str,"out1") || !strcmp(str,"output1")) {
		return 14;
	// Macro for input register
	}else if(!strcmp(str,"in") || !strcmp(str,"input")) {
		return 6;
	}
	
	if(strlen(str) >= 2 && (str[0] == 'r')) {
		// If it's not a digit, check hexadecimal value
		if(isdigit(str[1])) {
			return atoi(&str[1]);
		}else {
			const char l[] = {'a','b','c','d','e','f'};
			for(i=0;i<6;i++) {
				if(str[1] == l[i]) {
					return i+10;
				}
			}
		}
	}
	return -1;
}

/*
 *  Parses a number constant and returns its decimal value
 *  Value can be hexadecimal (starts with x), binary (starts with b)
 *  or just regular decimal.
 */
int getNormNum(char* str)
{
	int num, i, fac;
	switch(str[0]) {
		
	case 'x':
	case 'X':
		num = 0;
		fac = 1;
		for(i=strlen(str)-1;i>0;i--) {
			if(isdigit(str[i])) {
				num += fac * (str[i] - 48);
			}else if(str[i] >= 'a' && str[i] <= 'f') {
				num += fac * (str[i] - 87);
			}else if(str[i] >= 'A' && str[i] <= 'F') {
				num += fac * (str[i] - 55);
			}
			fac *= 16;
		}
		return num;
	
	case 'b':
	case 'B':
		num = 0;
		i = 1;
		while(str[i] != '\0') {
			num <<= 1;
			num |= (str[i] == '1') ? 1 : 0;
			i += 1;
		}
		return num;
		
	default:
		return atoi(str);
	}
}
