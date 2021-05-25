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

void station_init(struct station *station) {
    station->free_seats = 0;
    station->passengers_waiting = 0;
    station->passengers_leaving = 0;
    pthread_mutex_init(&(station->mutex), NULL);
    pthread_cond_init(&(station->free_seats_available), NULL);
    pthread_cond_init(&(station->passengers_on_board), NULL);
}
void station_load_train(struct station *station, int count) {
    pthread_mutex_lock(&(station->mutex)); 
    if (!count || !station->passengers_waiting) { 
        pthread_mutex_unlock(&(station->mutex)); 
        return; 
    }
    station->free_seats = count;
    pthread_cond_broadcast(&(station->free_seats_available));
    pthread_cond_wait(&(station->passengers_on_board), &(station->mutex));
    station->free_seats = 0;
    pthread_mutex_unlock(&(station->mutex));

void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&(station->mutex));
    station->passengers_waiting++;
    while (!station->free_seats)
        pthread_cond_wait(&(station->free_seats_available), &(station->mutex)); 
    station->passengers_waiting--;
    station->passengers_leaving++;
    station->free_seats--;
    pthread_mutex_unlock(&(station->mutex)); 
}

void station_on_board(struct station *station) {
    pthread_mutex_lock(&(station->mutex)); 
    if (!station->passengers_leaving && !(station->passengers_waiting && station->free_seats))
        pthread_cond_signal(&(station->passengers_on_board));
    pthread_mutex_unlock(&(station->mutex));
}