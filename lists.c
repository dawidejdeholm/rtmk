#include <stdlib.h>
#include "kernel.h"
#include "lists.h"

list * timer_list = NULL;
list * wait_list = NULL;
list * ready_list = NULL;
list * temp_list = NULL;


exception init_lists() {
 	timer_list = create_list();
	wait_list = create_list();
	ready_list = create_list();
	return SUCCESS;
}


/*
* TIMER LIST
*/

/*
* TIMER LIST: INSERTION
* The insertion of an element into the list, pass an integer with the function call. Insertion in the list is
* done so that the element with the lowest value of nTCnt appears first.  
*/
void timer_insert(listobj *newObj){
  listobj *pTemp = timer_list->pHead;
  while(pTemp->pNext != timer_list->pTail){
    if(pTemp->pNext->nTCnt >= newObj->nTCnt){
      break;
    }
    pTemp = pTemp->pNext;
  }
  newObj->pNext = pTemp->pNext;
  newObj->pPrevious = pTemp;
  pTemp->pNext = newObj;
  newObj->pNext->pPrevious = newObj;
}


/*
* TIMER LIST: EXTRACTION
* Extraction is always done from the front, i.e. at the "head-element".   
*/
listobj * timer_extract(){
listobj *extObj ;
  if(timer_list->pHead->pNext == timer_list->pTail)return NULL;
  extObj = timer_list->pHead->pNext;
  timer_list->pHead->pNext = extObj->pNext;
  return extObj;
}


/*
* WAITING LIST
*/

/*
* WAITING LIST: INSERTION
* A list, where the list is sorted so that the element with the lowest value 
* of TCB-> Deadline is  first. 
*/

void wait_insert(listobj *new_obj){
  listobj *newObj = new_obj;
  listobj *pTemp = wait_list->pHead;
  while(pTemp->pNext != wait_list->pTail){
    if(pTemp->pNext->pTask->DeadLine >= newObj->pTask->DeadLine){
      break;
    }
    pTemp = pTemp->pNext;
  }
  newObj->pNext = pTemp->pNext;
  newObj->pPrevious = pTemp;
  pTemp->pNext = newObj;
  newObj->pNext->pPrevious = newObj;
}


/*
* WAITING LIST: EXTRACTION
* Extraction made by the use of a pointer to the list element, struct l_obj * pBlock 
*/

listobj * wait_extract(listobj * pBlock){
  if (wait_list->pHead->pNext != wait_list->pTail){
	pBlock->pNext->pPrevious = pBlock->pPrevious;
        pBlock->pPrevious->pNext = pBlock->pNext;
        pBlock->pNext = NULL;
	pBlock->pPrevious = NULL;
  	return pBlock;
  }
  return FAIL;
}


/*
* READY LIST
*/

/*
* READY LIST: INSERTION
* The element with the lowest value of TCB-> Deadline is first placed first in the list.  
*/

void ready_insert(listobj *new_obj){
  listobj *newObj = new_obj;
  listobj *pTemp = ready_list->pHead;
  while(pTemp->pNext != ready_list->pTail){
    if(pTemp->pNext->pTask->DeadLine >= newObj->pTask->DeadLine){
      break;
    }
    pTemp = pTemp->pNext;
  }
  newObj->pNext = pTemp->pNext;
  newObj->pPrevious = pTemp;
  pTemp->pNext = newObj;
  newObj->pNext->pPrevious = newObj;
}

/*
* READY LIST: EXTRACTION
* Extraction is always done from the front, i.e. at the "head-element".   
*/

listobj * ready_extract(){
listobj *extObj ;
  if(ready_list->pHead->pNext == ready_list->pTail)return NULL;
  extObj = ready_list->pHead->pNext;
  ready_list->pHead->pNext = extObj->pNext;
  return extObj;
}


/*
* CREATES LIST
*
*/

list * create_list(){
	temp_list = (list *)malloc (sizeof(list));
	if(temp_list == NULL){
		return NULL;
	}
	temp_list->pHead = (listobj *)malloc (sizeof(listobj));
	if(temp_list->pHead == NULL) {
	  free(temp_list);
	  return NULL;
	}
	temp_list->pTail = (listobj *)malloc (sizeof(listobj));
	if(temp_list->pTail == NULL) {
	  free(temp_list->pHead);
	  free(temp_list);
	  return NULL;
	}
	temp_list->pHead->pPrevious = NULL;
	temp_list->pHead->pNext = temp_list->pTail;
	temp_list->pTail->pPrevious = temp_list->pHead;
	temp_list->pTail->pNext = NULL;
	
	return temp_list;
}


listobj * create_listobj(int num)
{
	listobj * myobj = (listobj *)calloc(1, sizeof(listobj));
	TCB *  mytcb = malloc(sizeof (TCB));
	if (myobj == NULL || mytcb == NULL)
	{
	  	free(myobj);
		free(mytcb);
		return NULL;
	}
	myobj->nTCnt = num;
	mytcb->DeadLine = num;
	myobj->pTask = mytcb;
	
	return (myobj);
}
