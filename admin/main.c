/*
    Admin CLI Utility for MTProxy
    Main entry point
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "admin/admin-cli.h"

static void print_usage(const char *program_name) {
    printf("MTProxy Admin CLI - Administration Utility\n\n");
    printf("Usage: %s [options] [command]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --host HOST     Server host (default: 127.0.0.1)\n");
    printf("  -p, --port PORT     Server port (default: 8888)\n");
    printf("  -t, --token TOKEN   Authentication token\n");
    printf("  -i, --interactive   Run in interactive mode\n");
    printf("  -j, --json          Output in JSON format\n");
    printf("  -v, --verbose       Verbose output\n");
    printf("  --no-color          Disable colored output\n");
    printf("  --help              Show this help message\n");
    printf("  --version           Show version information\n\n");
    printf("Examples:\n");
    printf("  %s status                    # Show server status\n", program_name);
    printf("  %s stats                     # Show statistics\n", program_name);
    printf("  %s -i                        # Interactive mode\n", program_name);
    printf("  %s cache-stats               # Show cache statistics\n", program_name);
    printf("  %s ratelimit status          # Show rate limit status\n", program_name);
    printf("  %s connections list          # List active connections\n", program_name);
}

static void print_version(void) {
    printf("MTProxy Admin CLI v1.0.0\n");
    printf("Build: March 2026\n");
}

int main(int argc, char *argv[]) {
    admin_cli_context_t ctx;
    
    // Параметры по умолчанию
    const char *host = "127.0.0.1";
    int port = 8888;
    const char *token = NULL;
    int interactive = 0;
    int json_output = 0;
    int verbose = 0;
    int no_color = 0;
    
    // Опции
    static struct option long_options[] = {
        {"host",       required_argument, 0, 'h'},
        {"port",       required_argument, 0, 'p'},
        {"token",      required_argument, 0, 't'},
        {"interactive", no_argument,      0, 'i'},
        {"json",       no_argument,      0, 'j'},
        {"verbose",    no_argument,      0, 'v'},
        {"no-color",   no_argument,      0, 'n'},
        {"help",       no_argument,      0, 'H'},
        {"version",    no_argument,      0, 'V'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "h:p:t:ijvVn", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                token = optarg;
                break;
            case 'i':
                interactive = 1;
                break;
            case 'j':
                json_output = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'n':
                no_color = 1;
                break;
            case 'H':
                print_usage(argv[0]);
                return 0;
            case 'V':
                print_version();
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Инициализация CLI
    if (admin_cli_init(&ctx) != 0) {
        fprintf(stderr, "Failed to initialize CLI\n");
        return 1;
    }
    
    ctx.output_format = json_output ? 1 : 0;
    ctx.verbose = verbose;
    ctx.color_enabled = !no_color;
    
    // Подключение к серверу (опционально)
    if (admin_cli_connect(&ctx, host, port, token) == 0) {
        if (verbose) {
            printf("Connected to %s:%d\n", host, port);
        }
    } else if (verbose) {
        printf("Warning: Could not connect to %s:%d (running in offline mode)\n", host, port);
    }
    
    int ret = 0;
    
    // Если есть команды в аргументах
    if (optind < argc || interactive) {
        if (interactive) {
            // Интерактивный режим
            ret = admin_cli_run(&ctx);
        } else {
            // Выполнение одной команды
            char command[1024] = "";
            
            // Сборка команды из аргументов
            for (int i = optind; i < argc; i++) {
                if (i > optind) strcat(command, " ");
                strcat(command, argv[i]);
            }
            
            ret = admin_cli_run_command(&ctx, command);
        }
    } else {
        // Нет команд - показать справку
        print_usage(argv[0]);
    }
    
    // Очистка
    admin_cli_cleanup(&ctx);
    
    return ret;
}
