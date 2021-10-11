#ifndef _Type_H
#define _Type_H

/** Maximum length of a single source line  */
#define MAX_LINE_LENGTH 80

typedef enum {
	I = 0
	, R
	, J
} CommandType;

typedef enum {
	Register = 1
	, Literal
	, LineLabel
	, RegisterOrLabel
} OperandType;

typedef struct {
	unsigned int machineCode;
	int instIndex;
	int operands[3];
	char labelText[20];
}Command;

typedef struct Label {
	char name[20];
	int offset;
	int isExternal;
	struct Label* next;
	int isDataLabel;
}Label;

typedef struct {
	char* name;
	int opcode;
	int funct;
	CommandType commandType;
	int numOperands;
	OperandType operandTypes[3];
	int isLabelData;
} Instruction;

typedef struct Entry {
	char name[20];
	struct Entry* next;
}Entry;

/**
* Processes a single line in the first pass and second pass. 
* Validates the operands addressing types, and prints error message if needed 
* Identifys the type of data, and encode it in the memory image of the data
* @param line The line text
* @param command The current command string;
* @param data current data
* @param commandOffset place were adress command
* @param numInstructions 
* @param commandCount The current data counter
* @param valueShort data short digit
* @param num data integers
* @param headLabel, pLabel variables for labels
* @param headEntry,  pEntry variables to keep entry data
* @param * ptr, * start, * endLine variables to moving throut line
* @param entryFile file name for entry data
* @param output_file file name for code and data in file .ob
* @param externalFile file name for external data
* @param cmdLabel
* @param machineCode represents a general machine code word contents
* @paramt digit1, digit2;
* @param binaryCode[33] array to keep numbers in binary number system
* @param hexCode[9] array to keep numbers in hexadecimal number system
* @param temp2 variable for set offset 100

 */
void processFile(FILE* programFile, char* fileName);

/**
* Convert decimal numbers to binary and hexadecimal number system
* @param hexDigit array for convert  to hexadecimal number system
* @param binArray array for convert to binary number system
*/
void intToBinArray(unsigned int n, char* binArray, char* hexArray);

/**
* Encodes the instructions according to the table of type instructions and their identification codes and create picture of memory
* @param machineCode the "encoding" line according instruction 
*/
#endif


