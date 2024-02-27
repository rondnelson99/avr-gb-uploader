#include <stdint.h>
#include <stdbool.h>

void fetcher_start();

bool fetcher_check_started();

// tells the fetcher that the buffer has a new free block, useful for if the buffer is full
void fetcher_buffer_ready();