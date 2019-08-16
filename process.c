#include "3140_concur.h"
#include "utils.h"
#include "realtime.h"
#include <fsl_device_registers.h>


/* the currently running process. current_process must be NULL if no process is running,
    otherwise it must point to the process_t of the currently running process
*/
typedef struct process_state {
	unsigned int *sp; //Current stack pointer
	struct process_state *next; //Next process
	unsigned int *orig_sp; //Original Stack Pointer
	int n; //Size of the stack frame
 	int realtime; //0 is non-realtime, 1 is realtime
	realtime_t*start; 
	realtime_t*deadline; 
} process_t;

process_t * current_process = NULL; 
process_t * realtime_process_queue = NULL;
process_t * process_queue = NULL;
process_t * process_tail = NULL;
realtime_t current_time; //Time needs to be relative to process_start, so set it to 0 there. 
int process_deadline_met = 0; 
int process_deadline_miss = 0; 

/**Calculates and returns the time between the start and deadline relative to current time in milliseconds
Requires: a process_t pointer**/
int getTotalMsec(process_t* proc){
	return (proc->start -> sec*1000) + (proc-> start->msec) + (proc -> deadline -> sec*1000) + (proc -> deadline -> msec);
}

/** Checks whether enough time has passed for a process with start time start to begin. 
Returns 0 is the start time is after (larger) than the current time, and 1 if the start time is before or equal 
to the current time
Requires: both current_time and start to be greater or equal to 0**/
static int can_start(realtime_t * start){
	int current_total = (current_time.sec*1000) + current_time.msec; 
	int start_time = (start -> sec)*1000 + start->msec; 
	return start_time > current_total ? 0 : 1;
}
/**Finds the process with the earliest deadline that can start. 
Returns: a process_t pointer to the process_t with the earliest deadline that has a start time after current_time, 
or NULL if there is none. **/
static process_t * find_ready() {
	if (!realtime_process_queue) return NULL;
	process_t *proc = realtime_process_queue;
	process_t *prev = NULL;
	while(proc != NULL && !(can_start(proc -> start))){
		prev = proc; 
		proc = proc -> next; 
	}
	
	if(proc != NULL){//One is ready 
		//Handling the first thing, update realtime_process_queue 
		if(prev == NULL){
					realtime_process_queue = proc->next; 
				}
		else{
			//Don't update realtime_process_queue, update prev
					prev -> next = proc -> next; 		
				}
		proc->next = NULL;
	}
	return proc;
}
/**
Adds proc to the end of the non realtime process-queue
Requires: proc is a non real-time process_t**/
static void enqueue(process_t* proc){
	if (!process_queue) {
		process_queue = proc;
	}
	if (process_tail) {
		process_tail->next = proc;
	}
	process_tail = proc;
	proc->next = NULL;
	
}
/**
Removes and returns the first process, or null if the queue is empty, from non realtime process-queue
**/
static  process_t * dequeue(){
		if (!process_queue) return NULL;
	process_t *proc = process_queue;
	process_queue = proc->next;
	if (process_tail == proc) {
		process_tail = NULL;
	}
	proc->next = NULL;
	return proc;
}
/**Takes in a real-time process_t and adds it to be right before the process in the queue with a longer deadline
than proc. 
Requires: proc is a real-time process**/
static void rt_sort(process_t *proc) {
	if (!realtime_process_queue) {
		realtime_process_queue = proc;
			proc -> next = NULL; 
	}
	
	else{
		process_t* temp = realtime_process_queue; 
		process_t* prev = NULL;
		if(getTotalMsec(temp) > getTotalMsec(proc)){ //temp process lower priority than new process
			realtime_process_queue = proc; 
			proc->next = temp; 
		
		}
		else{
			while(temp->next != NULL && getTotalMsec(temp) < getTotalMsec(proc)){
				//iterate through
				prev = temp; 
				temp = temp -> next; 
			}
			if (temp->next == NULL){//The incoming process has the latest deadline
				temp -> next = proc; 
				proc -> next = NULL; 
			}
			else{//The incoming process needs to be in the middle of the queue
				prev -> next = proc; 
				proc -> next = temp; 
			
					}
				}
		
		}

	
}

static void process_free(process_t *proc) {
	process_stack_free(proc->orig_sp, proc->n);
	free(proc);
}



/* Called by the runtime system to select another process.
   "cursp" = the stack pointer for the currently running process
*/


