/*
* TEST APPLICATION FOR MICROKERNEL
*/
mailbox *mb;
void task1(void);
void task2(void);
void task3(void);
void task4(void);
int button = 0x60;
int warm_water = 0;
int fill_cup = 0;

int main(void){

	if (init_kernel() != OK ) {
		while(1); }
	if (create_task( task1, 2000 ) != OK ) {
		while(1); }
	if (create_task( task2, 4000 ) != OK ) {
		while(1); }
	if (create_task( task3, 5000 ) != OK ) {
		while(1); }
	if (create_task( task4, 6000 ) != OK ) {
		while(1); }
	if ( (mb=create_mailbox(2,sizeof(int))
			) == NULL) {
		while(1); }
	run(); 
	return 0;
}

void task1(void)
{
	if(button == 0x60) {		//
		button = 1;
		send_no_wait(mb,&button);
		terminate();
	}
}
void task2(void){
	button = 0;
	receive_wait(mb,&button);		
	if(button == 1 && warm_water == 0) { 	
		wait(10);		    				
		warm_water = 0x2;              	
		send_no_wait(mb,&warm_water); 
		warm_water = 0xBEEF;
		terminate();		
	}
}
void task3(void){
	if(warm_water == 0xBEEF) {		
		receive_wait(mb,&warm_water);
		if(warm_water == 0x2) {	
			fill_cup = 0x1;			
			send_no_wait(mb,&fill_cup);
			fill_cup = 0xD34D;
			terminate();		
		}
	}
	else {
		wait(15);		
	}
}
void task4(void) {
	if(fill_cup == 0xD34D){
		receive_wait(mb,&fill_cup);
		if(fill_cup == 0x1) {		
			terminate();		
		}
	}
	else {
		wait(30);	
	}
}