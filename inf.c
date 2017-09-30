#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char* argv[])
{
	if (argc != 3)
	{
		int i;	
		for (i = 0; i < argc; i++)
		{
			printf("args: %s\n", argv[i]);
		}
		fprintf(stderr, "Usage: inf tag interval\n");
	}
	else
	{
		const char* tag = argv[1];
		int interval = atoi(argv[2]);
		while(1)
		{
			printf("%s\n", tag);
			sleep(interval);
		}
	}
}
