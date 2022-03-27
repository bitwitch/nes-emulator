#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu_6502.h"

#define MAX_TOKENS 32

void print_cpu_state(cpu_6502_t *cpu) {
    printf("PC\t\tA\t\tX\t\tY\t\tSP\t\tN V _ B D I Z C\n"); 
    printf("0x%.4X\t\t0x%.2X\t\t0x%.2X\t\t0x%.2X\t\t0x%2X\t\t%d %d %d %d %d %d %d %d\n", cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, (cpu->status >> 7) & 1, (cpu->status >> 6) & 1, (cpu->status >> 5) & 1, (cpu->status >> 4) & 1, (cpu->status >> 3) & 1, (cpu->status >> 2) & 1, (cpu->status >> 1) & 1, (cpu->status >> 0) & 1);
}

int get_command(char *command, size_t *len, char **tokens) {
    ssize_t result = getline(&command, len, stdin);

    if (result < 0)
        return result;

    int tok_num = 0;
    char **current = tokens;
    *current = strtok(command, " ");

    while (*current != NULL) {
        ++tok_num;
        ++current;
        if (tok_num >= MAX_TOKENS) {
            errno = ENOBUFS;
            return -1;
        }
        *current = strtok(NULL, " ");
    }
    return 0;
}

void print_usage(void) {
    printf("\n"
        "Commands:\n"
        "(p)oke  address data\n"
        "(d)ump  count address\n"
        "(s)tep\n"
        "(r)un\n\n");
}

uint16_t parse_address(char* str) {
    uint16_t address;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        address = (uint16_t)strtol(str, NULL, 16);
    } else {
        address = (uint16_t)strtol(str, NULL, 10);
    }
    return address;

}

uint8_t parse_data(char* str) {
    uint8_t data;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        data = (uint8_t)strtol(str, NULL, 16);
    } else {
        data = (uint8_t)strtol(str, NULL, 10);
    }
    return data;
}

int execute_command(char **tokens) {
    char **current = tokens;
    char c = (*current)[0];
    ++current;

    switch (c) {
        case 'p':
        {
            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply address and data\n");
                print_usage();
                break;
            }

            uint16_t address = parse_address(*current++);
            uint8_t data = parse_data(*current++);
            printf("poke: address = %#X data = %#X\n", address, data);
            break;
        }

        case 'd':
        {
            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply count and address\n");
                print_usage();
                break;
            }

            uint16_t count = (uint16_t)atoi(*current++);
            uint16_t address = parse_address(*current++);
            printf("dump: count = %d address = %#X\n", count, address);
            break;
        }
        
        case 's':
            printf("step\n");
            break;

        case 'r':
            printf("run\n");
            break;

        default:
            print_usage();
            break;
    }

    return 0;
}



/*
    print cpu state
    read a line of input
    parse command
    execute command
*/
int repl(void) {
    int quit = 0;

    cpu_6502_t cpu;
    reset_6502(&cpu);

    size_t command_size = 32;
    char *command = malloc(command_size * sizeof(char));
    char *tokens[MAX_TOKENS];

    while(!quit) {

        print_cpu_state(&cpu);
        printf("\n> ");

        if (get_command(command, &command_size, tokens) < 0) {
            fprintf(stderr, "Error: failed to read command: %s\n", strerror(errno));
            free(command);
            return 1;
        }

        execute_command(tokens);
    }

    free(command);

    return 0;
}


