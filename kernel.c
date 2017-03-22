#include "kernel.h"
#include <stdlib.h>
#include "lists.h"
#include <stdio.h>
#include <string.h>
#include "kernel_hwdep.h"

int KERNEL_MODE;
int ticks_counter;
TCB * Running = NULL;
mailbox *mb;
void idle(void);

#define idle_int 3452816845

exception init_kernel(){
	init_lists();
	KERNEL_MODE = INIT;
	ticks_counter = 0;
	create_task(&idle,idle_int);
	return OK;
}

exception create_task(void(*task_body)(), uint deadline){
	volatile int first_exec = TRUE;
	TCB * TCB_task;
	listobj * temp_obj;
	int isr_status = set_isr(ISR_OFF);
	TCB_task = (TCB *)malloc (sizeof(TCB));
	temp_obj = (listobj *)malloc(sizeof(listobj));
	set_isr(isr_status);
	if(TCB_task == NULL || temp_obj == NULL){
		free(TCB_task);
		free(temp_obj);
		set_isr(isr_status);
		return FAIL;
	}
	TCB_task->DeadLine = deadline;
	TCB_task->PC = task_body;
	TCB_task->SP = &(TCB_task->StackSeg[STACK_SIZE-1]);
	TCB_task->SPSR = 0;

	temp_obj->pTask = TCB_task;
	temp_obj->pMessage = NULL;
	temp_obj->pNext = NULL;
	temp_obj->pPrevious = NULL;

	if(KERNEL_MODE == INIT){
		ready_insert(temp_obj);
		Running = ready_list->pHead->pNext->pTask;
		return OK;
	}
	else {
		isr_off();
		SaveContext();
		if(first_exec) {
			first_exec = FALSE;
			ready_insert(temp_obj);
			Running = ready_list->pHead->pNext->pTask;
			LoadContext();
		}
	}
	return OK;		
}


void run(){
	KERNEL_MODE = RUNNING;
	//timer0_start();
	Running = ready_list->pHead->pNext->pTask;
	LoadContext();
}

void terminate() {
	if(ready_list->pHead->pNext->pTask->DeadLine != idle_int){ 
		int isr_status = set_isr(ISR_OFF);
		listobj * temp_obj = ready_extract();
		free(temp_obj->pTask);
		free(temp_obj);
		set_isr(isr_status);
		Running = ready_list->pHead->pNext->pTask;
	}
	LoadContext();
}


/*
 * COMMUNICATION FUCTIONS
 * mailbox, etc.
 */
mailbox *create_mailbox(uint nof_msg, uint size_of_msg) {
	mailbox * inbox; 
	int isr_status = set_isr(ISR_OFF);
	inbox = (mailbox *)malloc (sizeof(mailbox));
	set_isr(isr_status);
	if(inbox == NULL){
		return NULL;
	}
	isr_status = set_isr(ISR_OFF);
	inbox->pHead = (msg *)malloc (sizeof(msg));
	set_isr(isr_status);
	if(inbox->pHead == NULL) {
		free(inbox);
		return NULL;
	}
	isr_status = set_isr(ISR_OFF);
	inbox->pTail = (msg *)malloc (sizeof(msg));
	set_isr(isr_status);
	if(inbox->pTail == NULL) {
		free(inbox->pHead);
		free(inbox);
		return NULL;
	}

	inbox->pHead->pNext = inbox->pTail;
	inbox->pTail->pPrevious = inbox->pHead;

	inbox->nMaxMessages = nof_msg;
	inbox->nDataSize = size_of_msg;
	inbox->nBlockedMsg = 0;
	inbox->nMessages = 0;

	return inbox;
}

int no_messages(mailbox * mBox) {
	return mBox->nMessages;
}

exception remove_mailbox(mailbox* mBox){
	if(no_messages(mBox) == 0) {
		free(mBox->pHead);
		free(mBox->pTail);
		free(mBox);
		return OK;
	}
	else {
		return NOT_EMPTY;
	} 
}


