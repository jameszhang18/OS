#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
    int i;
    for (i = 0; i < 4; i++) {
        //Initialize the quardrant locks
        isection.quad[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        //Initialize lane locks
        isection.lanes[i].lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        //Initialize the condition variables
        isection.lanes[i].producer_cv=(pthread_cond_t)PTHREAD_COND_INITIALIZER;
        isection.lanes[i].consumer_cv=(pthread_cond_t)PTHREAD_COND_INITIALIZER;

        isection.lanes[i].in_cars = NULL;
        isection.lanes[i].out_cars = NULL;
        isection.lanes[i].inc = 0;
        isection.lanes[i].passed = 0;

        isection.lanes[i].buffer = malloc(sizeof(struct car*)*LANE_LENGTH);

        if (isection.lanes[i].buffer == NULL){
            exit(-1);
        }
        isection.lanes[i].capacity = LANE_LENGTH;
        isection.lanes[i].head = 0;
        isection.lanes[i].tail = 0;
        isection.lanes[i].in_buf = 0;
    }
}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    struct lane *l = arg;

    while (l->in_cars){
        pthread_mutex_lock(&l->lock);
        while (l->in_buf == l->capacity){
            pthread_cond_wait(&l->consumer_cv,&l->lock);
        }

        //acquire this_car from in_cars
        struct car *this_car;
        this_car = l->in_cars;

        //add this_car into the buffer at the tail
        l->buffer[l->tail] = this_car;

        //update the head of in_cars
        l->in_cars = this_car->next;
        this_car->next = NULL;
        
        //update all variables
        l->in_buf++;
        l->tail = (l->tail+1)%LANE_LENGTH;

        //signal to car_cross
        pthread_cond_signal(&l->producer_cv);
        pthread_mutex_unlock(&l->lock);
    }

    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
    struct lane *l = arg;
    int i;
    int *path;
    //destination lane
    struct lane *l_out;

    while (1){
        pthread_mutex_lock(&l->lock);
        if (l->inc == 0){
            pthread_mutex_unlock(&l->lock);
            return NULL;
        }
        while (l->in_buf == 0){
            pthread_cond_wait(&l->producer_cv,&l->lock);
        }

        //aquire this_car from the head of buffer and update buffer
        struct car *this_car;
        this_car = l->buffer[l->head];
        this_car->next = NULL;

        path = compute_path(this_car->in_dir,this_car->out_dir);
        if (path == NULL){
            pthread_mutex_unlock(&l->lock);
            return NULL;
        }

        //lock the quads on the path
        for (i = 0; i < 4; i++){
            if (path[i] != -1){
                pthread_mutex_lock(&isection.quad[path[i]-1]);
            }
        }

        //update all variables of l
        l->passed++;
        l->inc --;
        l->in_buf--;
        l->head = (l->head+1)%l->capacity;
        
        //unlock the quads on the path
        for (i = 0 ; i < 4 ; i++) {
            if (path[i]!=-1) {
                pthread_mutex_unlock(&isection.quad[path[i]-1]);
            }
        }

        free(path);

        //signal to car_arrive
        pthread_cond_signal(&l->consumer_cv);
        
        //update variables in the destination lane
        if (this_car->in_dir != this_car->out_dir){
            //not a u turn
            pthread_mutex_unlock(&l->lock);
            l_out = &isection.lanes[this_car->out_dir];
            pthread_mutex_lock(&l_out->lock);
            this_car->next = l_out->out_cars;
            l_out->out_cars = this_car;
            pthread_mutex_unlock(&l_out->lock);            
        }
        else {
            //u turn
            this_car->next = l->out_cars;
            l->out_cars = this_car;
            pthread_mutex_unlock(&l->lock);
        }

    }

    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int* path = (int*)malloc(sizeof(int) * 4);
    //initialize elements in path
	path[0]=1; path[1]=2; path[2]=3; path[3]=4;
    //check if path is created successfully
    if (path == NULL){
        return NULL;
    }
    //U turn
    if (in_dir == out_dir) {
    	return path;
    }
    else if ((in_dir+1)%4 == out_dir) {
    	path[0]=out_dir+1;
    	path[1]=-1; path[2]=-1; path[3]=-1;
    	return path;
    }
    else if ((in_dir+2)%4 == out_dir) {
    	path[in_dir]=-1;
    	path[(in_dir-1)%4]=-1;
    	return path;
    }
    else if ((in_dir+3)%4 == out_dir) {
    	path[in_dir]=-1;
    	return path;
    }

    return NULL;



}