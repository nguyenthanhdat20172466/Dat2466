#include <pthread.h>
#include <stdbool.h>

struct station {
    int free_seats; // number of free seats in the train, -if one exists at station-.
    int passengers_waiting; // number of passengers waiting at station
    int passengers_leaving; // number of passengers who have found a free seat
    pthread_mutex_t mutex;
    pthread_cond_t free_seats_available; // train arrived with free seats
    pthread_cond_t passengers_on_board; // all leaving passengers have been seated
};

/*
 * This function will be invoked to initialize the station object
 */
void station_init(struct station *station);

/*
 * This function must not return until the train is satisfactorily loaded
 * (all passengers are in their seats, and either the train is full or all wait-ing passengers have boarded).
 * "count" indicates how many seats are available on the train.
 */
void station_load_train(struct station *station, int count);

/*
 * This function must not return until a train is in the station
 * (i.e., a call to station load train is in progress) and there are enough free seats on the train
 * for this passenger to sit down. Once this function returns, the passenger robot will move the
 * passenger on board the train and into a seat.
 */
void station_wait_for_train(struct station *station);

/*
 * This function is to let the train know that it’s on board.
 */
void station_on_board(struct station *station);