exception send_wait(mailbox* mBox, void* Data){
	volatile int first_exec = TRUE;
	int isr_status;
	isr_off();		
	SaveContext();
	if(first_exec) {
		first_exec = FALSE;
		if(mBox->nBlockedMsg < 0 && mBox->pHead->pNext != mBox->pTail) {
			msg *recieved_msg;
			memcpy(mBox->pTail->pPrevious->pData, Data, mBox->nDataSize);
			recieved_msg = pop_msg(mBox);

			recieved_msg->pBlock->pMessage = NULL;

			wait_extract(recieved_msg->pBlock);
			ready_insert(recieved_msg->pBlock);
			mBox->nBlockedMsg++;

			Running = ready_list->pHead->pNext->pTask;
		}
		else {
			isr_status = set_isr(ISR_OFF);
			msg *temp_msg = (msg *)malloc (sizeof(msg));
			set_isr(isr_status);
			if(temp_msg == NULL) {
				free(temp_msg);
				return FAIL;
			}			
			temp_msg->pData = Data;
			temp_msg->pBlock = ready_list->pHead->pNext;
			ready_list->pHead->pNext->pMessage = temp_msg;
			temp_msg->Status = SENDER;
			push_msg(mBox, temp_msg);

			wait_insert(ready_extract());
			mBox->nBlockedMsg++;	
			Running = ready_list->pHead->pNext->pTask;
		}
		LoadContext(); 
	}
	else {
		if(ticks_counter >= Running->DeadLine){
			isr_off();
			pop_msg(mBox);	
			mBox->nBlockedMsg--;
			ready_list->pHead->pNext->pMessage = NULL;
			isr_on();
			return DEADLINE_REACHED;
		}
		else {
			return OK;
		}
	}
	return OK;
}


exception receive_wait(mailbox *mBox, void *Data){
	listobj * temp_obj ;
	msg *temp_msg ;
	volatile int first_exec = TRUE;
	isr_off();
	int isr_status = set_isr(ISR_OFF);
	temp_obj = (listobj *)malloc (sizeof(listobj));
	temp_msg = (msg *)malloc (sizeof(msg));
	set_isr(isr_status);
	if(temp_msg == NULL || temp_obj == NULL) {
		free(temp_obj);
		free(temp_msg);
		return FAIL;
	}
	SaveContext();
	if(first_exec){
		first_exec = FALSE;
		if(no_messages(mBox) !=0 && mBox->pHead->pNext != mBox->pTail){
			temp_msg = pop_msg(mBox);

			memcpy(Data, temp_msg->pData, mBox->nDataSize);

			if(mBox->nBlockedMsg != NULL) { 
				wait_extract(temp_msg->pBlock);
				ready_insert(temp_msg->pBlock);
				mBox->nBlockedMsg--;
				Running = ready_list->pHead->pNext->pTask;
			}
			else {
				free(temp_msg);
			}
		}
		else{
			temp_msg->pBlock = ready_list->pHead->pNext;
			temp_msg->pData = Data;
			temp_msg->Status = RECEIVER;
			push_msg(mBox,temp_msg);
			mBox->nBlockedMsg--;

			ready_list->pHead->pNext->pMessage = temp_msg;
			wait_insert(ready_extract());

			Running = ready_list->pHead->pNext->pTask;
		}
		LoadContext();
	}
	else {
		if(Running->DeadLine <= ticks_counter) {
			isr_off();
			ready_list->pHead->pNext->pMessage = NULL;
			isr_on();
			return DEADLINE_REACHED;
		}
		else {
			return OK;
		}
	}
	return OK;
}

exception send_no_wait(mailbox* mBox, void* pData){
	volatile int first_exec = TRUE;
	int isr_status;
	isr_off();
	SaveContext();
	if(first_exec) {
		first_exec = FALSE;
		if(mBox->nBlockedMsg < 0 && mBox->pHead->pNext != mBox->pTail) {
			memcpy(mBox->pTail->pPrevious->pData, pData, mBox->nDataSize);
			mBox->pTail->pPrevious->pBlock->pMessage = NULL;

			wait_extract(mBox->pTail->pPrevious->pBlock);
			ready_insert(mBox->pTail->pPrevious->pBlock);

			pop_msg(mBox);
			mBox->nBlockedMsg--;

			Running = ready_list->pHead->pNext->pTask;
			LoadContext();
		}
		else {
			isr_status = set_isr(ISR_OFF);
			msg* temp_msg = (msg *)malloc(sizeof(msg));
			temp_msg->pData = (char *)malloc(mBox->nDataSize);
			set_isr(isr_status);
			if(temp_msg == NULL || temp_msg->pData == NULL){
				free(temp_msg->pData);
				free(temp_msg);
				return FAIL;
			}	
			memcpy(temp_msg->pData, pData, mBox->nDataSize);
			temp_msg->pBlock = NULL;
			temp_msg->Status = SENDER;

			if(mBox->nMaxMessages <= mBox->nMessages) {
				msg* temp_removed_msg = pop_msg(mBox);
			}
			push_msg(mBox,temp_msg);
		}
	}
	return OK;
}


