/*
MIT License

Copyright (c) 2022 Ruslan Popov <ruslanpopov1512@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ASM_START \
".8086\n"\
"code SEGMENT para USE16 PUBLIC 'code'\n"\
"\tASSUME CS:code, DS:data, SS:stack\n"\
"\tMAIN:\n"\
"\t\tmov ax, data\n"\
"\t\tmov ds, ax\n"\
"\t\tmov ax, 0200h\n"\
"\t\tmov dl, 0\n"\
"\t\tmov bx, 0\n"\
"\t\tmov cx, 0\n"

#define ASM_END "\
		\n\
		mov ax, 4C00h\n\
		int 21h\n\
code ends\n\
\n\
data SEGMENT para USE16 PUBLIC 'data'\n\
	db 30000 DUP(0)\n\
data ends\n\
\n\
stack SEGMENT para USE16 STACK 'stack'\n\
	db 256 DUP(?)\n\
stack ends\n\
end MAIN"

#define MAX_CELLS 30000
#define MAX_FILE_SIZE 30000

int interpret(const char* fileContent, const size_t fileSize);
int generateASM(const char* fileContent, const size_t fileSize, const char* fileOutputName);

#define MAX_NESTING_LEVEL 32
size_t stackPointer = 0;
size_t stack[MAX_NESTING_LEVEL];
void push(size_t count);
size_t pop();

int main(int argc, char* argv[]) {
	printf("bf1-inanyan v0.1 - Brainfuck interpreter and assembler\n");
	printf("Copyright (C) 2022 Ruslan Popov <ruslanpopov1512@gmail.com>\n\n");

	if (argc < 2)
	{
		printf("ERROR: No file specified\n");
		return 1;
	}
	else if (argc > 3)
	{
		printf("ERROR: Too much arguments\n");
		return 2;
	}

	FILE* fRead;
	fRead = fopen(argv[1], "r");
	if (fRead == NULL)
	{
		printf("ERROR: Can't open read file\n");
		return 3;
	}

	char* fileContent = (char*)malloc(MAX_FILE_SIZE);
	if (fileContent == NULL)
	{
		printf("ERROR: Can't allocate memory\n");
		fclose(fRead);
		return 4;
	}

	size_t fileSize = 0;
	while ((fileContent[fileSize++] = fgetc(fRead)) > 0);
	fclose(fRead);

	int res = 0;
	if (argc == 3) res = generateASM(fileContent, fileSize, argv[2]);
	else res = interpret(fileContent, fileSize);

	free(fileContent);
	return res;
}

int interpret(const char* fileContent, const size_t fileSize)
{
	char* memory = (char*)malloc(30000);
	if (memory == NULL)
	{
		printf("ERROR: Can't allocate memory\n");
		return 5;
	}
	memset(memory, 0, MAX_CELLS);
	size_t pointer = 0;

	size_t filePos = 0;
	size_t skip = 0;

	while (filePos < fileSize)
	{
		char c = fileContent[filePos];

		if (c == '>') pointer++;
		else if (c == '<') pointer--;
		else if (c == '+') memory[pointer]++;
		else if (c == '-') memory[pointer]--;
		else if (c == '.') putchar(memory[pointer]);
		else if (c == ',') memory[pointer] = getchar();
		else if (c == '[')
		{
			if (memory[pointer] == 0)
			{
				while (filePos < fileSize)
				{
					filePos++;
					if (fileContent[filePos] == '[') skip++;
					else if (fileContent[filePos] == ']' && skip > 0) skip--;
					else if (fileContent[filePos] == ']' && skip == 0) break;
				}
			}
		}
		else if (c == ']')
		{
			if (memory[pointer] != 0)
			{
				while (filePos < fileSize)
				{
					filePos--;
					if (fileContent[filePos] == ']') skip++;
					else if (fileContent[filePos] == '[' && skip > 0) skip--;
					else if (fileContent[filePos] == '[' && skip == 0) break;
				}
			}
		}
		filePos++;
	}

	free(memory);
	return 0;
}

int generateASM(const char* fileContent, const size_t fileSize, const char* fileOutputName)
{
	FILE* work;
	work = fopen(fileOutputName, "w");
	if (work == NULL)
	{
		printf("ERROR: Can't open output file\n");
		return 6;
	}
	fprintf(work, ASM_START);

	size_t filePos = 0;
	size_t loopsCount = 0;

	while (filePos < fileSize)
	{
		char c = fileContent[filePos];
		if (c != '[' && c != ']') fprintf(work, "\t\t");

		if (c == '>')
		{
			fprintf(work, "mov ds:[bx], dl\n\t\t");
			int toAdd = 1;
			if (fileContent[filePos + 1] == '>')
			{
				filePos++;
				while (filePos < fileSize && fileContent[filePos] == '>')
				{
					toAdd++;
					filePos++;
				}
				filePos--;
			}
			fprintf(work, "add bx, %d\n\t\t", toAdd);
			fprintf(work, "mov dl, ds:[bx]");
		}
		else if (c == '<')
		{
			fprintf(work, "mov ds:[bx], dl\n\t\t");
			int toDec = 1;
			if (fileContent[filePos + 1] == '<')
			{
				filePos++;
				while (filePos < fileSize && fileContent[filePos] == '<')
				{
					toDec++;
					filePos++;
				}
				filePos--;
			}
			fprintf(work, "sub bx, %d\n\t\t", toDec);
			fprintf(work, "mov dl, ds:[bx]");
		}
		else if (c == '+')
		{
			int toAdd = 1;
			if (fileContent[filePos + 1] == '+')
			{
				filePos++;
				while (filePos < fileSize && fileContent[filePos] == '+')
				{
					toAdd++;
					filePos++;
				}
				filePos--;
			}
			fprintf(work, "add dl, %d", toAdd);
		}
		else if (c == '-')
		{
			int toDec = 1;
			if (fileContent[filePos + 1] == '-')
			{
				filePos++;
				while (filePos < fileSize && fileContent[filePos] == '-')
				{
					toDec++;
					filePos++;
				}
				filePos--;
			}
			fprintf(work, "sub dl, %d", toDec);
		}
		else if (c == '.')
		{
			fprintf(work, "mov ah, 02h\n\t\tint 21h");
		}
		else if (c == ',')
		{
			fprintf(work, "mov ah, 01h\n\t\tint 21h\n\t\tmov dl, al");
		}
		else if (c == '[')
		{
			loopsCount++;
			if (stackPointer == MAX_NESTING_LEVEL - 1)
			{
				printf("ERROR: Max netsing level reached\n");
				fclose(work);
				return 7;
			}
			push(loopsCount);
			fprintf(work, "\ncmp dl, 0\njz mark_%zu_1\nmark_%zu_0: ", loopsCount, loopsCount);
		}
		else if (c == ']')
		{
			if (stackPointer == 0)
			{
				printf("ERROR: Unbalanced brackets\n");
				fclose(work);
				return 8;
			}
			size_t count = pop();
			fprintf(work, "\ncmp dl, 0\njnz mark_%zu_0\nmark_%zu_1: ", count, count);
		}

		fputc('\n', work);
		filePos++;
	}

	if (stackPointer != 0)
	{
		printf("ERROR: Unbalanced brackets\n");
	}

	fprintf(work, ASM_END);
	fclose(work);
	return 0;
}

void push(size_t count)
{
	stack[stackPointer++] = count;
}

size_t pop()
{
	return stack[--stackPointer];
}