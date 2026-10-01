/* ============================================================================
 * TP1 - Sistemas Operacionais
 * Parte 1 - Leitores x Escritores
 *
 * Versao 1: SEM PREFERENCIA de ordem de acesso
 *           -> PODE OCORRER LEITURA SUJA (dirty read)
 *
 * Estrategia:
 *   - Escritores usam mutex APENAS entre si (evita perda de atualizacao
 *     entre escritores).
 *   - Leitores NAO usam mutex: leem a qualquer momento.
 *   - O escritor faz a atualizacao em DUAS ETAPAS com sleep no meio,
 *     deixando uma janela onde o estado fica inconsistente. Leitor que
 *     ler nessa janela ve "leitura suja".
 *
 * Baseado nos exemplos do professor:
 *   - testa-concorrencia-contador-global.c   (uso de pthread_mutex_t)
 *   - teste-concorrenica2-exemplo-saldo.c    (sleep para simular tempos)
 *   - criacao-threads-simples.c              (pthread_create / join)
 * ============================================================================ */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_LEITORES    3
#define NUM_ESCRITORES  2
#define NUM_OPERACOES   5

/* Estado compartilhado: duas contas que, em condicoes normais,
 * devem SEMPRE somar SALDO_TOTAL. */
#define SALDO_TOTAL 200
int conta_a = 100;
int conta_b = 100;

/* Mutex usado APENAS pelos escritores (entre eles). */
pthread_mutex_t mutex_escritores;

/* ----------------------------------------------------------------------------
 * LEITOR
 * Le sem qualquer lock. Se cair na janela critica do escritor,
 * ve um estado inconsistente = leitura suja.
 * ---------------------------------------------------------------------------- */
void *leitor(void *arg) {
    long id = (long)arg;

    for (int i = 0; i < NUM_OPERACOES; i++) {
        sleep(1 + (rand() % 2));   /* velocidade variavel */

        int a = conta_a;
        int b = conta_b;
        int total = a + b;

        printf("[Leitor %ld] conta_a=%3d  conta_b=%3d  total=%3d",
               id, a, b, total);
        if (total != SALDO_TOTAL)
            printf("   <<<< LEITURA SUJA (estado inconsistente!)");
        printf("\n");
        fflush(stdout);

        sleep(1);
    }
    pthread_exit(NULL);
}

/* ----------------------------------------------------------------------------
 * ESCRITOR
 * Serializa com outros escritores via mutex, mas faz a atualizacao em
 * duas etapas com sleep no meio. Durante esse sleep, o estado esta
 * "sujo" e pode ser lido por qualquer leitor.
 * ---------------------------------------------------------------------------- */
void *escritor(void *arg) {
    long id = (long)arg;

    for (int i = 0; i < NUM_OPERACOES; i++) {
        sleep(1 + (rand() % 3));

        pthread_mutex_lock(&mutex_escritores);   /* trava SO contra outros escritores */

        printf("[Escritor %ld] iniciando transferencia (debita A, credita B)...\n", id);
        conta_a -= 10;
        printf("[Escritor %ld] debitou 10 de conta_a (agora=%d). Dormindo...\n",
               id, conta_a);
        fflush(stdout);

        sleep(2);   /* <<< JANELA DE LEITURA SUJA >>> */

        conta_b += 10;
        printf("[Escritor %ld] creditou 10 em conta_b (agora=%d). Fim da operacao.\n",
               id, conta_b);
        fflush(stdout);

        pthread_mutex_unlock(&mutex_escritores);
        sleep(1);
    }
    pthread_exit(NULL);
}

/* ----------------------------------------------------------------------------
 * MAIN
 * ---------------------------------------------------------------------------- */
int main(void) {
    pthread_t leitores[NUM_LEITORES];
    pthread_t escritores[NUM_ESCRITORES];

    srand(time(NULL));
    pthread_mutex_init(&mutex_escritores, NULL);

    printf("=== TP1 - Parte 1 - Versao 1 (sem preferencia, leitura suja permitida) ===\n");
    printf("Estado inicial: conta_a=%d conta_b=%d total=%d\n\n",
           conta_a, conta_b, conta_a + conta_b);

    /* Cria escritores primeiro (eles sao mais lentos) */
    for (long i = 0; i < NUM_ESCRITORES; i++) {
        printf("Main: criando Escritor %ld\n", i);
        pthread_create(&escritores[i], NULL, escritor, (void *)i);
    }
    /* Cria leitores */
    for (long i = 0; i < NUM_LEITORES; i++) {
        printf("Main: criando Leitor %ld\n", i);
        pthread_create(&leitores[i], NULL, leitor, (void *)i);
    }

    /* Barreira de sincronizacao: espera todos terminarem */
    for (int i = 0; i < NUM_ESCRITORES; i++)
        pthread_join(escritores[i], NULL);
    for (int i = 0; i < NUM_LEITORES; i++)
        pthread_join(leitores[i], NULL);

    pthread_mutex_destroy(&mutex_escritores);

    printf("\nEstado final: conta_a=%d conta_b=%d total=%d\n",
           conta_a, conta_b, conta_a + conta_b);
    printf("(No final o total esta consistente porque os escritores se respeitaram via mutex.)\n");

    return 0;
}