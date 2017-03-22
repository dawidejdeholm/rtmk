#ifndef LISTS_H
#define LISTS_H

exception init_lists();
void timer_insert(listobj *pnewObj);
void ready_insert(listobj * new_obj);
void wait_insert(listobj * new_obj);
listobj * timer_extract();
listobj * ready_extract();
listobj * create_listobj(int num);

listobj * wait_extract(listobj * pBlock);
 

list * create_list();
listobj * create_listobj(int num);
extern list * timer_list;
extern list * wait_list;
extern list * ready_list;

#endif
