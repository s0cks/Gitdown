#ifndef LIFESTYLE_BUFFER_H
#define LIFESTYLE_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"{
#endif

#define __attribute__(x)

struct gitdown_buffer {
    uint8_t *data;

    size_t size;
    size_t asize;
};

typedef struct gitdown_buffer gitdown_buffer;

gitdown_buffer*
gitdown_buffer_alloc(size_t size) __attribute__((malloc)){
    gitdown_buffer* ret = malloc(sizeof(gitdown_buffer));

    if(!ret){
        fprintf(stderr, "[Gitdown] Allocation failed.\n");
        abort();
    }

    ret->data = NULL;
    ret->size = ret->asize = 0;
    return ret;
}

void
gitdown_buffer_free(gitdown_buffer *buf){
    if(!buf){
        return;
    }

    free(buf->data);
    free(buf);
}

void
gitdown_buffer_grow(gitdown_buffer *buf, size_t size){
    if(buf->asize >= size){
        return;
    }

    size_t nasize = buf->asize + 1024;
    while(nasize < size){
        nasize += 1024;
    }

    uint8_t* ret = realloc(buf->data, nasize);
    if(!ret){
        fprintf(stderr, "[Gitdown] Allocation failed.\n");
        abort();
    }

    buf->data = ret;
    buf->asize = nasize;
}

void
gitdown_buffer_put(gitdown_buffer *ob, const uint8_t *data, size_t size){
    if(ob->size + size > ob->asize){
        gitdown_buffer_grow(ob, ob->size + size);
    }

    memcpy(ob->data + ob->size, data, size);
    ob->size += size;
}

void
gitdown_buffer_puts(gitdown_buffer *ob, const char *str){
    gitdown_buffer_put(ob, (const uint8_t*) str, strlen(str));
}

void
gitdown_buffer_putc(gitdown_buffer *ob, const uint8_t c){
    if(ob->size >= ob->asize){
        gitdown_buffer_grow(ob, ob->size + 1);
    }

    ob->data[ob->size] = c;
    ob->size++;
}

void
gitdown_buffer_putc_rep(gitdown_buffer* ob, const uint8_t c, size_t times){
    for(size_t i = 0; i < times; i++){
        gitdown_buffer_putc(ob, c);
    }
}

int
gitdown_buffer_putf(gitdown_buffer *ob, const FILE *file){
    while(!(feof((FILE *) file) || ferror((FILE *) file))){
        gitdown_buffer_grow(ob, ob->size + 1024);
        ob->size += fread(ob->data + ob->size, 1, 1024, (FILE *) file);
    }

    return ferror((FILE *) file);
}

void
gitdown_buffer_printf(gitdown_buffer *ob, const char *fmt, ...) __attribute__((format (printf, 2, 3))){
    va_list ap;
    int n;

    if(ob->size >= ob->asize){
        gitdown_buffer_grow(ob, ob->size + 1);
    }

    va_start(ap, fmt);
    n = vsnprintf((char*) ob->data + ob->size, ob->asize - ob->size, fmt, ap);
    va_end(ap);

    if(n < 0){
        return;
    }

    if((size_t) n >= ob->asize - ob->size){
        gitdown_buffer_grow(ob, ob->size + n + 1);
        va_start(ap, fmt);
        n = vsnprintf((char*) ob->data + ob->size, ob->asize - ob->size, fmt, ap);
        va_end(ap);
    }

    if(n < 0){
        return;
    }

    ob->size += n;
}

void
gitdown_buffer_slurp(gitdown_buffer *ob, size_t size){
    if(size >= ob->size){
        ob->size = 0;
        return;
    }

    ob->size -= size;
    memmove(ob->data, ob->data + size, ob->size);
}

uint8_t
gitdown_buffer_getc(gitdown_buffer* ib, size_t index){
    if(index >= ib->size || index < 0){
        return 0;
    }

    return ib->data[index];
}

gitdown_buffer*
gitdown_buffer_slice(gitdown_buffer* ib, size_t offset, size_t len){
    gitdown_buffer* ob = gitdown_buffer_alloc(len);
    for(size_t i = 0; i < len; i++){
        gitdown_buffer_putc(ob, ib->data[offset + i]);
    }
    return ob;
}

const char*
gitdown_buffer_cstr(gitdown_buffer *buf){
    if(buf->size < buf->asize && buf->data[buf->size] == 0){
        return (char*) buf->data;
    }

    gitdown_buffer_grow(buf, buf->size + 1);
    buf->data[buf->size] = 0;
    return (char*) buf->data;
}

#ifdef __cplusplus
}
#endif

#endif