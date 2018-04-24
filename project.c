#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// Operating Systems Final Project
// Dalibor Loncarevic, Michel Piljic
// Project for 2 people
// April 5th, 2018

#define PAGES 256
#define PHY_MEMORY 65536
#define TLB_SIZE 16

// structure used to hold TLB entries
struct index {

	int page;
	int pageTable;
	//used to check when it was last used
	int timer;
};

// function prototypes
int tlb_check (uint8_t);
void add_tlb(uint8_t, int);

//variable declarations
struct index TLB[TLB_SIZE];
int tlb_hit;

//start of program
int main(int argc, char* argv[]) {

//did not enter an argument to open the file
	if (argc != 2) {
		printf("Please enter a file\n");
		return 0;
	
	}

	// variables
	int pageTable[PAGES];
	uint8_t memory[PHY_MEMORY];
	uint8_t* back;
	int i, input_num;
	char inputer[200];
	int addresses = 0;
	uint16_t virtual;
	uint8_t offset, page;
	int frame,page_faults = 0, free_page = 0;
	int physical;
	int8_t value;

	tlb_hit = 0;

	//open both backing store and the argumen file for reading
	int backingstore = open("BACKING_STORE.bin", O_RDONLY);
	FILE *inputs = fopen (argv[1],"r");

	//use memory mapping to allow for seamless access to simulate pageing
	back = mmap(0, PHY_MEMORY, PROT_READ, MAP_PRIVATE, backingstore, 0);

	//initalize tlb empty
	for (i = 0; i < TLB_SIZE; i++) {

		TLB[i].page = -1;
		TLB[i].pageTable = -1;
		TLB[i].timer = 0;	

	}

	// initialize page table empty
	for(i = 0; i < PAGES; i++) {

		pageTable[i] = -1;
	}

	// while still reading addresses from file
	while (fgets(inputer,sizeof(inputer), inputs) != NULL) {

	// obtaining data from address
	input_num = atoi(inputer);
	addresses++;
	virtual = input_num & 0xFFFF;
	offset = input_num & 0xFF; 
	page = ((virtual & 0xFF00)>>8);

	//check tlb
	frame = tlb_check(page);
	
	//check page table
	if (frame == -1) {

		frame = pageTable[page];
		// if it found the entry in page table add to tlb
		if (frame != -1) {
			add_tlb(page,pageTable[page]);
		}
	}

	//add to memory if both checks fail
	if (frame == -1) {

		//page fault counter
		page_faults++;
		frame = free_page;
		free_page++;

		//copying the value from backing store into memory
		memcpy(memory + frame*PAGES,back + page*PAGES,PAGES);
		pageTable[page] = frame;
		// adding to tlb
		add_tlb(page,pageTable[page]);
	
	} 
	// obtaining infromation from frame
	physical = frame << 8 | offset;
	value = memory[frame * PAGES + offset];

	//printing the informaton of this address
	printf("Virtual address: %d Physical Address: %d Value: %d\n",virtual, physical, value);
}
	//printing the statistics
	printf("Number of Translated Addresses = %d\n", addresses);
	printf("Page Faults = %d\n", page_faults);
	printf("Page Fault rate = %.3f\n", (float)page_faults/addresses);
	printf("TLB Hits = %d\n", tlb_hit);
	printf("TLB Hit rate = %.3f\n", (float)tlb_hit/addresses);
	//closing all files and mmap
	munmap(back, PHY_MEMORY);
	close (backingstore);
	fclose (inputs);
	return 0;
// end of program
}

//checks if entry is in tlb, increments each timer by 1 if not found, if found returns the frame, otherwise -1
int tlb_check (uint8_t reference) {

	int answer = -1;
	for (int i=0; i<TLB_SIZE; i++) {

		if (reference == TLB[i].page) {

			tlb_hit++;
			answer = TLB[i].pageTable;
			//reset timer
			TLB[i].timer = 0;
		}
		else {

			TLB[i].timer++;
		}


	}

	return answer;
}

//adds the entry into the tlb based on the timer, entry with largest timer gets replaced
void add_tlb (uint8_t page, int pageTable) {

	int max = 0;
	// fill empty places in TLB
	for (int i = 0; i < TLB_SIZE; i++) {

		if (TLB[i].page == -1) {
			
			TLB[i].page = page;
			TLB[i].pageTable = pageTable;
			TLB[i].timer = 0;
			return;
		}

	}
	// find largest time
	for (int i = 0; i < TLB_SIZE; i++) {

		if (TLB[i].timer > max) {

			max = TLB[i].timer;
		}
		
	}

	// find tlb with largest timer and replace it
	for (int i = 0; i < TLB_SIZE; i++) {

		if (max == TLB[i].timer) {

			TLB[i].page = page;
			TLB[i].pageTable = pageTable;
			TLB[i].timer = 0;
			return;
		}
		
	}

}
