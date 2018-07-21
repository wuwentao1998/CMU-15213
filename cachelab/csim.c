#define _CRT_SECURE_NO_DEPRECATE
#include "cachelab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define TRUE 1
#define FALSE 0
#define MAXLINE 100

typedef struct
{
	int valid;
	int tag;
	int index;
} cache_line;

int main(int argc, char* argv[])
{
	int s, S, E, b, B;
	int h = FALSE, v = FALSE;
	FILE* pFile;
	char* line;
	int num_hits;
	int num_misses;
	int num_evictions;

	if (argc == 10)
	{
		if ((int)sizeof(argv[1]) == 3)
		{
			h = TRUE;
			v = TRUE;
		} 
		else if (argv[1][0] == 'h')
			h = TRUE;
		else 
			v = TRUE;
	}

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-s") == 0)
			s = atoi(argv[i + 1]);
		else if (strcmp(argv[i], "-E") == 0)
			E = atoi(argv[i + 1]);
		else if (strcmp(argv[i], "-b") == 0)
			b = atoi(argv[i + 1]);
		else if (strcmp(argv[i], "-t") == 0)
			pFile = fopen(argv[i + 1], "r");
	}

	S = pow(2, s);
	B = pow(2, b);
	int tagSize = 8 - s - b;

	cache_line** cache = (cache_line**)malloc(S * sizeof(cache_line*));

	for (int i = 0; i < S; i++)
	{
		cache[i] = (cache_line*)malloc(E * sizeof(cache_line));
		for (int j = 0; j < E; j++)
		{
			cache[i][j].valid = 0;
			cache[i][j].tag = 0;
			cache[i][j].index = -1;
		}
	}

	if (pFile == NULL)
		perror("Error opening file");

	while (fgets(line, MAXLINE, pFile) != NULL)
	{
		if (line[0] == ' ')
			continue;
		char operation = line[0];
		char* address;
		strncpy(address, line + 3, 8);

		char* tagString;
		strncpy(tagString, address, tagSize);
		int tag = atoi(tagString);

		int groupNum = 0;
		for (int i = tagSize + s; i >= tagSize; i--)
		{
			groupNum += pow(2, (int)(address[i] - '0'));
		}

		int objectSize = (int)(line[12] - '0');
		
		switch (operation)
		{
		case 'L':

			break;
		case 'M':

			break;
		case 'S':

			break;
		default:
			break;
		}
	}

	//printSummary(num_hits, num_misses, num_evictions);
	return 0;
}
