/*
 * Copyright (c) 2017-2018 Grzegorz Blach. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <ucl.h>

typedef struct cmdl {
    ucl_emitter_t format;
    char *in;
    char *out;
} cmdl_t;

ucl_object_t *parse(char *fn) {
    if(strcmp(fn, "-") && access(fn, R_OK)) err(2, "%s", fn);

    struct ucl_parser *parser = ucl_parser_new(0);

    if(strcmp(fn, "-")) {
        ucl_parser_add_file(parser, fn);
    } else {
        int size = 0;
        char chunk[1025];
        char *data = malloc(0);
        while(fgets(chunk, 1025, stdin) != NULL) {
            size += 1024;
            data = realloc(data, size+1);
            strncat(data, chunk, 1024);
        }
        ucl_parser_add_string(parser, data, strlen(data));
        free(data);
    }

    const char *error_msg = ucl_parser_get_error(parser);
    if(error_msg) err(3, "%s", error_msg);
    ucl_object_t *object = ucl_parser_get_object(parser);
    ucl_parser_free(parser);
    return object;
}

void emit(char *fn, ucl_emitter_t format, ucl_object_t *object) {
    char *output = (char*) ucl_object_emit(object, format);
    FILE *file;

    if(fn) {
        file = fopen(fn, "w");
        if(! file) err(4, "%s", fn);
    } else {
        file = stdout;
    }

    fputs(output, file);
    if(fn) fclose(file);
    free(output);
}

cmdl_t cmdl_parse(int ac, char **av) {
    cmdl_t cmdl = { UCL_EMIT_CONFIG };

    for(int i=1; i<ac; i++) {
        if(av[i][0] == '-') {
            switch(av[i][1]) {
                case 'j': cmdl.format = UCL_EMIT_JSON; break;
                case 'c': cmdl.format = UCL_EMIT_JSON_COMPACT; break;
                case 'y': cmdl.format = UCL_EMIT_YAML; break;
                case 'm': cmdl.format = UCL_EMIT_MSGPACK; break;
                case '\0': cmdl.in = "-";
            }
        } else if(! cmdl.in) cmdl.in = av[i];
        else if(! cmdl.out) cmdl.out = av[i];
    }

    return cmdl;
}

void help(char *av0) {
    printf(
        "UnCLe 0.2 by Grzegorz Blach\n" \
        "Convert UCL config file to desired format.\n"
        "\n" \
        "Usage: %s [-j|-c|-y|-m] <in file> [out file]\n" \
        "    \tEmit human readable config format\n" \
        "  -j\tEmit fine formatted JSON\n" \
        "  -c\tEmit compacted JSON\n" \
        "  -y\tEmit embedded YAML format\n" \
        "  -m\tEmit msgpack output\n" \
        "\n" \
        "In file can be set to '-' for reading input from stdin.\n" \
        "Out file can be ommited for writing output to stdout.\n", av0
    );
}

int main(int ac, char **av) {
    cmdl_t cmdl = cmdl_parse(ac, av);
    if(! cmdl.in) {
        help(av[0]);
        return 1;
    }

    ucl_object_t *object = parse(cmdl.in);
    emit(cmdl.out, cmdl.format, object);
    ucl_object_unref(object);

    return 0;
}
