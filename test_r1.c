/*************************************************************************
 * Lab 5 Test case 1
 * 
 * pNRT: ^r r r r v
 * pRT1: ^________b b b v
 *
 *   You should see the sequence of processes depicted above:
 *  
 * 
 *   pRT1 should meet its deadline, if you check in the debugger.
 * 
 ************************************************************************/
 
#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"

/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/


 
/* Stack space for processes */
#define NRT_STACK 80
#define RT_STACK  80
 

/*The purpose of this test case is to test our busy waiting and our process_met. 
Given how long the start time is for our realtime process, there will definitely be a busy wait. 
Also, by using the longer deadline, we expect to not have any missed processes

*/
/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/


/* Constants used for 'work' and 'deadline's. Will be using the 10 second timer*/
realtime_t t_1msec = {0, 1};
realtime_t t_10sec = {10, 0};

/* Process start time Is now 5 seconds to ensure that we have to busy wait*/
realtime_t t_pRT1 = {5, 0};

 
/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}



/*----------------------------------------------------
 * Non real-time process
 *   Blinks red LED 4 times. 
 *   Should be blocked by real-time process at first.
 *----------------------------------------------------*/
 
void pNRT(void) {
	int i;
	for (i=0; i<4;i++){
	LEDRed_On();
	shortDelay();
	LEDRed_Toggle();
	shortDelay();
	}
	
}

/*-------------------
 * Real-time process
 *-------------------*/

void pRT1(void) {
	int i;
	for (i=0; i<3;i++){
	LEDBlue_On();//Using blue LED so we can tell if it's different than the non realtime process
	mediumDelay();
	LEDBlue_Toggle();
	mediumDelay();
	}
}


/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
    if (process_create(pNRT, NRT_STACK) < 0) { return -1; }
    if (process_rt_create(pRT1, RT_STACK, &t_pRT1, &t_10sec) < 0) { return -1; } //Using the 10sec deadline to ensure the 
		//Process meets its deadline
   
    /* Launch concurrent execution */
	process_start();

  LED_Off();
		
		//Check if there were any missed processes. For this test case, there should be none as the deadline is so far away
 while(process_deadline_miss>0) {
		LEDRed_On();
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_miss--;
	}
 
	//Check if there were any met procsses. For this test case, there should be one, as there was only one process. 
	while(process_deadline_met>0) {
		LEDGreen_On();
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_met--;
	}
	
	/* Hang out in infinite loop (so we can inspect variables if we want) */ 
	while (1);
	return 0;
}
