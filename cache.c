// Jelon Anderson, Daniel Diaz
// Cache Sim Part 3
// ECE 486

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

#define SET_SIZE 1024
#define GETS(x) ((x >> 5) & 0x3FF)		//this macro will return the set number given an address
#define GETT(x) (x >> 15)			//this macro will return a tag given an address


void initilize(void);
void read(unsigned int);
void write(unsigned int);
int findMatch(unsigned int, int);
int findLine(unsigned int, int);
void addToCache(unsigned int, int);
void resetlru(unsigned int , int);
void findoldest(unsigned int);

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
	struct line line[3];
};

// Define Global variables
struct set set[SET_SIZE];
bool version_flag = false;
bool trace_flag = false;
bool dump_flag = false;
int accesses = 0;
int reads = 0;
int writes = 0;
int cycleswithcache = 0;
int cycleswithoutcache = 0;
int streamins = 0;
int streamouts = 0;
int misses = 0;
int hits = 0;
int readhits = 0;
int writehits = 0;


int main(int argc, char* argv)
{
	FILE *trace;
	char c;
	unsigned int addr, tag, setIndex;

	//open the trace file to read only	
	trace = fopen("trace", "r");
	
	//test if trace file opened properly
	if(trace == NULL){
		printf("Couldn't open file\n");
		return 1;
	}
	
	initilize();				//initilize the memory history

	//loop through the file
	for(int i = 0; i < 200; i ++){		//more readable.
//	while(1){
		//read a character from the trace file
		c = fgetc(trace);

		//test the character read from file
		if(c == 'r'){
			//get address from file
			fscanf(trace, "%x", &addr);
		
			printf("set = %d/0x%x\ttag = %d/0x%x\n", GETS(addr), GETS(addr), GETT(addr), GETT(addr));
			//call read function
			read(addr);
		} else if(c == 'w'){
			fscanf(trace, "%x", &addr);
			printf("Write at %x\n", addr);
			write(addr);
		} else if(c == '-'){
			//call debug function
		} else if(c == EOF)
			break;
	}

	for(int i = 0; i < SET_SIZE; i ++)
	{
		if(set[i].line[0].history != NULL)
			printf("line %d\n", i);
	}

	fclose(trace);

	return 0;
}


void initilize(void)
{
	// Constructor for struct, declare LLL to be NULL
	for(int i = 0; i < SET_SIZE; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			set[i].line[j].history = NULL;
		}
	}
}

void read(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0)
	{		//match found
		printf("\tmatch found! %d\n", findMatch(address, 0));
	}
	else
	{					//no match found, find someone to evict
		printf("no match found\n");
		addToCache(address, findLine(address, 0));
	}
}


void write(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0)
	{	//match found
		printf("\tmatch found! %d\n", findMatch(address, 0));
	}
	else
	{					//no match found, find someone to evict
		printf("no match found\n");
		addToCache(address, findLine(address, 0));
	}
}


void findoldest(unsigned int addr)
{
	int LRU[3];
	// Store all line number in array
	for (int i = 0; i < 4; i++)
	{
		LRU[i] = set[GETS(addr)].line[i].lru;		//get line lru
	}
	
	int largest=LRU[0];
	int linenum;
	// Iterate to find the largest
	for (int i = 1; i < 4; i++)
    	{
        	if (largest < LRU[i])
		{
            		largest = LRU[i];
			linenum = i;
    		}
	}	
	
	resetlru(addr,linenum);
}

// Iterate through the address set and increment lru
void resetlru(unsigned int addr, int line)
{
	for (int i = 0; i < 4; i++)
	{
		set[GETS(addr)].line[i].lru += 1;		// Increment each line's LRU bit
	}

	set[GETS(addr)].line[line].lru = 0;			// Reset lru to most recent
}

//recursively find tag match
int findMatch(unsigned int address, int lineI)
{
	if(lineI == 4)
		return -1;
	else if(set[GETS(address)].line[lineI].valid){
		if(set[GETS(address)].line[lineI].tag == GETT(address))
			return lineI;}
	else
		return findMatch(address, lineI + 1);
}

//recursively find available line
int findLine(unsigned int address, int lineI)
{

	if(lineI == 4){
		printf("placing in line 0, LRU testing should go here");
		return 0;
	}
	else if(set[GETS(address)].line[lineI].valid)
		return findLine(address, lineI + 1);	
	else{
		printf("\tset = 0x%x\tline = 0x%x", GETS(address), lineI);
		return lineI;
	}		
}

//add the address to the specified line
void addToCache(unsigned int addr, int lineI)
{
	//allocate new node
	struct memNode * node = (struct memNode*) malloc(sizeof(struct memNode));
	
	//set address of node
	node -> address = addr;
	node -> next = set[GETS(addr)].line[lineI].history;	//point next node to history of line
	
	set[GETS(addr)].line[lineI].history = node;		//line history to new nod
	set[GETS(addr)].line[lineI].valid = true;		//set valid
	set[GETS(addr)].line[lineI].tag = GETT(addr);		//set tag
}
