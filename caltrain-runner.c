
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

struct station {
    int free_seats; 
    int passengers_waiting; 
    int passengers_leaving;
    pthread_mutex_t mutex;
    pthread_cond_t free_seats_available; 
    pthread_cond_t passengers_on_board; 
};
void station_init(struct station *station);
void station_load_train(struct station *station, int count);
void station_wait_for_train(struct station *station);
void station_on_board(struct station *station);
volatile int threads_completed = 0;

void *
passenger_thread(void *arg) {
    struct station *station = (struct station *) arg;
    station_wait_for_train(station);
    __sync_add_and_fetch(&threads_completed, 1);
    return NULL;
}

struct load_train_args {
    struct station *station;
    int free_seats;
};

volatile int load_train_returned = 0;

void *
load_train_thread(void *args) {
    struct load_train_args *ltargs = (struct load_train_args *) args;
    station_load_train(ltargs->station, ltargs->free_seats);
    load_train_returned = 1;
    return NULL;
}

const char *alarm_error_str;
int alarm_timeout;

void
_alarm(int seconds, const char *error_str) {
    alarm_timeout = seconds;
    alarm_error_str = error_str;
    alarm(seconds);
}

void
alarm_handler(int foo) {
    fprintf(stderr, "Error: Failed to complete after %d seconds. Something's "
                    "wrong, or your system is terribly slow. Possible error hint: [%s]\n",
            alarm_timeout, alarm_error_str);
    exit(1);
}

#ifndef MIN
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)
#endif

int
main() {
    struct station station;
    station_init(&station);

    srandom(getpid() ^ time(NULL));

    signal(SIGALRM, alarm_handler);

    _alarm(1, "station_load_train() did not return immediately when no waiting passengers");
    station_load_train(&station, 0);
    station_load_train(&station, 10);
    _alarm(0, NULL);

    const int total_number_of_passengers = 100;
    int passengers_left =  total_number_of_passengers;
    for ( int i = 0; i < total_number_of_passengers; i++) {
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, passenger_thread, &station);
        if (ret != 0) {

            perror("pthread_create");
            exit(1);
        }
    }
    _alarm (2, "station_load_train() did not return immediately when no free seats");
    station_load_train(&station, 0);
    _alarm(0, NULL);

    int total_number_of_passengers_on_board = 0;
    const int max_free_seats_per_train = 50;
    int pass = 0;
    while (passengers_left > 0) {
        _alarm(2, "Some more complicated issue appears to have caused passengers "
                "not to board when given the opportunity");

        int free_seats = random() % max_free_seats_per_train;

        printf("Train entering station with %d free seats\n", free_seats);
        load_train_returned = 0;
        struct load_train_args args = {&station, free_seats};
        pthread_t lt_tid;
        int ret = pthread_create(&lt_tid, NULL, load_train_thread, &args);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }

        int threads_to_reap = MIN(passengers_left, free_seats);
        int threads_reaped = 0;
        while (threads_reaped < threads_to_reap) {
            if (load_train_returned) {
                fprintf(stderr, "Error: station_load_train returned early!\n");
                exit(1);
            }
            if (threads_completed > 0) {
                if ((pass % 2) == 0)
                    usleep(random() % 2);
                threads_reaped++;
                station_on_board(&station);
                __sync_sub_and_fetch(&threads_completed, 1);
            }
        }
        for (int i = 0; i < 1000; i++) {
            if (i > 50 && load_train_returned)
                break;
            usleep(1000);
        }

        if (!load_train_returned) {
            fprintf(stderr, "Error: station_load_train failed to return\n");
            exit(1);
        }

        while (threads_completed > 0) {
            threads_reaped++;
            __sync_sub_and_fetch(&threads_completed, 1);
        }

        passengers_left -= threads_reaped;
        total_number_of_passengers_on_board += threads_reaped;
        printf("Train departed station with %d new passenger(s) (expected %d)%s\n",
               threads_reaped, threads_to_reap,
               (threads_to_reap != threads_reaped) ? " *****" : "");

        if (threads_to_reap != threads_reaped) {
            fprintf(stderr, "Error: Too many passengers on this train!\n");
            exit(1);
        }

        pass++;
    }

    if (total_number_of_passengers_on_board ==  total_number_of_passengers) {
        printf("Looks good!\n");
        return 0;
    } else {
        fprintf(stderr, "Error: expected %d total boarded passengers, but got %d!\n",
                total_number_of_passengers, total_number_of_passengers_on_board);
        return 1;
    }
}
