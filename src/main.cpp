#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

void run_process_control() {
    // Lógica do controle de tempo real e criação de suas threads
    while (true) {
        pause(); 
    }
}

void run_process_comms() {
    // Lógica da comunicação e criação de suas threads
    while (true) {
        pause(); 
    }
}

int main() {
    pid_t pid_control = fork();
    
    if (pid_control < 0) {
        std::cerr << "Erro no fork (Controle)\n";
        return EXIT_FAILURE;
    } 
    
    if (pid_control == 0) {
        run_process_control();
        exit(EXIT_SUCCESS); 
    }

    pid_t pid_comms = fork();
    
    if (pid_comms < 0) {
        std::cerr << "Erro no fork (Comunicação)\n";
        return EXIT_FAILURE;
    }
    
    if (pid_comms == 0) {
        run_process_comms();
        exit(EXIT_SUCCESS);
    }

    // Aguarda filhos terminarem
    int status;
    waitpid(pid_control, &status, 0);
    waitpid(pid_comms, &status, 0);

    return EXIT_SUCCESS;
}