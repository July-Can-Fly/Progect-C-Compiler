#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <ctype.h>
#include<limits.h>
#include"code.h"

char* binary[] = { "0000","0001","0010","0011","0100","0101","0110","0111","1000","1001","1010","1011","1100","1101","1110","1111" };
char hexChar[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

void processFile(FILE* programFile, char* fileName);
void intToBinArray(unsigned int n, char* binArray, char* hexArray);
unsigned int genMachineCode(Command* pCmd, Label* pLabel, int commandIndex, int commandOffset);
void freeMemory(Label* headLabel, Entry* headEntry);

/**
 * A  table instractions with opcode, funct by command name, operands, registers or labels
 */
Instruction instructions[] = {
		{"add", 0, 1, R, 3, {Register, Register, Register}, 0},
		{"sub", 0, 2, R, 3, {Register, Register, Register},0},
		{"and", 0, 3, R, 3, {Register, Register, Register},0},
		{"or", 0, 4, R, 3, {Register, Register, Register},0},
		{"nor", 0, 5, R, 3, {Register, Register, Register},0},
		{"move", 1, 1, R, 2, {Register, Register, 0},0},
		{"mvhi", 1, 2, R, 2, {Register, Register, 0},0},
		{"mvlo", 1, 3, R, 2, {Register, Register, 0},0},
		{"addi", 10, 0, I, 3, {Register, Literal, Register},0},
		{"subi", 11, 0, I, 3, {Register, Literal, Register},0},
		{"andi", 12, 0, I, 3, {Register, Literal, Register},0},
		{"ori", 13, 0, I, 3, {Register, Literal, Register},0},
		{"nori", 14, 0, I, 3, {Register, Literal, Register},0},
		{"bne", 15, 0, I, 3, {Register, Register, LineLabel},0},
		{"beq", 16, 0, I, 3, {Register, Register, LineLabel},0},
		{"blt", 17, 0, I, 3, {Register, Register, LineLabel},0},
		{"bgt", 18, 0, I, 3, {Register, Register, LineLabel},0},
		{"lb", 19, 0, I, 3, {Register, Literal, Register},1},
		{"sb", 20, 0, I, 3, {Register, Literal, Register},1},
		{"lw", 21, 0, I, 3, {Register, Literal, Register},1},
		{"sw", 22, 0, I, 3, {Register, Literal, Register},1},
		{"lh", 23, 0, I, 3, {Register, Literal, Register},1},
		{"sh", 24, 0, I, 3, {Register, Literal, Register},1},
		{"jmp", 30, 0, J, 1, {RegisterOrLabel, 0, 0},0},
		{"la", 31, 0, J, 1, {LineLabel, 0, 0},1},
		{"call", 32, 0, J, 1, {LineLabel, 0, 0},0},
		{"stop", 63, 0, J, 0, {0, 0, 0},0}
};


int main(int argc, char* argv[]) {
	int i;
	char fileName[50];
	FILE* programFile;
	char search[] = ".as";
	int progIndex;
	for (progIndex = 1; progIndex<argc; progIndex++) {
		i = strlen(argv[progIndex])-3;
		if (strcmp(argv[progIndex] + i, search) != 0)
		{
			printf("'%s' doesn't end with '%s'\n", argv[progIndex], search);
		}
		else
		{
			memcpy(fileName, argv[progIndex], i);
			fileName[i] = '\0';
			programFile = fopen(argv[progIndex], "r");
			if (programFile == NULL) {
				/* if file couldn't be opened, write to stderr. */
				printf("Error in file opening\n");
				return -1;
			}
			processFile(programFile, fileName);
			fclose(programFile);
		}
	}
	return 0;
}

			

/**
 * Validates the operands addressing types, and prints error message if needed 
 * also processes a single line in the first and second pass
  */
void processFile(FILE* programFile, char* fileName) {

	char entName[50] = { 0x00 }, extName[50] = { 0x00 }, obName[50] = { 0x00 };
	Command command[200];
	unsigned char data[500] = { 0x00 };
	int commandOffset = 100, commandIndex = 0, dataCount = 0, j = 0, charIx = 0, flagLineError = 0, flagProgramError = 0;
	int lineHasLabel = 0, length = 0, sign = 0;
	int minValue = 0, maxValue = 0, valueInt = 0, numInstructions = sizeof(instructions) / sizeof(instructions[0]), instIx = 0, parmIx = 0;
	int commandCount = 0;
	short valueShort = 0;
	long num = 0;
	char valueChar = 0, endOperand = 0;
	char line[80] = { 0x00 };
	Label* headLabel = NULL, * pLabel = NULL;
	Entry* headEntry = NULL, * pEntry = NULL;
	char* ptr, * start, * endLine;
	FILE* entryFile;
	FILE* output_file;
	FILE* externalFile;
	char* cmdLabel;
	unsigned int machineCode = 0;
	char binaryCode[33] = { '\0' };
	char hexCode[9] = { '\0' };
	int i = 0;
	unsigned int digit1 = 0, digit2 = 0;
	unsigned int temp2 = 100;
	sprintf(obName, "%s.ob", fileName);
	sprintf(entName, "%s.ent", fileName);
	sprintf(extName, "%s.ext", fileName);

	while (!feof(programFile) && fgets(line, MAX_LINE_LENGTH, programFile) != NULL)
	{
		flagLineError = lineHasLabel = 0;
		
		j = strspn(line, " \t");
		length = strlen(line);
		endLine = line + length;
		if (*(endLine - 1) == '\n')
		{
			length--;
			endLine--;
			*endLine = '\0';
			if (*(endLine - 1) == '\r')
			{
				length--;
				endLine--;
				*endLine = '\0';
			}
		}
		/*if this line not comment*/
		if (j < length && line[0] != ';')
		{
			/* Check if line starting with '.' */
			if (line[0] == '.')
			{
				/*find extern and if it's entry allocate memory witn malloc*/
				if (memcmp(line + 1, "extern", 6) == 0 && (line[7] == ' ' || line[7] == '\t'))
				{
					/*find first non blank or tab after extern*/
					charIx = strspn(line + 8, " \t");
					start = line + 8 + charIx;
					pLabel = (Label*)malloc(sizeof(Label));
					pLabel->next = headLabel;/*save in linked list*/
					headLabel = pLabel;
					strcpy(pLabel->name, start);
					pLabel->offset = -1;
					pLabel->isExternal = 1;
				}
				else
				{
					/*find entry and if it's entry allocate memory witn malloc*/
					if (memcmp(line + 1, "entry", 5) == 0 && (line[6] == ' ' || line[6] == '\t'))
					{
						/*find first non blank or tab after entry*/
						charIx = strspn(line + 7, " \t");
						start = line + 7 + charIx;
						pEntry = (Entry*)malloc(sizeof(Entry));
						pEntry->next = headEntry;
						headEntry = pEntry;
						strcpy(pEntry->name, start);
					}
					else
					{
						flagLineError = 1;
						printf("error not extern or entry\n");
					}
				}
			}
			else
			{
				start = line;
				if (line[0] != ' ' && line[0] != '\t')
				{
					charIx = strcspn(line, ": \t");/*skip all blanks and label*/
					if (line[charIx] != ':')
					{
						flagLineError = 1;
						printf("error label must end with a colon\n");
					}
					else
					{
						line[charIx] = '\0';
						pLabel = (Label*)malloc(sizeof(Label));
						if (pLabel == NULL)
						{
							printf("malloc failed");
							exit(-1);
						}
						pLabel->next = headLabel;
						pLabel->isExternal = 0;
						headLabel = pLabel;
						strcpy(pLabel->name, line);
						start = line + charIx + 1;
						lineHasLabel = 1;
					}
				}
				if (flagLineError == 0)
				{
					start+= strspn(start, " \t");
					if (start >= endLine)
					{
						flagLineError = 1;
						printf("no data after label\n");
					}
					else
					{
						
						/*if now is '.' we find data operand*/
						if (start[0] == '.')
						{
							if (lineHasLabel)
							{
								pLabel->offset = dataCount;
								pLabel->isDataLabel = 1;
							}
							switch (start[1])
							{
							case 'd':
								switch (start[2])
								{
								case 'b':
									minValue = CHAR_MIN;
									maxValue = CHAR_MAX;
									length = 1;
									break;
								case 'h':
									minValue = SHRT_MIN;
									maxValue = SHRT_MAX;
									length = 2;
									break;
								case 'w':
									minValue = INT_MIN;
									maxValue = INT_MAX;
									length = 4;
									break;
								default:
									flagLineError = 1;
									printf("invalid data type\n");
								}
								if (start[3] != ' ')
								{
									flagLineError = 1;
									printf("data type error\n");
								}
								if (flagLineError == 0)
								{
									charIx = strspn(start + 3, " \t");
									ptr = start + 3 + charIx;
									do
									{
										sign = 1;
										if (*ptr == '-')
										{
											sign = -1;
											ptr++;
										}
										num = atol(ptr) * sign;
										if (num<minValue || num >maxValue)
										{
											flagLineError = 1;
											printf("Value no in range\n");
										}
										else
										{
											switch (length)
											{
											case 1:
												valueChar = (char)num;
												data[dataCount] = valueChar;
												break;
											case 2:
												valueShort = (short)num;
												memcpy(data + dataCount, &valueShort, length);
												break;
											case 4:
												valueInt = (int)num;
												memcpy(data + dataCount, &valueInt, length);
												break;
											}
											dataCount += length;
										}
										charIx = strspn(ptr, "0123456789");
										ptr += charIx;
										if (*ptr == ' ' || *ptr == '\t')
										{
											charIx = strspn(ptr, " \t");
											ptr += charIx;
										}
										if (*ptr == ',')
										{
											charIx = strspn(ptr + 1, " \t");
											ptr = ptr + 1 + charIx;
										}
									} while (ptr < endLine);
								}
								break;
							case 'a':
								if (memcmp(start + 2, "sciz", 4) != 0 && (*(start + 6) == ' ' || *(start + 6) == '\t'))
								{
									flagLineError = 1;
									printf("invalid operation\n");
								}
								else
								{
									charIx = strspn(start + 7, " \t");
									start = start + 7 + charIx;
									if (*start != '"')
									{
										flagLineError = 1;
										printf("missing double quote\n");
									}
									else
									{
										start++;
										ptr = strchr(start, '"');
										if (ptr == NULL)
										{
											flagLineError = 1;
											printf("missing closing quotes");
										}
										else
										{
											length = ptr - start;
											memcpy(data + dataCount, start, length);
											dataCount += length;
											data[dataCount] = '\0';
											dataCount++;
										}
									}
								}
								break;
							default:
								flagLineError = 1;
								printf("invalid data operation\n");
							}
						}
						else
						{
							if (lineHasLabel)
							{
								pLabel->offset = commandOffset;
								pLabel->isDataLabel = 0;
							}
							ptr = start + strcspn(start, " \t");
							if (ptr != NULL)
							{
								*ptr = '\0';
								ptr++;
							}
							else
							{
								ptr = endLine;
							}
							for (instIx = 0; instIx < numInstructions && strcmp(instructions[instIx].name, start) != 0; instIx++)
							{
							}
							if (instIx >= numInstructions)
							{
								flagLineError = 1;
								printf("invalid instruction\n");
							}
							else
							{
								command[commandIndex].labelText[0] = '\0';
								command[commandIndex].instIndex = instIx;
								command[commandIndex].operands[0] = command[commandIndex].operands[1] = command[commandIndex].operands[2] = 0;
								for (parmIx = 0; parmIx < instructions[instIx].numOperands && flagLineError == 0; parmIx++)
								{
									charIx = strspn(ptr, " \t");
									start = ptr + charIx;
									if (start >= endLine)
									{
										printf("missing operand\n");
										flagLineError = 1;
									}
									else
									{
										if (parmIx > 0)
										{
											if (*start != ',')
											{
												flagLineError = 1;
												printf("missing comma between operands\n");
											}
											else
											{
												charIx = strspn(start + 1, " \t");
												start = start + 1 + charIx;
												if (start >= endLine)
												{
													printf("missing operand\n");
													flagLineError = 1;
												}
											}
										}
									}
									if (flagLineError == 0)
									{
										ptr = start + strcspn(start, ", \t");
										endOperand = *ptr;
										*ptr = '\0';
										if (instructions[instIx].operandTypes[parmIx] == Register ||
											instructions[instIx].operandTypes[parmIx] == RegisterOrLabel)
										{
											if (*start != '$' && instructions[instIx].operandTypes[parmIx] == Register)
											{
												flagLineError = 1;
												printf("register operand required\n");
											}
											else
											{
												valueInt = atoi(start + 1);
												if (valueInt < 0 || valueInt>31)
												{
													flagLineError = 1;
													printf("invalid register number\n");
												}
												else
												{
													command[commandIndex].operands[parmIx] = valueInt;
												}
											}
										}
										if (instructions[instIx].operandTypes[parmIx] == LineLabel ||
											(instructions[instIx].operandTypes[parmIx] == RegisterOrLabel &&
												*start != '$'))
										{
											if (!isalpha(*start))
											{
												flagLineError = 1;
												printf("invalid label\n");
											}
											else
											{
												strcpy(command[commandIndex].labelText, start);
												command[commandIndex].operands[parmIx] = -1;
											}
										}
										if (instructions[instIx].operandTypes[parmIx] == Literal)
										{
											sign = 1;
											if (*start == '-')
											{
												sign = -1;
												start++;
											}
											num = atol(start) * sign;
											if (num < SHRT_MIN || num > SHRT_MAX)
											{
												flagLineError = 1;
												printf("Value no in range\n");
											}
											else
											{
												command[commandIndex].operands[parmIx] = (int)num;
											}
										}
									}
									*ptr = endOperand;
								}
								if (ptr < endLine)
								{
									ptr += strspn(ptr, " \t");
									if (ptr < endLine)
									{
										flagLineError = 1;
										printf("too many parameters\n");
									}
								}
								if (instructions[instIx].numOperands == 2 && instructions[instIx].commandType == R)
								{
									command[commandIndex].operands[2] = command[commandIndex].operands[1];
									command[commandIndex].operands[1] = 0;
								}
								
								commandIndex++;
								commandOffset += 4;
							}
						}
					}
				}
			}
		}
		if (flagLineError == 1)
		{
			flagProgramError = 1;
		}
	}
	/*fclose(programFile);*/
	if (flagProgramError == 1)
	{
		printf("compiler errors\n");
		freeMemory(headLabel, headEntry);
		return;
	}
	commandCount = commandIndex;

	/*Write* .ext  file*/
	externalFile = fopen(extName, "w+");


	for (commandIndex = 0; commandIndex < commandCount; commandIndex++)
	{
		pLabel = NULL;
		cmdLabel = command[commandIndex].labelText;
		if (*cmdLabel != '\0')
		{
			for (pLabel = headLabel; pLabel != NULL && strcmp(cmdLabel, pLabel->name) != 0; pLabel = pLabel->next)
			{
			}
			if (pLabel == NULL)
			{
				flagLineError = 1;
				printf("Label not found: %s\n", cmdLabel);
			}
			else
			{
				if (pLabel->isExternal)
				{
					fprintf(externalFile, "%s %04d\n", pLabel->name, 4 * commandIndex + 100);
				}
			}
		}

		machineCode = genMachineCode(&command[commandIndex], pLabel, commandIndex, commandOffset);
		if (machineCode == 0)
		{
			flagLineError = 1;
		}
		command[commandIndex].machineCode = machineCode;
	}

	fclose(externalFile);
	if (flagLineError == 1)
	{
		printf("Compile errors in second pass\n");
		remove(extName);
		freeMemory(headLabel, headEntry);
		return;
	}
	/*Write *.ent  file*/
	entryFile = fopen(entName, "w+");
	for (pEntry = headEntry; pEntry != NULL; pEntry = pEntry->next)
	{
		for (pLabel = headLabel; pLabel != NULL && strcmp(pEntry->name, pLabel->name) != 0; pLabel = pLabel->next)
		{
		}
		if (pLabel == NULL)
		{
			flagLineError = 1;
			printf("Entry not found: %s\n", pEntry->name);
		}
		else
		{
			fprintf(entryFile, "%s %04d\n", pEntry->name, pLabel->offset + (pLabel->isDataLabel ? commandOffset : 0));
		}
	}
	fclose(entryFile);

	if (flagLineError == 1)
	{
		printf("Compile errors in second pass\n");
		remove(entName);
		freeMemory(headLabel, headEntry);
		return;
	}

	/*Write the code and data in file .ob*/
	output_file = fopen(obName, "w+");

	/* print data/code word count on top */
	fprintf(output_file, "%7d %d\n", (commandOffset - 100), dataCount);


	for (commandIndex = 0; commandIndex < commandCount; commandIndex++)
	{
		if (commandIndex < commandCount)
		{
			intToBinArray(command[commandIndex].machineCode, binaryCode, hexCode);
			fprintf(output_file, "%04d %s\n", temp2, hexCode);
			temp2 += 4;
		}

	}

	for (j = 0; j < dataCount; j += 4)
	{
		fprintf(output_file, "%04d ", temp2);
		for (i = 0; i < 4 && i + j < dataCount; i++)
		{
			digit1 = (unsigned int)data[j + i] % 16;
			digit2 = (unsigned int)data[j + i] / 16;
			fprintf(output_file, "%c%c", hexChar[digit2], hexChar[digit1]);
		}
		fprintf(output_file, "  ");
		fprintf(output_file, "\n");
		temp2 += 4;
	}
	/* Close the files */
	fclose(output_file);
	freeMemory(headLabel, headEntry);
}


/*Function to free memory allocated with malloc*/
void freeMemory(Label * headLabel, Entry* headEntry)
{
	Label* pLabel;
	Entry*  pEntry;
	pLabel = headLabel;
	while (pLabel != NULL)
	{		
		headLabel = pLabel;
		pLabel = pLabel->next;
		free(headLabel);		
	}
	pEntry = headEntry;
	while (pEntry != NULL)
	{
		headEntry = pEntry;
		pEntry = pEntry->next;
		free(headEntry);
	}
}
	
	
/*Encodes the instructions according to the table of type instructions and their identification codes and create picture of memory*/
unsigned int genMachineCode(Command *pCmd, Label* pLabel, int commandIndex , int commandOffset)
{
	Instruction* pInst = &instructions[pCmd->instIndex];
	unsigned int machineCode = 0;

	switch (pInst->commandType)
	{
		case R:
			machineCode = (pInst->opcode << 26) + (pCmd->operands[0] << 21) + (pCmd->operands[1] << 16) + (pCmd->operands[2] << 11)
				+ (pInst->funct << 6);
			break;
		case I: 
			machineCode = (pInst->opcode << 26) + (pCmd->operands[0] << 21);
			if (pInst->operandTypes[1] == Literal)
			{
				machineCode += (pCmd->operands[2] << 16);
				if (pCmd->operands[1] < 0)
				{
					unsigned int temp = USHRT_MAX + pCmd->operands[1] + 1;
					machineCode += temp;
				}
				else
				{
					machineCode += pCmd->operands[1];
				}
			}
			else
			{
				machineCode += (pCmd->operands[1] << 16);
				if (!pLabel->isExternal)
				{
					int offset = pLabel->offset - 100 - 4 * commandIndex;
					if (offset < 0)
					{
						offset = USHRT_MAX + offset + 1;
					}
					machineCode += offset;
					if (pLabel->isDataLabel != pInst->isLabelData)
					{
						printf("Label type mismatch (data - instruction)\n");
						machineCode = 0;
					}
				}
			}
			break;
		case J:
			machineCode = pInst->opcode << 26;
			if (pCmd->operands[0] >= 0)
			{
				machineCode = machineCode + pCmd->operands[0];
				if (pInst->opcode == 30)
				{
					machineCode += (1 << 25);
				}
			}
			else
			{
				if (!pLabel->isExternal)
				{
					machineCode = machineCode + pLabel->offset;
					if (pInst->opcode == 31)
					{
						machineCode += commandOffset;
					}
					if (pLabel->isDataLabel != pInst->isLabelData)
					{
						printf("Label type mismatch (data - instruction)\n");
						machineCode = 0;
					}
				}
			}
			break;
	}
	return machineCode;
}

