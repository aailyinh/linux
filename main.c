#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

sem_t smphr;

bool sigterm_flag = false;
bool sigint_flag = false;
bool sigchild_flag = false;

void sigtermHandler(int signum){
    sigterm_flag = true;
}

void sigintHandler(int signum){
    sigint_flag = true;
}

void sigchildHandler(int signum){
    sigchild_flag = true;
}

void newLog(const char* msg){
    int FileDescriptor = open("log.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
    write(FileDescriptor, msg, strlen(msg));
    close(FileDescriptor);
}

void execute(char** argv){
    int od = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
    dup2(od, STDOUT_FILENO);
    close(od);
    execv(argv[0], argv);
}

int deamon(char* fileName){
    signal(SIGTERM, sigtermHandler); // here--------------------
    signal(SIGCHLD, sigchildHandler);
    signal(SIGINT, sigintHandler);
    sem_init(&smphr, 0, 1);
    int counter = 0;
    int descriptor = open(fileName, O_RDWR, S_IRWXU);
    char commands[64256];
    read(descriptor, commands, sizeof(commands));
    close(descriptor);
    char* commandLines[10];
    char* currentCommand;
    currentCommand = strtok(commands, "\n");
    while(currentCommand != NULL){
        commandLines[counter++] = currentCommand;
        currentCommand = strtok(NULL, "\n");
    }
    while(!sigterm_flag){
        if(sigint_flag){
            for(int i = 0; i < counter; i++){
                pid_t pid;
                pid = fork();
                if(pid == 0){
                    char *newArgv[10];
                    char *arg;
                    int cnt = 0;
                    arg = strtok(commandLines[i], " ");
                    newArgv[cnt++] = arg;
                    while (arg != NULL)
                    {
                        arg = strtok(NULL, " ");
                        newArgv[cnt++] = arg;
                    }
                    newArgv[cnt] = NULL;
                    int wait = sem_wait(&smphr);
                    if(wait != -1){
                        newLog("command is executed \n");
                        execute(newArgv);
                    }
                    else{
                        newLog("Error of sem_wait \n");
                    }
                }
                else if (pid > 0){
                    while(1){
                        if(sigchild_flag){
                            waitpid(-1, NULL, 0);
                            sem_post(&smphr);
                            sigchild_flag = false;
                            break;
                        }
                        pause();
                    }
                }
                else if (pid == -1){
                    newLog("Fork Error \n");
                }
                sigint_flag = false;
            }
        }
        pause();
    }
    if(sigterm_flag){
        sem_destroy(&smphr);
        newLog("Finished");
        exit(EXIT_SUCCESS);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    pid_t pid;
    pid = fork();
    if(pid == 0){
        umask(0);
        setsid();
        deamon(argv[1]);
    }
    else if(pid < 0){
        newLog("Error \n");
        exit(EXIT_FAILURE);
    }
    else if(pid > 0 ){
        exit(EXIT_SUCCESS);
    }
    return 0;
}
