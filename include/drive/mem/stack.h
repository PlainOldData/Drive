#ifndef STACK_INCLUDED_
#define STACK_INCLUDED_


/* ----------------------------------------------------------------- Types -- */


struct drvi_stack_marker;


struct drv_mem_stack {
        void *mem;
        unsigned long mem_size;
        struct drvi_stack_marker *top;
};


/* ------------------------------------------------------------- Interface -- */


void
drvm_stack_init(
        struct drv_mem_stack *st,
        void *mem,
        unsigned long bytes);


void*
drvm_stack_push(
        struct drv_mem_stack *st,
        unsigned long bytes,
        const char *label);


void*
drvm_stack_pop(
        struct drv_mem_stack *st);


void*
drvm_stack_pop_till(
        struct drv_mem_stack *st,
        void *target);


void*
drvm_stack_clear(
        struct drv_mem_stack *st);


/* -------------------------------------------------------------- Internal -- */


struct drvi_stack_marker {
        const char *label;
        struct drvi_stack_marker *prev;
        void *start, *end;
};


#endif


#ifdef STACK_IMPL
#ifndef STACK_IMPL_INCLUDED_
#define STACK_IMPL_INCLUDED_


#include <drive/mem.h>


void
drvm_stack_init(
        struct drv_mem_stack *st,
        void *mem,
        unsigned long bytes)
{
        st->mem = mem;
        st->mem_size = bytes;
        st->top = 0;
}


void*
drvm_stack_push(
        struct drv_mem_stack *st,
        unsigned long bytes,
        const char *label)
{
        unsigned char *start = (unsigned char*)st->mem;
        unsigned long free = st->mem_size;
        unsigned char *end = start;
        struct drvi_stack_marker *prev = st->top;
        
        if(st->top) {
                end = (unsigned char*)st->top->end;
                free -= end - start;
        }
        
        unsigned long needed = sizeof(struct drvi_stack_marker);
        needed += bytes;
        
        if(free < needed) {
                return 0;
        }
        
        struct drvi_stack_marker *next = (struct drvi_stack_marker *)end;
        next->start = end + sizeof(*next);
        next->end = next->start + bytes;
        next->label = label;
        next->prev = prev;
        
        st->top = next;
        
        return next->start;
}


void*
drvm_stack_pop(
        struct drv_mem_stack *st)
{
        if(!st->top) {
                return 0;
        }
        
        if(!st->top->prev) {
                st->top = 0;
                return 0;
        }
        
        st->top = st->top->prev;
        return st->top->start;
}


void*
drvm_stack_pop_till(
        struct drv_mem_stack *st,
        void *target)
{

}


void*
drvm_stack_clear(
        struct drv_mem_stack *st)
{

}


#endif
#endif
