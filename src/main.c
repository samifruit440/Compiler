#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "parser.h"
#include "codegen.h"
#include "ast.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
    int arg_idx = 1;
    const char *source_file = NULL;
    
    /* Parse command-line flags */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-O] <source.c>\n", argv[0]);
        fprintf(stderr, "  -O    Enable compile-time evaluation (constant folding)\n");
        return 1;
    }
    
    /* Check for -O flag (compile-time evaluation) */
    if (argc >= 2 && strcmp(argv[1], "-O") == 0) {
        compilation_mode = MODE_CTE;
        arg_idx = 2;
        if (argc < 3) {
            fprintf(stderr, "Error: Expected source file after -O flag\n");
            fprintf(stderr, "Usage: %s [-O] <source.c>\n", argv[0]);
            return 1;
        }
    }
    
    source_file = argv[arg_idx];
    
    /* Read source file */
    FILE *src = fopen(source_file, "r");
    if (!src) {
        fprintf(stderr, "Error: Could not open '%s'\n", source_file);
        return 1;
    }
    
    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    fseek(src, 0, SEEK_SET);
    
    char *input = malloc(size + 1);
    fread(input, 1, size, src);
    input[size] = '\0';
    fclose(src);
    
    /* Create out directory */
    mkdir("out", 0755);
    
    /* Dump tokens to file */
    dump_tokens_to_file(input, "out/tokens.txt");
    
    /* Parse */
    Expr *expr = parse_program(input);
    
    /* Generate output filename */
    char outfile[256];
    snprintf(outfile, sizeof(outfile), "out/output.s");
    
    /* Generate assembly */
    FILE *out = fopen(outfile, "w");
    if (!out) {
        fprintf(stderr, "Error: Could not open '%s' for writing\n", outfile);
        return 1;
    }
    
    emit_program(out, expr);
    fclose(out);
    
    const char *mode_str = (compilation_mode == MODE_CTE) ? " (CTE)" : " (RTE)";
    printf("Compiled '%s' → '%s'%s\n", source_file, outfile, mode_str);
    
    /* Clean up */
    expr_free(expr);
    free(input);
    
    return 0;
}
