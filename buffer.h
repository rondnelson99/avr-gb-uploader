#include <stdint.h>
#include <stdbool.h>

void buffer_init();

bool buffer_push_ready();

void buffer_push_byte(uint8_t byte);

void buffer_restart_push_block();

bool buffer_push_block_advance();

bool buffer_read_ready();

uint8_t buffer_read_byte();

void buffer_restart_read_block();

bool buffer_read_block_advance();
