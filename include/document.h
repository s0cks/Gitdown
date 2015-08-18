#ifndef GITDOWN_DOCUMENT_H
#define GITDOWN_DOCUMENT_H

#include <stdint.h>
#include <ctype.h>

#include "buffer.h"
#include "stack.h"

#ifdef __cplusplus
export "C" {
#endif

struct link_ref {
    uint8_t id;
    gitdown_buffer *link;
    gitdown_buffer *title;

    struct link_ref *next;
};

struct gitdown_document_renderer {
    void (*doc_header)(gitdown_buffer* ob);
    void (*doc_footer)(gitdown_buffer* ob);

    void (*header)(gitdown_buffer *ob, const gitdown_buffer* content);
    void (*hrule)(gitdown_buffer *ob);
    void (*user_link)(gitdown_buffer* ob, const gitdown_buffer* content);
    void (*issue_link)(gitdown_buffer* ob, const gitdown_buffer* content);
    void (*line_break)(gitdown_buffer* ob);
    void (*unordered_list)(gitdown_buffer* ob, const gitdown_buffer* list_item);
    void (*ordered_list)(gitdown_buffer* ob, const gitdown_buffer* list_item, int index);
    void (*free_line)(gitdown_buffer* ob, const gitdown_buffer* line);
};

struct gitdown_document_parser{
    size_t pos;
    gitdown_buffer* data;
};

struct gitdown_document {
    struct link_ref* link_refs[8];
    struct gitdown_document_renderer renderer;
    struct gitdown_document_parser parser;
};

typedef struct gitdown_document gitdown_document;
typedef struct gitdown_document_renderer gitdown_document_renderer;
typedef struct gitdown_document_parser gitdown_document_parser;
typedef struct link_ref link_ref;

static uint8_t
gitdown_hash_link_ref(const uint8_t *link_ref, size_t len) {
    uint8_t hash = 0;

    for (size_t i = 0; i < len; i++) {
        hash = (uint8_t) (tolower(link_ref[i]) + (hash << 6) + (hash << 16) - hash);
    }

    return hash;
}

static link_ref *
gitdown_add_link_ref(link_ref **refs, const uint8_t *name, size_t size) {
    link_ref *ref = calloc(1, sizeof(link_ref));

    ref->id = gitdown_hash_link_ref(name, size);
    ref->next = refs[ref->id % 8];

    refs[ref->id % 8] = ref;
    return ref;
}

static link_ref *
gitdown_find_link_ref(link_ref **refs, uint8_t *name, size_t size) {
    uint8_t hash = gitdown_hash_link_ref(name, size);

    link_ref *ref = refs[hash % 8];
    while (ref != NULL) {
        if (ref->id == hash) {
            return ref;
        }

        ref = ref->next;
    }

    return NULL;
}

static void
gitdown_free_link_refs(link_ref **refs) {
    for (size_t i = 0; i < 8; i++) {
        link_ref *ref = refs[i];
        link_ref *next;

        while (ref) {
            next = ref->next;
            gitdown_buffer_free(ref->link);
            gitdown_buffer_free(ref->title);
            free(ref);
            ref = next;
        }
    }
}

gitdown_document_parser*
gitdown_parser_alloc(){
    gitdown_document_parser* parser = malloc(sizeof(gitdown_document_parser));
    parser->pos = 0;
    parser->data = gitdown_buffer_alloc(1024);
    return parser;
}

gitdown_document*
gitdown_document_alloc(const gitdown_document_renderer* renderer) {
    gitdown_document *doc = malloc(sizeof(gitdown_document));
    memcpy(&doc->renderer, renderer, sizeof(gitdown_document_renderer));
    gitdown_document_parser* parser = gitdown_parser_alloc();
    memcpy(&doc->parser, parser, sizeof(gitdown_document_parser));
    return doc;
}


static int
is_whitespace(uint8_t c){
    return c == '\n' || c == ' ' || c == '\t' || c == '\0' || c == '\r';
}

static int
is_number(uint8_t c){
    return isdigit(c);
}

static uint8_t
gitdown_parser_next_char(gitdown_document_parser* parser){
    return parser->data->data[parser->pos++];
}

static uint8_t
gitdown_parser_next_real_char(gitdown_document_parser* parser){
    uint8_t c;
    while(is_whitespace(c = gitdown_parser_next_char(parser)));
    return c;
}

void
gitdown_document_render(gitdown_document* doc, gitdown_buffer* ob, const uint8_t* data, size_t size){
    memset(&doc->link_refs, 0x0, 8 * sizeof(void*));

    if(doc->renderer.doc_header){
        doc->renderer.doc_header(ob);
    }

    gitdown_document_parser* parser = &doc->parser;
    gitdown_buffer_put(parser->data, data, size);

    uint8_t c;
    while(parser->pos < size && (c = gitdown_parser_next_char(parser)) != '\0'){
        if(c == '#'){
            gitdown_buffer* buffer = gitdown_buffer_alloc(64);

            while((c = gitdown_parser_next_char(parser)) != '\n'){
                gitdown_buffer_putc(buffer, c);
            }

            if(is_number(gitdown_buffer_getc(buffer, 0))){
                if(doc->renderer.issue_link){
                    doc->renderer.issue_link(ob, buffer);
                }
            } else{
                if(doc->renderer.header){
                    doc->renderer.header(ob, buffer);
                }
            }

            gitdown_buffer_free(buffer);
        } else if(c == '-'){
            if((c = gitdown_parser_next_real_char(parser)) == '-' && (c = gitdown_parser_next_real_char(parser)) == '-'){
                if(doc->renderer.hrule){
                    doc->renderer.hrule(ob);
                }
            } else{
                fprintf(stderr, "Expected: -; Got: %c\n", c);
                abort();
            }
        } else if(c == '@'){
            gitdown_buffer* buffer = gitdown_buffer_alloc(64);
            while(!is_whitespace(c = gitdown_parser_next_char(parser))){
                gitdown_buffer_putc(buffer, c);
            }

            if(doc->renderer.user_link){
                doc->renderer.user_link(ob, buffer);
            }

            gitdown_buffer_free(buffer);
        } else if(c == '.'){
            if(doc->renderer.line_break){
                doc->renderer.line_break(ob);
            }
        } else if(isdigit(c)){
            gitdown_buffer* index_buffer = gitdown_buffer_alloc(10);
            gitdown_buffer_putc(index_buffer, c);
            while(isdigit(c = gitdown_parser_next_char(parser))){
                gitdown_buffer_putc(index_buffer, c);
            }

            int index =  atoi(gitdown_buffer_cstr(index_buffer));

            gitdown_buffer_free(index_buffer);

            gitdown_buffer* content_buffer = gitdown_buffer_alloc(64);
            while((c = gitdown_parser_next_char(parser)) != '\n'){
                gitdown_buffer_putc(content_buffer, c);
            }

            if(doc->renderer.ordered_list){
                doc->renderer.ordered_list(ob, content_buffer, index);
            }

            gitdown_buffer_free(content_buffer);
        } else if(c == '+'){
            gitdown_buffer* content_buffer = gitdown_buffer_alloc(64);

            while((c = gitdown_parser_next_char(parser)) != '\n'){
                gitdown_buffer_putc(content_buffer, c);
            }

            if(doc->renderer.unordered_list){
                doc->renderer.unordered_list(ob, content_buffer);
            }

            gitdown_buffer_free(content_buffer);
        }
    }

    if(doc->renderer.doc_footer){
        doc->renderer.doc_footer(ob);
    }

    gitdown_free_link_refs(doc->link_refs);
}

void
gitdown_document_free(gitdown_document* doc){
    gitdown_buffer_free(doc->parser.data);
    free(doc);
}

#ifdef __cplusplus
}
#endif

#endif