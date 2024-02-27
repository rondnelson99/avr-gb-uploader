#include <stdint.h>
#include <stdbool.h>

void buffer_init();

void buffer_push_byte(uint8_t byte);

void buffer_restart_push_block();

bool buffer_push_block_advance();

uint8_t buffer_read_byte();

void buffer_restart_read_block();

bool buffer_read_block_advance();