unsigned int * process_select (unsigned int * cursp) {
	NVIC_EnableIRQ(PIT1_IRQn);
	if (cursp) {
		// Suspending a process which has not yet finished, save state and make it the tail
		current_process->sp = cursp;
		//Can't just push tail process, you need to know which queue it came from
		if (current_process -> realtime == 0){
			enqueue(current_process);
		}
		else{
			rt_sort(current_process);//This is a realtime process
		}
	} 
	else {
		// Check if a process was running, free its resources if one just finished
		if (current_process) {
			if(current_process -> realtime == 1){
				int process_time = getTotalMsec(current_process);
				int current_msec = (current_time.sec *1000) + current_time.msec; 
				if(process_time < current_msec) {
					process_deadline_miss += 1; 
				}
			else{
					process_deadline_met +=1; 
			}
		}
			process_free(current_process);
		}
	}
	if(realtime_process_queue)//Are there realtime processes
	{
		current_process = find_ready();
	
	if (current_process) {
		__enable_irq();
		return current_process->sp;
	}
	else {
		current_process = dequeue(); 
		if(current_process){
			__enable_irq();
			return current_process -> sp; 
		}
		else{
			while(current_process == NULL){
				current_process = find_ready();
			}
			return current_process -> sp; 
		}
	}
}
	
	else {
		// No process was selected, exit the scheduler
			current_process = dequeue(); 
		if(current_process){
			__enable_irq();
			return current_process -> sp; 
		}
		__enable_irq();
		return NULL;
	}
	__enable_irq();
		return NULL;
	
}




/* Starts up the concurrent execution */
void process_start (void) {
	current_time.sec = 0; 
	current_time.msec = 0; 
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
	PIT->MCR = 0;
	PIT->CHANNEL[0].LDVAL = DEFAULT_SYSTEM_CLOCK / 10;
	PIT->CHANNEL[1].LDVAL = DEFAULT_SYSTEM_CLOCK / 1000;//DEFAULT_SYSTEM_CLOCK = 1 second
	NVIC_EnableIRQ(PIT0_IRQn);
	NVIC_EnableIRQ(PIT1_IRQn);
	NVIC_SetPriority(PIT0_IRQn, 1);//Should be a lower priority                                                                                   BBBBBBBBBBBBBBBBBBBBBBBBthis to be a lower priority than the PIT1 timer.
	NVIC_SetPriority(SVCall_IRQn, 1);
	NVIC_SetPriority(PIT1_IRQn, 0);//Should set this to be a lower priority than the PIT1 timer.
	// Don't enable the timer yet. The scheduler will do so itself
	PIT->CHANNEL[1].TCTRL = (1<<1);
	PIT->CHANNEL[1].TCTRL |= 1;
	// Bail out fast if no processes were ever created
	if (!process_queue && !realtime_process_queue) return;
	//PIT->CHANNEL[1].TCTRL = 3;
	process_begin();
}
int process_rt_create(void (*f)(void), int n, realtime_t* start, realtime_t* deadline){
		unsigned int *sp = process_stack_init(f, n);
	if (!sp) return -1;
	
	process_t *proc = (process_t*) malloc(sizeof(process_t));
	if (!proc) {
		process_stack_free(sp, n);
		return -1;
	} 
	proc->sp = proc->orig_sp = sp;
	proc->n = n;
	proc->realtime = 1;
	proc -> start = start; 
	proc -> deadline = deadline; 
	rt_sort(proc);
	return 0;
}

/* Create a new process */
int process_create (void (*f)(void), int n) {
	unsigned int *sp = process_stack_init(f, n);
	if (!sp) return -1;
	
	process_t *proc = (process_t*) malloc(sizeof(process_t));
	if (!proc) {
		process_stack_free(sp, n);
		return -1;
	}
	proc->sp = proc->orig_sp = sp;
	proc->n = n;
	proc->realtime = 0; //Not in a realtime
	proc-> start = NULL;
	proc -> deadline = NULL;
	
	enqueue(proc);
	return 0;
}
/**Updates the current_time by one millisecond and resets the PIT1 timer to go off after another millisecond**/
void PIT1_IRQHandler(void){
	__disable_irq();
	current_time.msec += 1; 
	if(current_time.msec >= 1000){
		current_time.sec +=1; 
		current_time.msec = 0; 
	}
	PIT->CHANNEL[1].LDVAL = DEFAULT_SYSTEM_CLOCK / 1000;
	PIT -> CHANNEL[1].TFLG = 1; 
	__enable_irq();
}
