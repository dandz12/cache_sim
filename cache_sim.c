/************************************************************************************
*using unsigned int now instead of unsigned long, int is 32 bits
*
*
*
*************************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

#define SET_SIZE 1024
#define GETS(x) ((x >> 5) & 0x3FF)		//this macro will return the set number given an address
#define GETT(x) (x >> 15)			//this macro will return the tag given an address

void initilize(void);
void read(unsigned int);
void write(unsigned int);
int findMatch(unsigned int, int);
int findLine(unsigned int, int);
void addToCache(unsigned int, int);
void resetlru(unsigned int , int);
int findoldest(unsigned int);

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
//	for(int i = 0; i < 200; i ++){		//more readable.
	while(1){
		//read a character from the trace file
		c = fgetc(trace);

		//test the character read from file
		if(c == 'r'){
			//get address from file
			fscanf(trace, "%x", &addr);
			reads++;		//increment reads
			accesses++;		//increment accesses

			//call read function
			read(addr);
		} else if(c == 'w'){
			//get address from file
			fscanf(trace, "%x", &addr);
			writes++;		//increment writes
			accesses++;		//increment accesses

			//call write function
			write(addr);
		} else if(c == '-'){
			//call debug function
		} else if(c == EOF)
			break;
	}
	cycleswithcache = ((hits * 1) + (misses * 51));
	cycleswithoutcache = (accesses * 50);

	//Print the results
	printf("Accesses:\t\t%d\n", accesses);
	printf("Reads:\t\t\t%d\n", reads);
	printf("Writes:\t\t\t%d\n", writes);
	printf("Stream-in:\t\t%d\n", streamins);
	printf("Stream-out:\t\t%d\n", streamouts);
	printf("Misses:\t\t\t%d\n", misses);
	printf("Hits:\t\t\t%d\n", hits);
	printf("Read Hits:\t\t%d\n", readhits);
	printf("Write Hits:\t\t%d\n", writehits);
	printf("Cycles W/Cache:\t\t%d\n", cycleswithcache);
	printf("Cycles W/O Cache:\t%d\n", cycleswithoutcache);

	fclose(trace);

return 0;
}


void initilize(void)
{
	for(int i = 0; i < 1024; i ++){
		set[i].line[0].history = NULL;
		set[i].line[1].history = NULL;
		set[i].line[2].history = NULL;
		set[i].line[3].history = NULL;
	}
}

void read(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0){			 //match found
		hits++;					 //increment hits
		readhits++;                              //read hit so increment read hits
		resetlru(address, findMatch(address, 0));//reset the LRU based on this line having been accessed;
	}
	else{					//no match found, find someone to evict
		streamins++;
		misses++;
		addToCache(address, findLine(address, 0));
	}
}

void write(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0){			//match found
		hits++;					//increment hits
		writehits++;				//write hit so increment write hits
		resetlru(address, findMatch(address, 0));//reset the LRU based on this line having been accessed
	}
	else{					//no match found, find someone to evict
		streamins++;
		misses++;
		addToCache(address, findLine(address, 0));
	}
}

int findoldest(unsigned int addr)
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

	return linenum;
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
	if(lineI > 3)
		return -1;
	else if(set[GETS(address)].line[lineI].valid){
		if(set[GETS(address)].line[lineI].tag == GETT(address))
			return lineI;}

		return findMatch(address, (lineI + 1));
}

//recursively find available line
int findLine(unsigned int address, int lineI)
{

	if(lineI > 3){
		streamouts++;
		return findoldest(address);
	}
	else if(set[GETS(address)].line[lineI].valid)
		return findLine(address, (lineI + 1));	
	else{
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
