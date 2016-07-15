#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uart1.h>
#include "packets_gap_calculator.h"

static volatile BIT do_verbose = 1;

uint32 dist32(uint32 a, uint32 b)
{
    if(a > b) {
    	return a -b;
    }
    return  b -a;
}

void Init(struct PacketsGapCalculator *this) {
    memset(this, 0, sizeof (struct PacketsGapCalculator) );
}

uint32 FixGap(uint32 BigGap) {
	XDATA int count;
	XDATA uint32 gap;
	if(BigGap < 302000) {
		// Not intersting to print,
		return BigGap;
	}
	count = (BigGap + 150000) / 300000;
	if(count > 10) {
		// Packets are too far apart
		return 0;
	}
	gap = BigGap / count;
	if(do_verbose)
		printf("PacketsGapCalculator FixGap returning %lu for input %lu count = %d\r\n", gap, BigGap, count);
	return gap;
}


void PacketCaptured(struct PacketsGapCalculator * this, int channel, uint32 now) {
	XDATA uint32 gap;
	if(do_verbose)
		printf("PacketsGapCalculator called\r\n");

	if(this->packet_captured == NUM_PACKETS) {
		if(do_verbose)
			printf("PacketsGapCalculator already have enough packets\r\n");
		return;
	}
	
	if(channel != 0) {
		if(do_verbose)
			printf("PacketsGapCalculator called wrong channel\r\n");
		return;
	}

	if (this->last_0_packet == 0) {
		this->last_0_packet = now;
		if(do_verbose)
			printf("PacketsGapCalculator updating last_0_packet\r\n");
		return;
	}

	gap = FixGap(now - this->last_0_packet);

	if (gap < 298000 || gap > 302000) {
		if(do_verbose)
			printf("PacketsGapCalculator wrong gap = %lu\r\n", gap);
		this->last_0_packet = now;
		return;
	}

	if(do_verbose)
		printf("PacketsGapCalculator We have a valid packet to add to our db gap = %lu\r\n", gap);
	
	this->time_diffs[this->packet_captured] = gap;
	this->packet_captured++;
	this->last_0_packet = now;

	if(this->packet_captured == NUM_PACKETS) {
		FinalizeCalculations(this);
	}

}

void FinalizeCalculations(struct PacketsGapCalculator *this) {
	// We have enough data, let's calculate average...
	XDATA int i;
	this->packets_gap = 0;
	for (i = 0; i < NUM_PACKETS; i++) {
		this->packets_gap += this->time_diffs[i];
	}
	this->packets_gap = this->packets_gap / NUM_PACKETS;

	for (i = 0; i < NUM_PACKETS; i++) {
		if(dist32(this->packets_gap , this->time_diffs[i]) > 3 ) {
			if(do_verbose)
					printf("PacketsGapCalculator We have an error gap = %lu time_diff = %ly\r\n", this->packets_gap, this->time_diffs[i]);
			this->error = 1;
		}
	}
}

uint32 GetInterpacketDelay(struct PacketsGapCalculator *this) {
	if(this->error ) {
		if(do_verbose)
			printf("PacketsGapCalculator error detected returning 0\r\n");
		return 0;
	}
	if(this->packet_captured != NUM_PACKETS) {
		if(do_verbose)
			printf("PacketsGapCalculator not enough packets captured returning 0\r\n");
		return 0;
	}
	if(do_verbose)
		printf("PacketsGapCalculator returning %lu\r\n", this->packets_gap);
	return this->packets_gap;
}

/*
 * When all is ok (and learning finished), yellow led is off
 * On error led is always on
 * when learning, blink the number of captured delayes
 */
void FlushLed(struct PacketsGapCalculator *this) {
	
	if(this->error ) {
		LED_YELLOW(1);
		return;
	}
	if(this->packet_captured == NUM_PACKETS) {
		LED_YELLOW(0);
		return;
	}
	// Now we are counting... cycles of 2048 ms 
	{
		XDATA long now = (getMs() & 0x7ff) / 4 ;
		if(now == 75) {
			LED_YELLOW(1);
			return;
		}
		if(this->packet_captured > 0 && now == 150){
			LED_YELLOW(1);
			return;
		}
		if(this->packet_captured > 1 && now == 225){
			LED_YELLOW(1);
			return;
		}
		if(this->packet_captured > 2 && now == 300){
			LED_YELLOW(1);
			return;
		}

		LED_YELLOW(0);
	}
}



XDATA struct PacketsGapCalculator g_PacketsGapCalculator;