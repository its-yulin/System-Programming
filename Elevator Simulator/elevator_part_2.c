#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "elevator.h"

typedef struct {
	Dllist wait;
	pthread_cond_t *cond;
} Global_waiting;

typedef struct {
	Dllist load;
	Dllist unload;
	int travel_dir;
} E_metadata;


void initialize_simulation(Elevator_Simulation *es)
{
	Global_waiting *vars = (Global_waiting *)malloc(sizeof(Global_waiting));
	vars->wait = new_dllist();
	vars->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);
}

void initialize_elevator(Elevator *e)
{
	E_metadata *vars = (E_metadata *)malloc(sizeof(E_metadata));
	vars->load = new_dllist();
	vars->unload = new_dllist();
	vars->travel_dir = 1;
	e->v = (void *)(vars);
}
void initialize_person(Person *p)
{
    p->v = malloc(sizeof(int));
    *(int*)p->v = (p->to - p->from > 0)? 1: -1;
}

void wait_for_elevator(Person *p)
{	
	Dllist wait = ((Global_waiting *)(p->es->v))->wait;
	pthread_cond_t *cond = ((Global_waiting *)(p->es->v))->cond;
	pthread_mutex_lock(p->es->lock);
	dll_append(wait, new_jval_v(p));
	pthread_cond_signal(cond);
	pthread_mutex_unlock(p->es->lock);
	pthread_mutex_lock(p->lock);
	while(p->e == NULL) {
		pthread_cond_wait(p->cond, p->lock);
	}
	pthread_mutex_unlock(p->lock);
}

void wait_to_get_off_elevator(Person *p)
{
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);
	pthread_mutex_lock(p->lock);
	while(p->e->onfloor != p->to) {
		pthread_cond_wait(p->cond, p->lock);
	}
	pthread_mutex_unlock(p->lock);
}

void person_done(Person *p)
{
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);
}

int change_direction(Elevator* e) {
    int temp = ((E_metadata *)(e->v))->travel_dir;
	if(e->onfloor + temp > e->es->nfloors || e->onfloor + temp < 1) {
		pthread_mutex_lock(e->lock);
		temp *= -1;
		((E_metadata *)(e->v))->travel_dir = temp;
		pthread_mutex_unlock(e->lock);
	}
    return temp;
}

void signal_leave(Dllist unload, Elevator* e) {
    Dllist itr;
    Person* p;
	dll_traverse(itr, unload) {
		p = (Person *)jval_v(dll_val(itr));
		pthread_mutex_lock(p->lock);
		pthread_cond_signal(p->cond);
		pthread_mutex_unlock(p->lock);
	}
}

void signal_load(Dllist load, Elevator* e) {
    Dllist itr;
    Person* p;
    dll_traverse(itr, load) {
		p = (Person *)jval_v(dll_val(itr));
		pthread_mutex_lock(p->lock);
		p->e = e;
		pthread_cond_signal(p->cond);
		pthread_mutex_unlock(p->lock);
	}    
}

void *elevator(void *arg)
{
	Elevator *e;
	e = (Elevator *)(arg);

	while(1) {
		Dllist wait = ((Global_waiting *)(e->es->v))->wait;
		pthread_cond_t *cond = ((Global_waiting *)(e->es->v))->cond;
		Dllist load = ((E_metadata *)(e->v))->load;
		Dllist unload = ((E_metadata *)(e->v))->unload;
		int e_travel_dir = change_direction(e);
		pthread_mutex_lock(e->es->lock);

		while(dll_empty(wait)) {
			pthread_cond_wait(cond, e->es->lock);
		}
		
		Dllist itr;
		Person *p;
		int p_travel_dir; 
		dll_traverse(itr, wait) {
			p = (Person *)jval_v(dll_val(itr));
			p_travel_dir = *(int*) p->v;
			if(e->onfloor == p->from && e_travel_dir == p_travel_dir) {
				pthread_mutex_lock(e->lock);
				dll_append(load, new_jval_v(p));
				pthread_mutex_unlock(e->lock);
				itr = itr->blink;
				dll_delete_node(itr->flink);
			}
		}
		pthread_mutex_unlock(e->es->lock);
		pthread_mutex_lock(e->lock);
		dll_traverse(itr, e->people) {
			p = (Person *)jval_v(dll_val(itr));
			if(e->onfloor == p->to) {
				dll_append(unload, new_jval_v(p));
			}
		}
		pthread_mutex_unlock(e->lock);

		if((!dll_empty(load) || !dll_empty(unload)) && e->door_open == 0) {
			open_door(e);
		}

        signal_leave(unload, e);
		dll_traverse(itr, unload) {
			p = (Person *)jval_v(dll_val(itr));
			pthread_mutex_lock(e->lock);
			while(p->ptr != NULL) {
				pthread_cond_wait(e->cond, e->lock);
			}
			pthread_mutex_unlock(e->lock);
			itr = itr->blink;
			dll_delete_node(itr->flink);
		}
		signal_load(load, e);
		dll_traverse(itr, load) {
			p = (Person *)jval_v(dll_val(itr));
			pthread_mutex_lock(e->lock);
			while(p->ptr == NULL) {
				pthread_cond_wait(e->cond, e->lock);
			}
			pthread_mutex_unlock(e->lock);
			itr = itr->blink;
			dll_delete_node(itr->flink);
		}
		if(e->door_open == 1) {
			close_door(e);
		}
		move_to_floor(e, e->onfloor + e_travel_dir);	
	}
}