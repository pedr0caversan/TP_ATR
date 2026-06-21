#pragma once

// Instrumentação de tempo para análise de escalabilidade das tasks.
// Ativar com: make ANALISE=1
// Sem a flag, a struct tem métodos vazios e o compilador elimina as chamadas.

#include <chrono>

#define ANALISE 1

#ifdef ANALISE

#include <cstdio>

// Estrutura de medição de tempo de execução, jitter e bloqueio, a partir dos
// quais se pode inferir response time e lateness
// OBS.: printf's durante a execução interferem na própria medição, servem
// apenas para debug
struct Medicao {
        const char* name;
        double worst_case_us = 0.0;
        double sum_us = 0.0;
        long n = 0;
        std::chrono::steady_clock::time_point initial_t = {};

        void inicio() { initial_t = std::chrono::steady_clock::now(); }

        // Finaliza a medição. Imprime estatísticas a cada 'imprimir_a_cada'
        // chamadas.
        void fim(int imprimir_a_cada = 0) {
            double us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - initial_t)
                            .count();
            _acumular(us);
            // if (imprimir_a_cada > 0 && n % imprimir_a_cada == 0) {
            //     printf("[ANALISE][%s] worst_case=%.0fus | media=%.0fus\n",
            //     name,
            //            worst_case_us, sum_us / n);
            // }
        }

        // Mede o jitter de release = atraso real do wakeup em relação ao
        // esperado
        void jitter(std::chrono::steady_clock::time_point esperado,
                    int imprimir_a_cada = 0) {
            double us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - esperado)
                            .count();
            _acumular(us);
            // if (imprimir_a_cada > 0 && n % imprimir_a_cada == 0) {
            //     printf(
            //         "[ANALISE][%s] jitter: pior_caso=%.0fus |
            //         media=%.0fus\n", name, worst_case_us, sum_us / n);
            // }
        }

        void resumo() const {
            // if (n > 0) {
            //     printf("[ANALISE][%s] pior_caso=%.0fus | media=%.0fus\n",
            //     name,
            //            worst_case_us, sum_us / n);
            // }
        }

    private:
        void _acumular(double us) {
            sum_us += us;
            ++n;
            if (us > worst_case_us) worst_case_us = us;
        }
};

// Imprime o resumo final da análise de tempo de uma task
inline void resumo_task(const char* task, const Medicao* exec,
                        const Medicao* jitter = nullptr,
                        const Medicao* bloqueio = nullptr) {
    if (!exec || exec->n == 0) return;

    double pior_total = (jitter ? jitter->worst_case_us : 0.0) +
                        (bloqueio ? bloqueio->worst_case_us : 0.0) +
                        exec->worst_case_us;

    printf("\n[ANALISE][%s] n=%ld\n", task, exec->n);
    printf("[ANALISE][%s] jitter de release  : pior=%.0fus | media=%.0fus\n",
           task, jitter ? jitter->worst_case_us : 0.0,
           jitter ? jitter->sum_us / jitter->n : 0.0);
    printf("[ANALISE][%s] bloqueio (sem/mtx) : pior=%.0fus | media=%.0fus\n",
           task, bloqueio ? bloqueio->worst_case_us : 0.0,
           bloqueio ? bloqueio->sum_us / bloqueio->n : 0.0);
    printf("[ANALISE][%s] execução           : pior=%.0fus | media=%.0fus\n",
           task, exec->worst_case_us, exec->sum_us / exec->n);
    printf("\n[ANALISE][%s] TOTAL pior caso    : %.0fus\n\n", task, pior_total);
}

#else  // sem flag ANALISE: métodos vazios, compilador elimina as chamadas

struct Medicao {
        const char* name = nullptr;
        void inicio() {}
        void fim(int = 0) {}
        void jitter(std::chrono::steady_clock::time_point, int = 0) {}
        void resumo() const {}
};

inline void resumo_task(const char*, const Medicao*, const Medicao* = nullptr,
                        const Medicao* = nullptr) {}

#endif  // ANALISE
