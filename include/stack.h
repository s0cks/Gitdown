#ifndef GITDOWN_STACK_H
#define GITDOWN_STACK_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif

struct gitdown_stack{
    void** item;
    size_t size;
    size_t asize;
};

typedef struct gitdown_stack gitdown_stack;

void
gitdown_stack_grow(gitdown_stack* stack, size_t nsize){
    if(stack->asize >= nsize){
        return;
    }

    stack->item = realloc(stack->item, nsize * sizeof(void*));
    memset(stack->item + stack->asize, 0x0, (nsize - stack->asize) * sizeof(void*));
    stack->asize = nsize;

    if(stack->size > nsize){
        stack->size = nsize;
    }
}

void
gitdown_stack_alloc(gitdown_stack* stack, size_t size){
    stack->item = NULL;
    stack->size = stack->asize = 0;

    if(!size){
        size = 8;
    }

    gitdown_stack_grow(stack, size);
}

void
gitdown_stack_free(gitdown_stack* stack){
    free(stack->item);
}

void
gitdown_stack_push(gitdown_stack* stack, void* item){
    if(stack->size >= stack->asize){
        gitdown_stack_grow(stack, stack->size * 2);
    }

    stack->item[stack->size++] = item;
}

void*
gitdown_stack_pop(gitdown_stack* stack){
    if(!stack->size){
        return NULL;
    }

    return stack->item[--stack->size];
}

void*
gitdown_stack_top(gitdown_stack* stack){
    if(!stack->size){
        return NULL;
    }

    return stack->item[stack->size - 1];
}

#ifdef __cplusplus
};
#endif

#endif