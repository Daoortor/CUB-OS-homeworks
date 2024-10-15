#include <semaphore.h>
#include <pthread.h>

enum state {
    MAN,
    WOMAN,
    FREE,
};

struct bathroom {
    enum state state;
    pthread_mutex_t state_mutex;
    sem_t cnt;
};

void bathroom_create(struct bathroom *bathroom) {
    bathroom->state = FREE;
    pthread_mutex_init(&bathroom->state_mutex, NULL);
    sem_init(&bathroom->cnt, 0, 3);
}

struct bathroom bathroom;
int x = (bathroom_create(&bathroom), 0);

void use_bathroom() {}

void man() {

    use_bathroom();
}

void woman() {
    use_bathroom();
}