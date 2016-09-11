#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uart1.h>
#include "packets_gap_calculator.h"

static volatile BIT do_verbose = 0;

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

    this->last_good_packet = now;
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

//    if(do_verbose)
        printf("PacketsGapCalculator We have a valid packet to add to our db gap = %lu\r\n", gap);


    if (gap < 298000 || gap > 302000) {
        if(do_verbose)
            printf("PacketsGapCalculator wrong gap = %lu\r\n", gap);
        this->last_0_packet = now;
        return;
    }

    this->last_0_packet = now;
    if(this->packet_captured == NUM_PACKETS) {
        if(do_verbose)
            printf("PacketsGapCalculator already have enough packets\r\n");
        return;
    }
    
    this->time_diffs[this->packet_captured] = gap;
    this->packet_captured++;

    if(this->packet_captured == NUM_PACKETS) {
        FinalizeCalculations(this);
    }

}

void FinalizeCalculations(struct PacketsGapCalculator *this) {
    // We have enough packets, if all are the same time, or only one ms difference,
    // we have a good result.
    // WARNING!!! The 2 variables below must not use XDATA 
    uint32 min = this->time_diffs[0];
    uint32 max = this->time_diffs[0];
    XDATA uint32 i;

    for (i = 0; i < NUM_PACKETS; i++) {
        if(min > this->time_diffs[i]) {
            min = this->time_diffs[i];
        }
        if(max < this->time_diffs[i]) {
            max = this->time_diffs[i];
        }
    }
    printf("min = %lu max = %lu\r\n", min, max);
    if(min == max || (min+1) == max) {
        this->packets_gap = max;
        printf("We have a good average %lu\r\n", max);
        return;
    }
    printf("We have recieved some bad value, so we will start again :-( \r\n");
    Init(this);
    
}

int IsTooFar(XDATA struct PacketsGapCalculator *this, XDATA uint32 now) {

    if(now - this->last_good_packet > HOURS_BEFORE_FALLBACK_TO_CHANNEL_0 * 60 * 60 * 1000 ) {
        return 1;
    }
    
    return 0;
}

uint32 GetInterpacketDelay(XDATA struct PacketsGapCalculator *this, uint32 now) {

    if(IsTooFar(this, now)) {
        if(do_verbose)
            printf("PacketsGapCalculator too much time without a packet captured, need to return 0\r\n");
        return 0;
    }

    if(this->error) {
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
void FlushLed(XDATA struct PacketsGapCalculator *this, XDATA uint32 now) {
    uint32 now_cycled = (now & 0x7ff) / 4 ;

    if(this->error ) {
        LED_YELLOW(1);
        return;
    }
    
    if(IsTooFar(this, now)) {
        // Half a cycle will be on, half off
        LED_YELLOW(now_cycled < 0x7ff / 8);
        return;
    }

    if(this->packet_captured == NUM_PACKETS) {
        LED_YELLOW(0);
        return;
    }
    
    // Now we are counting... cycles of 2048 ms (now_cycled was devided by 4)
    if(now_cycled == 75) {
        LED_YELLOW(1);
        return;
    }
    if(this->packet_captured > 0 && now_cycled == 150){
        LED_YELLOW(1);
        return;
    }
    if(this->packet_captured > 1 && now_cycled == 225){
        LED_YELLOW(1);
        return;
    }
    if(this->packet_captured > 2 && now_cycled == 300){
        LED_YELLOW(1);
        return;
    }

    LED_YELLOW(0);
}

void PrintStatus(struct PacketsGapCalculator *this) {
    XDATA uint8 i;
    printf("    packets_gap %lu\r\n", this->packets_gap);
    printf("    error %hhu\r\n", this->error);
    printf("    last_0_packet %lu\r\n", this->last_0_packet);
    //printf("    packet_captured %hhu\r\n", this->packet_captured);
    printf("    last_good_packet %lu\r\n", this->last_good_packet);
    printf("    packets_gap %lu\r\n", this->packets_gap);
    for(i = 0; i < this->packet_captured; i++) {
        printf("    packets_diff %lu\r\n", this->time_diffs[i]);
    }
    

}

XDATA struct PacketsGapCalculator g_PacketsGapCalculator;