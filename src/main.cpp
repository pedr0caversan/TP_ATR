#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

/* Gerencia criação de processos, apenas*/
int main()
{
    /*=====================================================================
    Criação de processos da lógica interna do robô
    =====================================================================*/

    pid_t pid_main_process = fork();

    if (pid_main_process < 0)
    {
        std::cerr << "Erro no fork para processo principal\n";
        return EXIT_FAILURE;
    }

    if (pid_main_process == 0)
    {
        char* args[] = {(char*)"mainProcessInit", nullptr};
        execv("./build/mainProcessInit", args);
        std::cerr << "Erro no execv do processo principal\n";
        exit(EXIT_FAILURE);
    }

    pid_t pid_nav_command = fork();

    if (pid_nav_command < 0)
    {
        std::cerr << "Erro no fork para comando de navegação\n";
        return EXIT_FAILURE;
    }

    if (pid_nav_command == 0)
    {
        char* args[] = {(char*)"navCommandInit", nullptr};
        execv("./build/navCommandInit", args);
        std::cerr << "Erro no execv do processo de comando de navegação\n";
        exit(EXIT_FAILURE);
    }

    /*=====================================================================
    Criação de processos relacionados à interface
    =====================================================================*/

    pid_t pid_simulation = fork();

    if (pid_simulation < 0)
    {
        std::cerr << "Erro no fork para simulação\n";
        return EXIT_FAILURE;
    }

    if (pid_simulation == 0)
    {
        char* args[] = {(char*)"python3",
                        (char*)"src/interface/simulation/SimulationInit.py",
                        nullptr};
        // execute vector path (procura o executável no path)
        execvp("python3", args);
        std::cerr << "Erro no execvp da simulação\n";
        exit(EXIT_FAILURE);
    }

    pid_t pid_remote_op = fork();

    if (pid_remote_op < 0)
    {
        std::cerr << "Erro no fork para simulação\n";
        return EXIT_FAILURE;
    }

    if (pid_remote_op == 0)
    {
        char* args[] = {(char*)"python3",
                        (char*)"src/interface/simulation/SimulationInit.py",
                        nullptr};
        // execute vector path (procura o executável no path)
        execvp("python3", args);
        std::cerr << "Erro no execvp da simulação\n";
        exit(EXIT_FAILURE);
    }

    /*=====================================================================
    Espera pela finalização dos processos
    =====================================================================*/

    // TODO (Pedro) : verificar se será necessário saber a forma que o filho
    // terminou pelo segundo argumento da função
    waitpid(pid_main_process, nullptr, 0);
    waitpid(pid_nav_command, nullptr, 0);
    waitpid(pid_simulation, nullptr, 0);
    waitpid(pid_remote_op, nullptr, 0);

    return EXIT_SUCCESS;
}