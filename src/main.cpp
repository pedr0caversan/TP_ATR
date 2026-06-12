#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "IPC/Channels.hpp"
#include "processes/cameraInspection/CameraInspectionTask.hpp"
#include "processes/mainProcess/MainProcessInit.hpp"
#include "processes/navigationCommand/NavigationCommandTask.hpp"

/* Gerencia criação de processos, apenas*/
int main() {
    /*=====================================================================
    Criação de processos da lógica interna do robô
    =====================================================================*/

    pid_t pid_main_process = fork();

    if (pid_main_process < 0) {
        std::cerr << "Erro no fork para processo principal\n";
        return EXIT_FAILURE;
    }

    if (pid_main_process == 0) {
        mainProcessInit();
        exit(EXIT_SUCCESS);
    }

    pid_t pid_nav_command = fork();

    if (pid_nav_command < 0) {
        std::cerr << "Erro no fork para comando de navegação\n";
        return EXIT_FAILURE;
    }

    if (pid_nav_command == 0) {
        navigationCommandHandler();
        exit(EXIT_SUCCESS);
    }

    pid_t pid_camera = fork();

    if (pid_camera < 0) {
        std::cerr << "Erro no fork para câmera de inspeção\n";
        return EXIT_FAILURE;
    }

    if (pid_camera == 0) {
        cameraInspectionHandler();
        exit(EXIT_SUCCESS);
    }

    /*=====================================================================
    Criação de processos relacionados à interface
    =====================================================================*/

    pid_t pid_simulation = fork();

    if (pid_simulation < 0) {
        std::cerr << "Erro no fork para simulação\n";
        return EXIT_FAILURE;
    }

    if (pid_simulation == 0) {
        char* args[] = {(char*)"python3",
                        (char*)"../src/interface/simulation/SimulationInit.py",
                        nullptr};
        execvp("python3", args);
        std::cerr << "Erro no execvp da simulação\n";
        exit(EXIT_FAILURE);
    }

    pid_t pid_remote_op = fork();

    if (pid_remote_op < 0) {
        std::cerr << "Erro no fork para operação remota\n";
        return EXIT_FAILURE;
    }

    if (pid_remote_op == 0) {
        char* args[] = {
            (char*)"python3",
            (char*)"../src/interface/remoteOp/RemoteOperationInit.py", nullptr};
        execvp("python3", args);
        std::cerr << "Erro no execvp da operação remota\n";
        exit(EXIT_FAILURE);
    }

    /*=====================================================================
    Espera pela finalização dos processos
    =====================================================================*/

    waitpid(pid_main_process, nullptr, 0);
    waitpid(pid_nav_command, nullptr, 0);
    waitpid(pid_camera, nullptr, 0);
    waitpid(pid_simulation, nullptr, 0);
    waitpid(pid_remote_op, nullptr, 0);

    // Libera segmento de memória compartilhada
    int shmid = shmget(SHM_NAV_KEY, sizeof(NavInfo), 0666);
    if (shmid >= 0) shmctl(shmid, IPC_RMID, nullptr);

    return EXIT_SUCCESS;
}
