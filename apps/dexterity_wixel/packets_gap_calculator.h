#ifndef TIMER_CALCULATOR
#define TIMER_CALCULATOR

#define NUM_PACKETS 5
#define HOURS_BEFORE_FALLBACK_TO_CHANNEL_0 8ul


/*
 * Propose of this class is to calculate the time between packets on chanel 0, as each wixel
 * is measuring it.  
 * 
 * 
 * 
 */

struct PacketsGapCalculator {

    uint32 packets_gap;
    uint8 error;

    uint32 last_0_packet;
    uint8 packet_captured;
    uint32 time_diffs[NUM_PACKETS];
    

    uint32 last_good_packet;

};

void Init(struct PacketsGapCalculator *pPacketsGapCalculator);

void PacketCaptured(struct PacketsGapCalculator * this, int channel, uint32 now);

uint32 GetInterpacketDelay(XDATA struct PacketsGapCalculator *this, uint32 now);

/*
 * When all is ok (and learning finished), yellow led is off
 * On error led is always on
 * when learning, blink the number of captured delayes
 */
void FlushLed(XDATA struct PacketsGapCalculator *this, XDATA uint32 now);

void FinalizeCalculations(struct PacketsGapCalculator *this);

void PrintStatus(struct PacketsGapCalculator *this);

extern XDATA struct PacketsGapCalculator g_PacketsGapCalculator;




#endif /* TIMER_CALCULATOR */