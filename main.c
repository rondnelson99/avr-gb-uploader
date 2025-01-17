#include "fetcher.h"
#include "buffer.h"
#include "sender.h"
#include <util/delay.h>
#include "lib/iopins.h"
#include <avr/interrupt.h> // Contains all interrupt vectors

void main()
{
	as_output(12);
	buffer_init();
	fetcher_init();
	sei();
	while (! fetcher_check_started()) {
		fetcher_start();
		_delay_ms(100);
	}
	sender_init();
	while (true) {
	}
}
