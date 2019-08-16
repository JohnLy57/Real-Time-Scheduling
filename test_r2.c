/*************************************************************************
 * Lab 5 Test file #2
 * 
 * pRT2: ^__g g g v
 * pRT1: ^________b b b v
 *
 *   You should see the sequence of processes depicted above:
 *     
 * 
 *   pRT2 should miss its deadline, and pRT1 should meet it
 * 
 ************************************************************************/
 
#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"

/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/


 
/* Stack space for processes */
#define NRT_STACK 60
#define RT_STACK  60
 

/*The purpose of this test case is to test deadline priorities amongst the realtime processes and preemptiveness. 
We have 2 realtime procsses. 
One arrives earlier but has a later deadline. The other arrives later but has an earlier deadline. We want to see if the 
second test takes over once it arrives. 
*/
/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/


/* Constants used for 'work' and 'deadline's. Will be using the 10 second timer for the first realtime process, and the 
5msec for the second realtime process*/
realtime_t t_1msec = {0, 5};
realtime_t t_10sec = {10, 0};

/* Using two different start times so we can test preemptiveness*/
realtime_t t_pRT1 = {1, 0};
realtime_t t_pRT2 = {3, 0};

 
/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}




/*-------------------
 * Real-time process
 *-------------------*/
//First realtime process. Should blink blue when run
void pRT1(void) {
	int i;
	for (i=0; i<3;i++){
	LEDBlue_On();
	mediumDelay();
	LEDBlue_Toggle();
	mediumDelay();
	}
}

//Second realtime process. Use a green LED so we can tell it apart from the first realtime process
void pRT2(void) {
	int i;
	for (i=0; i<3;i++){
	LEDGreen_On();
	mediumDelay();
	LEDGreen_Toggle();
	mediumDelay();
	}
}


/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
    //if (process_create(pNRT, NRT_STACK) < 0) { return -1; } //Not a realtime
    if (process_rt_create(pRT1, RT_STACK, &t_pRT1, &t_10sec) < 0) { return -1; } //Arrives first, but has a later deadline
		if (process_rt_create(pRT2, RT_STACK, &t_pRT2, &t_1msec) < 0) { return -1; } //Arrives second, but has an earlier deadline

		
   
    /* Launch concurrent execution */
	process_start();

  LED_Off();
		
		//Check if there were any missed processes. For this test case, there should be one
 while(process_deadline_miss>0) {
		LEDRed_On();
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_miss--;
	}
 
	//Check if there were any met procsses. For this test case, there should be one 
	while(process_deadline_met>0) {
		LEDGreen_On();
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_met--;
	}
	

	while (1){
	}
	return 0;
}
