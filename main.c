#include "gitdown.h"

#define HEADER_SIZE 100

static void
pre_doc(gitdown_buffer *ob) {
    gitdown_buffer_puts(ob, "/*\n *");
    gitdown_buffer_putc_rep(ob, '=', HEADER_SIZE);
    gitdown_buffer_putc(ob, '\n');
}

static void
post_doc(gitdown_buffer *ob) {
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, '=', HEADER_SIZE);
    gitdown_buffer_puts(ob, "\n */");
}

static void
header(gitdown_buffer *ob, const gitdown_buffer *content){
    size_t pad = (HEADER_SIZE - content->size) / 2;
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, ' ', pad);
    gitdown_buffer_put(ob, content->data, content->size);
    gitdown_buffer_putc_rep(ob, ' ', pad);
    gitdown_buffer_putc(ob, '\n');
}

static void
hrule(gitdown_buffer* ob){
    gitdown_buffer_puts(ob, " *\n *");
    gitdown_buffer_putc_rep(ob, '-', HEADER_SIZE);
    gitdown_buffer_puts(ob, "\n *\n");
}

static void
user_link(gitdown_buffer* ob, const gitdown_buffer* content){
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, ' ', 5);
    gitdown_buffer_puts(ob, "https://github.com/");
    gitdown_buffer_put(ob, content->data, content->size);
    gitdown_buffer_putc(ob, '\n');
}

static void
ordered_list(gitdown_buffer* ob, const gitdown_buffer* content, int index){
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, ' ', 10);
    gitdown_buffer_printf(ob, "%d.) %s\n", index, gitdown_buffer_cstr((gitdown_buffer *) content));
}

static void
unordered_list(gitdown_buffer* ob, const gitdown_buffer* content){
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, ' ', 10);
    gitdown_buffer_printf(ob, "+ %s\n", gitdown_buffer_cstr((gitdown_buffer *) content));
}

static void
free_line(gitdown_buffer* ob, const gitdown_buffer* content){
    gitdown_buffer_puts(ob, " * ");

    if(content->size > HEADER_SIZE - 2){
        size_t off = 0;
        int len = (int) content->size;

        while(len >= 0){
            gitdown_buffer* line = gitdown_buffer_slice((gitdown_buffer *) content, off, HEADER_SIZE - 2);
            gitdown_buffer_put(ob, line->data, line->size);
            gitdown_buffer_puts(ob, "\n * ");
            gitdown_buffer_free(line);

            off += HEADER_SIZE;
            len -= HEADER_SIZE + 2;
        }
    } else{
        gitdown_buffer_put(ob, content->data, content->size);
    }

    gitdown_buffer_putc(ob, '\n');
}

static void
issue_link(gitdown_buffer* ob, const gitdown_buffer* content){
    gitdown_buffer_puts(ob, " *");
    gitdown_buffer_putc_rep(ob, ' ', 5);
    gitdown_buffer_puts(ob, "https://github.com/");

    gitdown_buffer* b = gitdown_buffer_alloc(64);

    uint8_t c;
    size_t pos = 0;
    while((c = gitdown_buffer_getc((gitdown_buffer *) content, pos++)) != ' '){
        gitdown_buffer_putc(b, c);
    }

    gitdown_buffer* repo = gitdown_buffer_slice((gitdown_buffer *) content, pos, content->size);
    gitdown_buffer_put(ob, repo->data, content->size - pos);
    gitdown_buffer_puts(ob, "/issues/");
    gitdown_buffer_put(ob, b->data, b->size);
    gitdown_buffer_putc(ob, '\n');

    gitdown_buffer_free(b);
    gitdown_buffer_free(repo);
}

static void
line_break(gitdown_buffer* ob){
    gitdown_buffer_puts(ob, " *\n");
}

int main(int argc, char** argv){
    fprintf(stdout, "Parsing %s.\n", argv[1]);

    static const gitdown_document_renderer renderer_defaults = {
            &pre_doc,
            &post_doc,

            &header,
            &hrule,
            &user_link,
            &issue_link,
            &line_break,
            &unordered_list,
            &ordered_list,
            &free_line
    };

    gitdown_document_renderer* renderer = malloc(sizeof(gitdown_document_renderer));
    memcpy(renderer, &renderer_defaults, sizeof(gitdown_document_renderer));
    gitdown_buffer* ib = gitdown_buffer_alloc(1024);
    FILE* file = fopen(argv[1], "r");

    if(gitdown_buffer_putf(ib, file)){
        fprintf(stderr, "I/O error.\n");
        return 5;
    }

    if(file != stdin){
        fclose(file);
    }

    gitdown_buffer* ob = gitdown_buffer_alloc(1024);
    gitdown_document* doc = gitdown_document_alloc(renderer);
    gitdown_document_render(doc, ob, ib->data, ib->size);
    gitdown_buffer_free(ib);
    gitdown_document_free(doc);
    fprintf(stdout, "Output:\n");
    fprintf(stdout, "%s\n", gitdown_buffer_cstr(ob));
    gitdown_buffer_free(ob);
    return 0;
}