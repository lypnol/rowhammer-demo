#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


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

static uint64_t get_physical_addr(uint64_t virtual_addr) {
  static int g_pagemap_fd = -1;
  uint64_t value;
  
  // open the pagemap
  if(g_pagemap_fd == -1) {
      g_pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
  }
  if(g_pagemap_fd == -1) return 0;
  
  // read physical address
  off_t offset = (virtual_addr / 4096) * sizeof(value);
  int got = pread(g_pagemap_fd, &value, sizeof(value), offset);
  if(got != 8) return 0;

  // Check the "page present" flag.
  if(!(value & (1ULL << 63))) return 0;

  // return physical address
  uint64_t frame_num = value & ((1ULL << 55) - 1);
  return (frame_num * 4096) | (virtual_addr & (4095));
}


int main(int argc, char* argv[]) {
  // input parsing
  setvbuf(stdout, 0, _IONBF, 0);
  if (argc < 2)
    exit(fprintf(stderr, "ERROR: Usage: %s <gigabytes to allocate, e.g. 1-32>\n", argv[0]));
  char* end = 0;
  size_t memsizegb = strtoull(argv[1],&end,10);
  size_t memsize = 1024ULL * 1024ULL * 1024ULL * memsizegb;
  if (end == argv[1] || memsizegb == 0)
    exit(fprintf(stderr, "ERROR: Usage: %s <gigabytes to allocate, e.g. 1-32>\n", argv[0]));

  // reserve address space for array
  uint32_t* array = (uint32_t*) malloc(memsize);
  // uint32_t* array = (uint32_t*) mmap(, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  // alternatively you can try: uint32_t* array = (uint32_t*) mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (array == 0 || array == (uint32_t*)-1ULL)
    exit(fprintf(stderr, "ERROR: could not allocate enough memory\n"));
  printf("allocated at %p\n", (void*)get_physical_addr((uint64_t)array));
  // allocate memory for array by filling it with random but recognizable data
  size_t percent = 0;
  for (size_t i = 0; i < memsize/sizeof(uint32_t); ++i)
  {
    if ((100 * i) / (memsize/sizeof(uint32_t)) != percent)
      printf("\033[2K\rAllocating memory... %zu%%",percent = (100 * i) / (memsize/sizeof(uint32_t)));
    array[i] = 0xFFFFFFFFU * (rand() % 2);
  }
  printf("\033[2K\r");

  size_t flips = 0, tries = 0, replaceline = 0, offset = 0;
  size_t phys;
  char timestr[26];
  // hammer random addresses forever
  while (1)
  {
    for (size_t i = 0; i < 2 * memsizegb; ++i)
    {
      offset = rand() % (memsize/sizeof(uint32_t));
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
    PRINTF_REWIND_AND_RESET();
    printf("Checking...");
    for (size_t i = 0; i < memsize/sizeof(uint32_t); ++i)
    {
      if (array[i] != 0 && array[i] != 0xFFFFFFFFU)
      {
        uint32_t correct = 0xFFFFFFFFU * (__builtin_popcount(array[i]) / 16U);
        PRINTF_REWIND();
        gettimestr(timestr);
        phys = get_physical_addr((uint64_t)(array+i));
        printf("[!] %s Found %8zu. flip at %p Value %8x instead of %8x.\n",timestr,++flips,(void*)phys,array[i],correct);
        array[i] = correct;
      }
    }
  }
}

