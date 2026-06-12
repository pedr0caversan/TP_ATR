#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "IPC/Channels.hpp"
#include "processes/cameraInspection/CameraInspectionTask.hpp"
#include "processes/mainProcess/MainProcessInit.hpp"
#include "processes/navigationCommand/NavigationCommandTask.hpp"

/* Gerencia criação de processos e IPC, apenas*/
int main() {
    /*=====================================================================
    Criação de processos e IPC da lógica interna do robô
    =====================================================================*/

    // compartilha PIDs antes do fork para que filhos herdem essa informação
    int pid_sharing_shmid =
        shmget(SHM_PID_SHARING_KEY, sizeof(SignalPIDs), 0666 | IPC_CREAT);
    if (pid_sharing_shmid < 0) {
        perror("shmget registry");
        return EXIT_FAILURE;
    }
    SignalPIDs* registry = (SignalPIDs*)shmat(pid_sharing_shmid, nullptr, 0);
    if (registry == (void*)-1) {
        perror("shmat registry");
        return EXIT_FAILURE;
    }
    registry->ready = false;  // filhos aguardarão este flag antes de ler os
                              // PIDs - evita race condition

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

    // preenche memória compartilhada com PIDs que serão usados para realizar
    // IPC por signal
    registry->pid_main_process = pid_main_process;
    registry->pid_nav_command = pid_nav_command;
    registry->pid_camera = pid_camera;
    registry->ready = true;  // libera os filhos que estiverem aguardando
    shmdt(registry);

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

    // Libera segmentos de memória compartilhada
    int shmid = shmget(SHM_NAV_KEY, sizeof(NavInfo), 0666);
    if (shmid >= 0) {
        shmctl(shmid, IPC_RMID, nullptr);
    }

    int reg_shmid = shmget(SHM_PID_SHARING_KEY, sizeof(SignalPIDs), 0666);
    if (reg_shmid >= 0) {
        shmctl(reg_shmid, IPC_RMID, nullptr);
    }

    return EXIT_SUCCESS;
}
