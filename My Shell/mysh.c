#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include "tokens.h"

typedef struct {
    int promptType;
    char* promptString;
}Options;

typedef struct {
    bool append;
    char *input_file_name, *output_file_name, **cmd;
}Cmd;

typedef struct {
    bool foreground_process;
    Cmd *cmd_list;
    int num_commands;
}CmdSet;

Options parseArgs(int argc, char *argv[]);
char** removeOperators(char **tokens, int operator_loc);
int error = 0;

Options parseArgs(int argc, char *argv[]){
    Options opts;
    if (argc == 1){
        opts.promptType = 1;
        opts.promptString = "mysh: ";
        return opts;
    }else if (argc == 2){
        if (strcmp(argv[1], "-") == 0){
            opts.promptType = 0;
            opts.promptString = "";
            return opts;
        }else {
            opts.promptType = 2;
            opts.promptString = argv[1];
            return opts;
        }
    }
    fprintf(stderr, "Error: Usage: %s [prompt]\n", "mysh");
    exit(1);
}

char** removeOperators(char **tokens, int operator_loc){
    int counter = 0;
    while(tokens[counter] != NULL){
        counter++;
    }
    char **cmd_tokens = (char**) malloc((counter - 1) * sizeof(char*));

    int offset = 0;
    for (int i = 0; tokens[i] != NULL; i++){
        if ((offset != 2) && (i == operator_loc || (i == operator_loc + 1))) {
            offset++;
        }else{
            cmd_tokens[i - offset] = tokens[i];
        }
    }
    cmd_tokens[counter-offset] = NULL;
    return cmd_tokens;
}