exception receive_no_wait(mailbox* mBox, void* pData){
	volatile bool status = FAIL;
	volatile int first_exec = TRUE;
	listobj * temp_obj;
	isr_off();
	SaveContext();
	if(first_exec) {
		first_exec = FALSE;
		if(no_messages(mBox) != 0 && mBox->pTail->pPrevious->Status == SENDER ){
			msg* sender = pop_msg(mBox);
			memcpy(pData,sender->pData,mBox->nDataSize);
			if(sender->pBlock != NULL && mBox->nBlockedMsg != 0) {
				temp_obj = sender->pBlock;
				temp_obj->pMessage = NULL;
				mBox->nBlockedMsg--;
				ready_insert(wait_extract(temp_obj));
				Running = ready_list->pHead->pNext->pTask;
				status = OK;
			}
			else {
				free(sender);
				status = OK;
			}
		}
		LoadContext();
	}
	return status;
}




exception wait(uint nTicks) {
	volatile int first_wait = TRUE;
	listobj * temp_obj = NULL;
	isr_off();
	SaveContext();
	if(first_wait) {
		first_wait = FALSE;
		temp_obj = ready_extract();
		temp_obj->nTCnt = nTicks+ticks_counter;
		timer_insert(temp_obj);
		Running = ready_list->pHead->pNext->pTask;
		LoadContext();
	}
	else {
		if (ticks_counter > Running->DeadLine) {
			return DEADLINE_REACHED;
		}
		else {
			return OK;
		}
	}
	return OK;
}


void set_ticks( uint no_of_ticks ){
	ticks_counter = no_of_ticks;
}
uint ticks( void ){
	return ticks_counter;
}
uint deadline( void ){
	return Running->DeadLine;
}
void set_deadline( uint nNew ){
	volatile int first_exec = TRUE;
	isr_off();
	SaveContext();
	if(first_exec){
		first_exec = FALSE;
		Running->DeadLine = nNew;
		ready_insert(ready_extract());
		Running = ready_list->pHead->pNext->pTask;
		LoadContext();
	}
}

exception push_msg(mailbox *mBox, msg* msg_sent){
	if(mBox == NULL || msg_sent == NULL) {
		return FAIL;
	}
	msg_sent->pNext = mBox->pHead->pNext;
	msg_sent->pPrevious = mBox->pHead;
	mBox->pHead->pNext->pPrevious = msg_sent;
	mBox->pHead->pNext = msg_sent;
	mBox->nMessages++;
	return OK;
}

msg* pop_msg(mailbox *mBox){
	msg* temp_msg = mBox->pTail->pPrevious;
	temp_msg->pPrevious->pNext = temp_msg->pNext;
	mBox->pTail->pPrevious = temp_msg->pPrevious;
	mBox->nMessages--;
	temp_msg->pNext = NULL;
	temp_msg->pPrevious = NULL;
	return temp_msg;
}

void idle(){
	while(1){
		SaveContext();   
		TimerInt();
		LoadContext();
	}
}

void TimerInt(void){
	listobj * extract_obj;
	listobj * temp_obj;
	ticks_counter++;
	temp_obj = wait_list->pHead->pNext;
	while(temp_obj != wait_list->pTail && temp_obj != NULL && temp_obj->pTask->DeadLine <= ticks_counter) {
		extract_obj = wait_extract(temp_obj);
		ready_insert(extract_obj);
		temp_obj = wait_list->pHead->pNext;
		Running = ready_list->pHead->pNext->pTask;
	}
	temp_obj = timer_list->pHead->pNext;
	while(temp_obj != timer_list->pTail && temp_obj != NULL && temp_obj->nTCnt <= ticks_counter) {
		ready_insert(timer_extract());
		temp_obj = timer_list->pHead->pNext;
		Running = ready_list->pHead->pNext->pTask;
	}
}