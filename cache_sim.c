/************************************************************************************
*Author: Daniel Diaz, Jelon Anderson
*Class: ECE486
*This program is a cache simulator, referece the FS for specs on input file.
*This program will read in an input file and output all important information
*at the end. Debug flags are optional but can be specified in the input file or
*passed in as arguments to the file.
*************************************************************************************/
#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

#define VERSION 0.5
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
void setFlags(char);
int setD(unsigned int, int);

//node to hold line history as it's added to the cache
struct memNode{
	unsigned int address;
	int tag;
	bool hit;
	bool read;
	int LRU;
	bool dirty;
	bool valid;
	struct memNode* next;
};

//function to print history in a particular line
void printHistory(struct memNode*);

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

// Define Global flags
struct set set[SET_SIZE];
bool version_flag = false;
bool trace_flag = false;
bool dump_flag = false;
bool g_read = false;		//this variable is a flag for the dump debug command
bool g_hit = false;		//this variable is a flag for the dump debug command

//define global variables for printout at the end of the program
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
int in = 0;

int main(int argc, char* argv[])
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
	
	initilize();					//initilize the memory history

	if(argc > 1){
		for(int i = 1; i < argc; i++){		//loop through arguments and set flags
			if(argv[i][0] == '-')		//testing to make sure input is a debug flag
				setFlags(argv[i][1]);}}	//call function to set flags
	
	//loop through the file
	while(1){
		//read a character from the trace file
		c = fgetc(trace);

		//test the character read from file
		if(c == 'r'){
			//get address from file
			fscanf(trace, "%x", &addr);
			reads++;		//increment reads
			accesses++;		//increment accesses
			g_read = true;
	
			//print input if the trace flag is present
			if(trace_flag){
				printf("r 0x%x ", addr);
				in++;
				if(in == 8){
					printf("\n");
					in = 0;}
			}

			//call read function
			read(addr);
		} else if(c == 'w'){
			//get address from file
			fscanf(trace, "%x", &addr);
			writes++;		//increment writes
			accesses++;		//increment accesses
			g_read = false;
		
			//print input if the trace flag is present
			if(trace_flag){
				printf("w 0x%x ", addr);
				in++;
				if(in == 8){
					printf("\n");
					in = 0;}
			}

			//call write function
			write(addr);
		} else if(c == '-'){
			setFlags(fgetc(trace));
		} else if(c == EOF)
			break;
	}
	
	//determine the cycles based on the hits and misses
	cycleswithcache = ((hits * 1) + (misses * 51));
	cycleswithoutcache = (accesses * 50);

	//print pertanent dump information
	if(dump_flag)
	{
		for(int i = 0; i < 1024; i ++)
		{
			if(set[i].line[0].history != NULL){
				printf("Line %d: ", i);
				printHistory(set[i].line[0].history);}
			if(set[i].line[1].history != NULL){
				printf("Line %d: ", i);
				printHistory(set[i].line[1].history);}
			if(set[i].line[2].history != NULL){
				printf("Line %d: ", i);
				printHistory(set[i].line[2].history);}
			if(set[i].line[3].history != NULL){
				printf("Line %d: ", i);
				printHistory(set[i].line[3].history);}
		}
	}

	//print version if flag is set
	if(version_flag)
		printf("Version: %.2f\n", VERSION);


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

//set all line history to NULL
void initilize(void)
{
	for(int i = 0; i < 1024; i ++){
		set[i].line[0].history = NULL;
		set[i].line[1].history = NULL;
		set[i].line[2].history = NULL;
		set[i].line[3].history = NULL;
	}
}

//function for reading an address
void read(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0){			 //match found
		hits++;					 //increment hits
		readhits++;                              //read hit so increment read hits
		g_hit = true;
		resetlru(address, findMatch(address, 0));//reset the LRU based on this line having been accessed;
	}
	else{					//no match found, find someone to evict
		streamins++;
		misses++;
		g_hit = false;
		addToCache(address, setD(address, findLine(address, 0)));
	}
}

//function to write an address
void write(unsigned int address)
{	
	//find a tag match
	if(findMatch(address, 0) >= 0){			//match found
		hits++;					//increment hits
		writehits++;				//write hit so increment write hits
		g_hit = true;
		set[GETS(address)].line[findMatch(address, 0)].dirty = true;	
		resetlru(address, findMatch(address, 0));//reset the LRU based on this line having been accessed
	}
	else{					//no match found, find someone to evict
		streamins++;
		misses++;
		g_hit = false;
		addToCache(address, setD(address, findLine(address, 0)));
	}
}

int setD(unsigned int addr, int index)
{
	set[GETS(addr)].line[index].dirty = false;
	return index;
}

//determine the oldest line in the LRU
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
		int old = findoldest(address);
		if(set[GETS(address)].line[old].dirty)
			streamouts++;
		return old;
	}
	else if(set[GETS(address)].line[lineI].valid)
		return findLine(address, (lineI + 1));	
	else{
		return lineI;}		
}

//set the debug flags
void setFlags(char flag)
{
	if(flag == 'd')
		dump_flag = true;
	if(flag == 't')
		trace_flag = true;
	if(flag == 'v')
		version_flag = true;
}

//add the address to the specified line
void addToCache(unsigned int addr, int lineI)
{	
	//add new node to line history	
	if(dump_flag){
	//allocate new node
	struct memNode * node = (struct memNode*) malloc(sizeof(struct memNode));
	
	//set address of node
	node -> address = addr;
	node -> next = set[GETS(addr)].line[lineI].history;	//point next node to history of line

	//set information for the dump debug
	node -> read = g_read;
	node -> hit = g_hit;
	node -> tag = GETT(addr);
	node -> LRU = 0;
	node -> dirty = false;
	node -> valid = true;
	set[GETS(addr)].line[lineI].history = node;		//line history to new nod
	}

	set[GETS(addr)].line[lineI].valid = true;		//set valid
	set[GETS(addr)].line[lineI].tag = GETT(addr);		//set tag
}

//print the history from a given line
void printHistory(struct memNode* head)
{
	if(head == NULL)
		return;

	printHistory(head -> next);

	printf("Address = 0x%x ", head-> address);
	printf("Tag = 0x%x ", head -> tag);
	printf("%s ", head -> hit ? "hit" : "miss");
	printf("%s ", head -> read? "read" : "write");
	printf("%s ", head -> dirty ? "dirty" : "clean");
	printf("%s ", head -> valid ? "valid" : "invlaid");
	printf("LRU: %d\n", head -> LRU);
}
