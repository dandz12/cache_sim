/************************************************************************************
*using unsigned int now instead of unsigned long, int is 32 bits
*
*
*
*************************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

struct memNode{
	unsigned int address;
	struct memNode* next;
};

struct line{
	bool valid;
	bool dirty;
	int lru;
	int tag;
	struct memNode* history;
};

struct set{
	struct line line[4];
};

int main(int argc, char* argv)
{
	FILE *trace;
	struct set set[1024];
	char c;
	unsigned int addr, tag, setIndex;

	
	trace = fopen("trace", "r");

	if(trace == NULL){
		printf("Couldn't open file\n");
		return 1;
	}
	for(int i = 0; i < 1024; i ++){
		set[i].line[0].history = NULL;
	}
		
	//loop through the file
	for(int i = 0; i < 20; i ++){		//more readable.
//	while(1){

		c = fgetc(trace);

		if(c == 'r'){
			fscanf(trace, "%x", &addr);
			
			//determine tag mask with 0xFFC00000, this grabs the leftmost 10 bits			
			setIndex = (addr >> 5) & 0x3FF;
			//printf("new addr = %x, set number = %u\n", tag, tag);
			printf("set = 0x%x\n", setIndex);
			//allocate node
			struct memNode* node= (struct memNode*) malloc (sizeof(struct memNode));
			
			node -> address = addr;
			node -> next = set[setIndex].line[0].history;
			set[setIndex].line[0].history = node;
			printf("Read at %x\n", addr);
		
			
		} else if(c == 'w'){
			fscanf(trace, "%x", &addr);
			printf("Write at %x\n", addr);
		} else if(c == '-'){
			//call debug function
		} else if(c == EOF)
			break;
	}

/*	//print the history of set0 line[0]
	printf("reads at ");
	while(set[0].line[0].history != NULL){
		printf("%x ", set[0].line[0].history->address);
		set[0].line[0].history = set[0].line[0].history->next;
	}
	printf("\n");*/



	for(int i = 0; i < 1024; i ++)
	{
		if(set[i].line[0].history != NULL)
			printf("line %d\n", i);
	}

	fclose(trace);

return 0;
}
