#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "cpu_6502.h"
#include "cart.h"

#define MAX_TOKENS 32


#ifdef DEBUG_LOG
FILE *logfile;
#endif



void print_cpu_state(cpu_t *cpu) {
    printf("PC\t\tA\t\tX\t\tY\t\tSP\t\tN V _ B D I Z C\n"); 
    printf("0x%.4X\t\t0x%.2X\t\t0x%.2X\t\t0x%.2X\t\t0x%2X\t\t%d %d %d %d %d %d %d %d\n", cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, (cpu->status >> 7) & 1, (cpu->status >> 6) & 1, (cpu->status >> 5) & 1, (cpu->status >> 4) & 1, (cpu->status >> 3) & 1, (cpu->status >> 2) & 1, (cpu->status >> 1) & 1, (cpu->status >> 0) & 1);
}

int get_command(char **command, size_t *len, char **tokens) {
    ssize_t result = getline(command, len, stdin);

    if (result < 0)
        return result;

    int tok_num = 0;
    char **current = tokens;
    *current = strtok(*command, " \r\n");

    while (*current != NULL) {
        ++tok_num;
        ++current;
        if (tok_num >= MAX_TOKENS) {
            errno = ENOBUFS;
            return -1;
        }
        *current = strtok(NULL, " \r\n");
    }
    return 0;
}

void print_usage(void) {
    printf("\n"
        "Commands:\n"
        "(p)oke      address  data\n"
        "(d)ump      count    address\n"
        "(r)egister  name     data\n"
        "(l)oad      address  filepath\n"
        "(s)tep\n"
        "(e)xecute\n"
        "(q)uit\n\n");
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
    uint8_t data = (uint8_t)strtol(str, NULL, 0);
    return data;
}

/* returns zero normally and nonzero to signal quit */
int execute_command(cpu_t *cpu, char **tokens) {
    char **current = tokens;

    if (*current == NULL)
        return 0;

    char c = (*current)[0];
    ++current;

    switch (c) {
        /* poke */
        case 'p':
        {
            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply address and data to poke\n");
                print_usage();
                break;
            }

            uint16_t address = parse_address(*current++);
            uint8_t data = parse_data(*current++);
            printf("poke: address = %#X data = %#X\n", address, data);
            bus_write(address, data);
            break;
        }

        /* dump */
        case 'd':
        {
            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply count and address to dump\n");
                print_usage();
                break;
            }

            uint16_t count = (uint16_t)atoi(*current++);
            uint16_t start = parse_address(*current++);
            printf("\n");
            for (uint16_t address = start; count > 0; --count, ++address) 
                printf("0x%.4X\t\t0x%.2X\n", address, bus_read(address));
            printf("\n");
            break;
        }

        /* register */
        case 'r':
        {
            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply register name and data to register\n");
                print_usage();
                break;
            }

            char reg = (*current++)[0];
            uint8_t data = parse_data(*current++);

            switch(reg) {
                case 'a': cpu->a      = data; break;
                case 'x': cpu->x      = data; break;
                case 'y': cpu->y      = data; break;
                case 's': cpu->sp     = data; break;
                case 'p': cpu->status = data; break;
                default: 
                    printf("Invalid register name. Valid names include: a, x, y, sp, p (status)\n");
                    break;
            }
            break;
        }

        /* load */
        case 'l':
        {

            if (*current == NULL || *(current+1) == NULL) {
                printf("Error: must supply address and filepath to load\n");
                print_usage();
                break;
            }

            uint16_t address = parse_address(*current++);
            char *filepath = *current++;

            if (strcasestr(filepath, ".nes")) {
                cart_t cart = ines_read(filepath);
                load_memory(address, cart.prg_rom, CART_PRG_ROM_SIZE(cart.header));
                delete_cart(&cart);
                cpu_reset(cpu);
            } else {

                FILE *fp = fopen(filepath, "r");
                if (!fp) {
                    perror("fopen");
                    return 1;
                }

                if (fseek(fp, 0L, SEEK_END) < 0) {
                    perror("fseek");
                    return 1;
                }

                int bytes = ftell(fp);
                if (bytes < 0) {
                    perror("ftell");
                    return 1;
                }

                rewind(fp);

                uint8_t *buf = malloc(bytes * sizeof(uint8_t));
                if (!buf) {
                    perror("malloc");
                    return 1;
                }

                fread(buf, sizeof(uint8_t), bytes, fp);

                if (ferror(fp) && !feof(fp)) {
                    perror("fread");
                    free(buf);
                    return 1;
                }

                load_memory(address, buf, bytes);

                free(buf);
            }
            break;
        }

        /* step */
        case 's':
        {
            printf("step\n");
            cpu_tick(cpu);
            break;
        }

        /* execute */
        case 'e':
        {
            printf("execute\n");
            cpu_run(cpu);
            break;
        }

        /* quit */
        case 'q':
            printf("Exiting.\n");
            return 1;

        default:
            printf("Invalid command\n");
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

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    cpu_t cpu;
    cpu_reset(&cpu);

    size_t command_size = 64;
    char *command = malloc(command_size * sizeof(char)); 
    char *tokens[MAX_TOKENS];
    int quit = 0;
    int rc = 0;

    /*init_test_asm();*/

    while(!quit) {
        print_cpu_state(&cpu);
        printf("\n> ");

        if (get_command(&command, &command_size, tokens) < 0) {
            fprintf(stderr, "Error: failed to read command: %s\n", strerror(errno));
            rc = 1;
            break;
        }

        quit = execute_command(&cpu, tokens);
    }

    free(command);

#ifdef DEBUG_LOG
    if (logfile)
        fclose(logfile);
#endif


    return rc;
}


