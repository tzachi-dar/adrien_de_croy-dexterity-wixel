#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uart1.h>
#include "packets_gap_calculator.h"

extern volatile BIT do_verbose;

uint32 dist32(uint32 a, uint32 b)
{
    if(a > b) {
    	return a -b;
    }
    return  b -a;
}



XDATA struct PacketsGapCalculator PacketsGapCalculator11;

void Init(struct PacketsGapCalculator *pPacketsGapCalculator) {
    memset(&PacketsGapCalculator11, 0, sizeof (struct PacketsGapCalculator) );
//    pPacketsGapCalculator->error = 0;
//    pPacketsGapCalculator->packet_captured = 0;
}

void PacketCaptured(struct PacketsGapCalculator * this, int channel) {
	XDATA uint32 gap;
	XDATA uint32 now = getMs();
	if(do_verbose)
		printf("PacketsGapCalculator called\r\n");

	if(PacketsGapCalculator11.packet_captured == NUM_PACKETS) {
		if(do_verbose)
			printf("PacketsGapCalculator already have enough packets\r\n");
		return;
	}
	
	if(channel != 0) {
		if(do_verbose)
			printf("PacketsGapCalculator called wrong channel\r\n");
		return;
	}

	if (PacketsGapCalculator11.last_0_packet == 0) {
		PacketsGapCalculator11.last_0_packet = now;
		if(do_verbose)
			printf("PacketsGapCalculator updating last_0_packet\r\n");
		return;
	}

	gap = now - PacketsGapCalculator11.last_0_packet;

	if (gap < 298000 || gap > 302000) {
		if(do_verbose)
			printf("PacketsGapCalculator wrong gap = %lu\r\n", gap);
		PacketsGapCalculator11.last_0_packet = now;
		return;
	}

	if(do_verbose)
		printf("PacketsGapCalculator We have a valid packet to add to our db gap = %lu\r\n", gap);
	
	PacketsGapCalculator11.time_diffs[PacketsGapCalculator11.packet_captured] = gap;
	PacketsGapCalculator11.packet_captured++;
	PacketsGapCalculator11.last_0_packet = now;

	if(PacketsGapCalculator11.packet_captured == NUM_PACKETS) {
		FinalizeCalculations(&PacketsGapCalculator11);
	}

}

void FinalizeCalculations(struct PacketsGapCalculator *this) {
	// We have enough data, let's calculate average...
	XDATA int i;
	PacketsGapCalculator11.packets_gap = 0;
	for (i = 0; i < NUM_PACKETS; i++) {
		PacketsGapCalculator11.packets_gap += PacketsGapCalculator11.time_diffs[i];
	}
	PacketsGapCalculator11.packets_gap = PacketsGapCalculator11.packets_gap / NUM_PACKETS;

	for (i = 0; i < NUM_PACKETS; i++) {
		if(dist32(PacketsGapCalculator11.packets_gap , PacketsGapCalculator11.time_diffs[i]) > 50 ) {
			if(do_verbose)
					printf("PacketsGapCalculator We have an error gap = %lu time_diff = %ly\r\n", PacketsGapCalculator11.packets_gap, PacketsGapCalculator11.time_diffs[i]);
			PacketsGapCalculator11.error = 1;
		}
	}
}

uint32 GetInterpacketDelay(struct PacketsGapCalculator *this) {
	if(PacketsGapCalculator11.error ) {
		if(do_verbose)
			printf("PacketsGapCalculator error detected returning 0\r\n");
		return 0;
	}
	if(PacketsGapCalculator11.packet_captured != NUM_PACKETS) {
		if(do_verbose)
			printf("PacketsGapCalculator not enough packets captured returning 0\r\n");
		return 0;
	}
	if(do_verbose)
		printf("PacketsGapCalculator returning %lu\r\n", PacketsGapCalculator11.packets_gap);
	return PacketsGapCalculator11.packets_gap;
}

/*
 * When all is ok (and learning finished), yellow led is off
 * On error led is always on
 * when learning, blink the number of captured delayes
 */
void FlushLed(struct PacketsGapCalculator *this) {
	
	if(PacketsGapCalculator11.error ) {
		LED_YELLOW(1);
		return;
	}
	if(PacketsGapCalculator11.packet_captured == NUM_PACKETS) {
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
		if(PacketsGapCalculator11.packet_captured > 1 && now == 150){
			LED_YELLOW(1);
			return;
		}
		if(PacketsGapCalculator11.packet_captured > 2 && now == 225){
			LED_YELLOW(1);
			return;
		}
		LED_YELLOW(0);
	}
}



XDATA struct PacketsGapCalculator g_PacketsGapCalculator;