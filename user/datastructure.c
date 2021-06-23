#include "datastructure.h"
#include "user/user.h"

struct DN *mkqueue(void* data){
    struct DN *head = (struct DN *)malloc(sizeof(struct DN));
    head->data = data;
    head->next = head;
    head->prev = head;
    return head;
}

struct DN *pop_queue(struct DN *head){
    if(head->next == head){     //queue empty
        free(head);
        return NULL;
    }
    else{
        struct DN *prevhead = head;
        head = head->next;
        head->prev = prevhead->prev;
        free(prevhead);
        return head;
    }

}

void push_queue(struct DN *head, void *data){
    struct DN *newtail = (struct DN *)malloc(sizeof(struct DN));
    struct DN *prevtail = head->prev;
    prevtail->next = newtail;
    head->prev = newtail;
    newtail->next = head;
    newtail->prev = prevtail;
    newtail->data = data;
}

void delete_queue(struct DN *head){
    while(head = pop_queue(head));
}

struct SN *mkstack(void* data){
    struct SN *head = (struct SN *)malloc(sizeof(struct SN));
    head->data = data;
    head->next = NULL;
    return head;
}

struct SN *pop_stack(struct SN *head){
    if(head->next == NULL){     //queue empty
        free(head);
        return NULL;
    }
    else{
        struct SN *prevhead = head;
        head = head->next;
        free(prevhead);
        return head;
    }
}

struct SN *push_stack(struct SN *head, void *data){
    struct SN *newhead = (struct SN *)malloc(sizeof(struct SN));
    newhead->next = head;
    newhead->data = data;
    return newhead;
}

void delete_stack(struct SN *head){
    while(head = pop_stack(head));
}

