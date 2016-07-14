#ifndef TIMER_CALCULATOR
#define TIMER_CALCULATOR

#define NUM_PACKETS 3


/*
 * Propose of this class is to calculate the time between packets on chanel 0, as each wixel
 * is measuring it.  
 * 
 * 
 * 
 */

struct PacketsGapCalculator {


	uint32 packets_gap;
	int error;


	uint32 last_0_packet;
	uint32 time_diffs[NUM_PACKETS];
    int packet_captured;




};

void Init(struct PacketsGapCalculator *pPacketsGapCalculator);

void PacketCaptured(struct PacketsGapCalculator * this, int channel);

uint32 GetInterpacketDelay(struct PacketsGapCalculator *this);

/*
 * When all is ok (and learning finished), yellow led is off
 * On error led is always on
 * when learning, blink the number of captured delayes
 */
void FlushLed(struct PacketsGapCalculator *this);

void FinalizeCalculations(struct PacketsGapCalculator *this);

extern XDATA struct PacketsGapCalculator g_PacketsGapCalculator;




#endif /* TIMER_CALCULATOR */