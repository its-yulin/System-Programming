#include <stdio.h>
#include <pthread.h>
#include "elevator.h"
#include "dllist.h"

Dllist global;

void initialize_simulation(Elevator_Simulation *es)
{
    global = new_dllist();
    es->v = global;
}

void initialize_elevator(Elevator *e)
{

}

void initialize_person(Person *e)
{

}

void wait_for_elevator(Person *p)
{
    pthread_mutex_lock(p->es->lock);
    global = (Dllist) p->es->v;
    dll_append(global, new_jval_v((void *) p));
    pthread_mutex_unlock(p->es->lock);
    pthread_mutex_lock(p->lock);
    pthread_cond_wait(p->cond, p->lock);
    pthread_mutex_unlock(p->lock);
}

void wait_to_get_off_elevator(Person *p)
{
    pthread_mutex_lock(p->lock);
    pthread_cond_signal(p->e->cond);
    pthread_cond_wait(p->cond, p->lock);
    pthread_mutex_unlock(p->lock);
}

void person_done(Person *p)
{
    pthread_mutex_lock(p->lock);
    pthread_cond_signal(p->e->cond);
    pthread_mutex_unlock(p->lock); 
}

void *elevator(void *arg)
{
    Elevator *e = (Elevator *)arg;
    Person *p = NULL;
    while(1){
        pthread_mutex_lock(e->es->lock);
        global = (Dllist) e->es->v;
        if(!dll_empty(global)){
            Dllist first_person = dll_first(global);
            p = (Person*) jval_v(dll_val(first_person));
            dll_delete_node(first_person);
        }
        pthread_mutex_unlock(e->es->lock);
        if(p == NULL){
            continue;
        }
        if(p->from != e->onfloor){
            if(e->door_open){
                close_door(e);
            }
            move_to_floor(e, p->from);
        }
        if(!e->door_open){
            open_door(e);
        }
        p->e = e;
        pthread_mutex_lock(e->lock);
        pthread_cond_signal(p->cond);
        pthread_mutex_unlock(e->lock);
        pthread_mutex_lock(e->lock);
        while(dll_empty(e->people)) {
            pthread_cond_wait(e->cond, e->lock);
        }
        pthread_mutex_unlock(e->lock);
        close_door(e);
        move_to_floor(e, p->to);
        open_door(e);
        pthread_mutex_lock(e->lock);
        pthread_cond_signal(p->cond);
        pthread_mutex_unlock(e->lock);
        pthread_mutex_lock(e->lock);
        while(!dll_empty(e->people)) {
            pthread_cond_wait(e->cond, e->lock);
        }
        pthread_mutex_unlock(e->lock);
        close_door(e);
    }
    return NULL;
}