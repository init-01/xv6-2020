#ifndef __DATASTRUCTURE__
#define __DATASTRUCTURE__

#define NULL 0

struct SN{
    struct SN *next;
    void* data;
};
struct DN{
    struct DN *next;
    struct DN *prev;
    void* data;
};

struct DN *mkqueue(void* data);
struct DN *pop_queue(struct DN *head);
void push_queue(struct DN *head, void *data);
void delete_queue(struct DN *head);

struct SN *mkstack(void* data);
struct SN *pop_stack(struct SN *head);
struct SN *push_stack(struct SN *head, void *data);
void delete_stack(struct SN *head);

#endif