int main(int argc, char **argv){
    char * cmdargv;
    char * fgets_return;
    bool cmds_executed = false;
    char **cmd_tokens;
    int pid, wpid, status;
    Options opts = parseArgs(argc, argv);
    int *foreground_pids;

    while(1){
        printf("%s", opts.promptString);
        cmdargv = malloc(1024);
        do{
            fgets_return = fgets(cmdargv, 1024, stdin);
        }while(fgets_return == NULL && errno == EINTR);

        cmdargv[strlen(cmdargv) - 1] = '\0';
        error = 0;
        if (strcmp(cmdargv, "") == 0)
            error = 1;
        if( fgets_return == NULL || (strcmp(cmdargv, "exit") == 0)){
            if (errno != EINTR){
                exit(0);
            }
        }
        char** cmds = get_tokens(cmdargv, "|");
        int num_cmds = 0;
        while(cmds[num_cmds] != NULL){
            num_cmds++;
        }
        char** tokens = get_tokens(cmdargv, " \t\n");
        int counter = 0;
        int input_num = 0, output_num = 0;
        if (strcmp(&cmdargv[strlen(cmdargv) - 1], "|") == 0){
            fprintf(stderr, "Error: Ambiguous output redirection.\n");
            error = 1;
        }else{
            for (int i = 0; tokens[i] != NULL; i++){
                if (!error){
                    if (strcmp(tokens[i], "<") == 0){
                        input_num++;
                        if (input_num >= 2){
                            fprintf(stderr, "Error: Ambiguous input redirection.\n");
                            error = 1;
                        }
                    }else if (strcmp(tokens[i], ">") == 0){
                        output_num++;
                        if (output_num >= 2){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                        }
                   }else if (strcmp(tokens[i], ">>") == 0){
                        output_num++;
                        if (output_num >= 2){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                       }
                    }else if (strcmp(tokens[i], "|") == 0){
                        output_num++;
                        if (output_num >= 2){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                            output_num = 0;
                        }else if (input_num >= 2 ){
                            fprintf(stderr, "Error: Ambiguous input redirection.\n");
                            error = 1;
                            input_num = 0;
                        }else{
                            counter++;
                            input_num = 1;
                            output_num = 0;
                        }
                    }
                }
            }
        }

        CmdSet set_of_commands;
        if (!error){
            char *background_char;
            background_char = strchr(cmdargv, '&');
            if (strcmp(&cmdargv[strlen(cmdargv)-1], "&") == 0) {
                cmdargv[strlen(cmdargv)-1] = '\0';
                set_of_commands.foreground_process = false;
            }else{
                set_of_commands.foreground_process = true;
            }

            if ((strchr(cmdargv, '&') != NULL) && strcmp(background_char, "&")){
                fprintf(stderr, "Error: \"&\" must be last token on command line\n");
                error = 1;
            }else {
                char **pipe_tokens = get_tokens(cmdargv, "|");
                int num_cmds;
                int counter = 0;
                while (pipe_tokens[counter] != NULL) {
                    counter++;
                }
                set_of_commands.cmd_list = malloc(counter * sizeof(Cmd));
                for (num_cmds = 0; pipe_tokens[num_cmds] != NULL; num_cmds++) {
                    char **cmd_tokens = get_tokens(pipe_tokens[num_cmds], " \t\n");
                    Cmd new_cmd;
                    new_cmd.append = false;
                    new_cmd.input_file_name = malloc(1024 * sizeof(char));
                    new_cmd.output_file_name = malloc(1024 * sizeof(char));
                    new_cmd.cmd = cmd_tokens;
                   for (int num_args = 0; new_cmd.cmd[num_args] != NULL; num_args++) {
                        if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], ">") == 0) {
                            if (new_cmd.cmd[num_args + 1] == NULL) {
                                fprintf(stderr, "Error: Missing filename for output redirection.\n");
                                error = 1;
                            } else if (strcmp(new_cmd.output_file_name, "") != 0) {
                                fprintf(stderr, "Error: Ambiguous output redirection.\n");
                                error = 1;
                            }else {
                                int file_descriptor = open(new_cmd.cmd[num_args + 1], O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                if (file_descriptor == -1){
                                    fprintf(stderr, "Error: open(\"%s\"): %s\n", new_cmd.cmd[num_args + 1], strerror(errno));
                                    error = 1;
                                }else {
                                    close(file_descriptor);
                                    new_cmd.output_file_name = new_cmd.cmd[num_args + 1];
                                    new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                                    if (num_args != 0)
                                        num_args -= 1;
                                }
                            }
                        }
                        if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], "<") == 0) {
                            if (new_cmd.cmd[num_args + 1] == NULL) {
                               fprintf(stderr, "Error: Missing filename for input redirection.\n");
                               error = 1;
                           } else if (strcmp(new_cmd.input_file_name, "") != 0) {
                               fprintf(stderr, "Error: Ambiguous input redirection.\n");
                               error = 1;
                           } else {
                                int file_descriptor = open(new_cmd.cmd[num_args + 1], O_RDONLY);
                                if (file_descriptor == -1){
                                   fprintf(stderr, "Error: open(\"%s\"): %s\n", new_cmd.cmd[num_args + 1], strerror(errno));
                                   error = 1;
                                }else{
                                    close(file_descriptor);
                                    new_cmd.input_file_name = new_cmd.cmd[num_args + 1];
                                    new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                                    if (num_args != 0)
                                        num_args -= 1;
                                }
                            }
                        }
                        if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], ">>") == 0) {
                            if (new_cmd.cmd[num_args + 1] == NULL) {
                                fprintf(stderr, "Error: Missing filename for output redirection.\n");
                                error = 1;
                            } else if (strcmp(new_cmd.output_file_name, "") != 0) {
                                fprintf(stderr, "Error: Ambiguous output redirection.\n");
                                error = 1;
                            } else {
                                new_cmd.output_file_name = new_cmd.cmd[num_args + 1];
                                new_cmd.append = true;
                                new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                                if (num_args != 0)
                                    num_args -= 1;
                            }
                        }
                    }
                    set_of_commands.cmd_list[num_cmds] = new_cmd;
                    set_of_commands.num_commands = counter;
                }
            }
            for (int i = 0; i < set_of_commands.num_commands; i++){
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }
        }

        if (!error){
            for (int i = 0; i < set_of_commands.num_commands; i++){
                if (set_of_commands.num_commands > 1){
                    if ((i == 0) && (strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0) && (set_of_commands.num_commands > 1)){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }else if ((i == set_of_commands.num_commands - 1) && (strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    }else if ((i != 0) && (i != set_of_commands.num_commands - 1)){
                        if ((strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                        }else if ((strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous input redirection.\n");
                            error = 1;
                        }
                    }
                }
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }
            foreground_pids = (int *) malloc(set_of_commands.num_commands * sizeof(int));
        }

        int pipefd[] = {-1, -1}, prevpipefd[] = {-1, -1};
        if (!error) {
            for (int cmd_index = 0; cmd_index < set_of_commands.num_commands; cmd_index++) {
                if ((set_of_commands.num_commands > 1) && (cmd_index < set_of_commands.num_commands - 1)) {
                    pipe(pipefd);
                }
                pid = fork();
                if (pid == 0) {
                    int file_descriptor;
                    if (set_of_commands.num_commands > 1) {
                        if (cmd_index == 0) {
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                        } else if (cmd_index == set_of_commands.num_commands - 1) {
                            dup2(prevpipefd[0], 0);
                            close(prevpipefd[0]);
                        } else {
                            dup2(prevpipefd[0], 0);
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                            close(prevpipefd[0]);
                        }
                    }
                    if ((strcmp(set_of_commands.cmd_list[cmd_index].output_file_name, "") != 0)) {
                        if (set_of_commands.cmd_list[cmd_index].append == true) {
                            file_descriptor = open(set_of_commands.cmd_list[cmd_index].output_file_name,
                                                   O_CREAT | O_WRONLY | O_APPEND,
                                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        } else {
                            file_descriptor = open(set_of_commands.cmd_list[cmd_index].output_file_name,
                                                   O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        }
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", set_of_commands.cmd_list[cmd_index].output_file_name, strerror(errno));
                            exit(0);
                        }
                        int dup_out = dup2(file_descriptor, 1);
                        close(file_descriptor);
                    }
                    if ((strcmp(set_of_commands.cmd_list[cmd_index].input_file_name, "") != 0)) {
                        file_descriptor = open(set_of_commands.cmd_list[cmd_index].input_file_name, O_RDONLY);
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", set_of_commands.cmd_list[cmd_index].input_file_name, strerror(errno));
                            exit(0);
                        }
                        int dup_out = dup2(file_descriptor, 0);
                        close(file_descriptor);
                    }
                    if (execvp(set_of_commands.cmd_list[cmd_index].cmd[0], set_of_commands.cmd_list[cmd_index].cmd) ==
                        -1) {
                        perror("execvp()");
                    }
                    exit(-1);
                } else {
                    foreground_pids[cmd_index] = pid;
                    if (set_of_commands.foreground_process && (cmd_index == (set_of_commands.num_commands - 1))) {
                        int sum = 0;
                        while (sum != (-1 * set_of_commands.num_commands)) {
                            sum =0;
                            for (int i = 0; i < set_of_commands.num_commands; i++){
                                sum += foreground_pids[i];
                            }
                            do{
                                wpid = wait(&status);
                            }while(wpid == -1 && errno == EINTR);
                            for (int i = 0; i < set_of_commands.num_commands; i++){
                                if (foreground_pids[i] == wpid){
                                foreground_pids[i] = -1;
                                }
                            }
                        }
                    }
                    if (set_of_commands.num_commands > 1) {
                        close(pipefd[1]);
                        close(prevpipefd[0]);
                        prevpipefd[0] = pipefd[0];
                        prevpipefd[1] = pipefd[1];
                    }
                }
            }
        }
    }
}