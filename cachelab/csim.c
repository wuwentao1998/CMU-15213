#define _CRT_SECURE_NO_DEPRECATE
#include "cachelab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define TRUE 1
#define FALSE 0
#define MAXLINE 100

int num_hits = 0;
int num_misses = 0;
int num_evictions = 0;
int time = 0;

static void cacheVisit(long groupNum, long tag, cache_line** cache, int E)
{
	cache_line* group = cache[groupNum];
	int i;
	for (i = 0; i < E; i++)
	{
		if (group[i].tag == tag)
			break;
	}
	if (i == E)
	{
		num_misses++;
		int j;
		for (int j = 0; j < E; j++)
		{
			if (group[j].valid == 0)
			{
				group[j].valid = 1;
				group[j].tag = tag;
				group[j].time_stamp = time;
				break;
			}
		}
		if (j == E)
		{
			int k = oldest(group);
			group[k].tag = tag;
			group[k].time_stamp = time;
			num_evictions++;
		}
	}
	else if (group[i].valid == 1)
	{
		group[i].time_stamp = time;
		num_hits++;
	}
	else
	{
		num_misses++;
		group[i].valid = 1;
		group[i].time_stamp = time;
	}


}

static char* h2b(char c)
{
	switch (c)
	{
	case 0:
		return "0000";
	case 1:
		return "0001";
	case 2:
		return "0010";
	case 3:
		return "0011";
	case 4:
		return "0100";
	case 5:
		return "0101";
	case 6:
		return "0110";
	case 7:
		return "0111";
	case 8:
		return "1000";
	case 9:
		return "1001";
	case 10:
		return "1010";
	case 11:
		return "1011";
	case 12:
		return "1100";
	case 13:
		return "1101";
	case 14:
		return "1110";
	case 15:
		return "1111";
	}
}

typedef struct
{
	int valid;
	long tag;
	int time_stamp;
} cache_line;

int main(int argc, char* argv[])
{

	int s, S, E, b, B;
	int h = FALSE, v = FALSE;
	FILE* pFile;
	char* line;


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

	cache_line** cache = (cache_line**)malloc(S * sizeof(cache_line*));

	for (int i = 0; i < S; i++)
	{
		cache[i] = (cache_line*)malloc(E * sizeof(cache_line));
		for (int j = 0; j < E; j++)
		{
			cache[i][j].valid = 0;
			cache[i][j].tag = 0;
			cache[i][j].time_stamp = 0;
		}
	}

	if (pFile == NULL)
		perror("Error opening file");

	while (fgets(line, MAXLINE, pFile) != NULL)
	{
		if (line[0] == ' ')
			continue;
		time++;	
		char operation = line[1];

		char* offset = strchr(line, ',') + 1;
		//int objectSize = atoi(offset);

		*(offset - 1) = '\0';
		char* addressString;
		strcpy(addressString, line + 3);
		int addressSize = sizeof(addressString) / sizeof(char);
		char* address;
		for (int i = 0; i < addressSize; i++)
		{
			char* temp = h2b(addressString[i]);
			strcat(address, temp);
		}

		int tagSize = sizeof(address) / sizeof(char) - s - b;
		char * tagString;
		strncpy(tagString, address, tagSize);
		long tag = atol(tagString);

		long groupNum = -1;
		int n = 0;
		for (int i = tagSize + s - 1; i >= tagSize; i--)
		{
			groupNum += pow(2, n++) * (int)(address[i] - '0');
		}
		int isMiss;
		switch (operation)
		{
		case 'L':
			cacheVisit(groupNum, tag, cache, E);
			break;
		case 'M':
			cacheVisit(groupNum, tag, cache, E);
			num_hits++;
			break;
		case 'S':
			cacheVisit(groupNum, tag, cache, E);
			break;
		default:
			break;
		}
	}

	printSummary(num_hits, num_misses, num_evictions);
	for (int i = 0; i < S; i++)
		free(cache[i]);
	free(cache);
	fclose(pFile);
	return 0;
}
