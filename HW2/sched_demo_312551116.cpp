#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <sched.h>
using namespace std;

pthread_barrier_t barrier;

typedef struct{
	pthread_t thread_id;
	int thread_num;
	int sched_policy;
	int sched_priority;
	int time_wait;
}thread_info_t;

void *thread_func(void *arg){
	thread_info_t *thread_info = (thread_info_t *)arg;
	int thread_id = thread_info -> thread_num;

	pthread_barrier_wait(&barrier);
	for(int i = 0;i < 3;i++){
		printf("Thread %d is running\n", thread_id);
	}

	time_t time_start = time(NULL);
	while(time(NULL) - time_start < thread_info -> time_wait){
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	int num_threads = 0;
	float time_wait = 0;
	char *policies_str = NULL;
	char *priorities_str = NULL;
	
	int opt;
	while((opt = getopt(argc, argv, "n:t:s:p:")) != -1){
		switch(opt){
			case 'n':
				num_threads = atoi(optarg);
				break;
			case 't':
				time_wait = atoi(optarg);
				break;
			case 's':
				policies_str = optarg;
				break;
			case 'p':
				priorities_str = optarg;
				break;
			default:
				//exit(EXIT_FAILURE);
				cout << endl;
		}
	}
	
	char *token = strtok(priorities_str, ",");
	int priorities[num_threads];
	for(int i = 0;i < num_threads;i++){
		priorities[i] = atoi(token);
		token = strtok(NULL, ",");
	}

	token = strtok(policies_str, ",");
	int policies[num_threads];
	for(int i = 0;i < num_threads;i++){
		if(token[0] == 'F'){
			policies[i] = 1;
		}
		else{
			policies[i] = 0;
		}
		token = strtok(NULL, ",");
	}
	
	pthread_t threads[num_threads];
	pthread_attr_t thread_attrs[num_threads];
	thread_info_t thread_infos[num_threads];
	pthread_barrier_init(&barrier, NULL, num_threads);

	for(int i = 0;i < num_threads;i++){
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(i, &cpuset);
		pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);

		thread_infos[i].thread_num = i;
		thread_infos[i].sched_policy = policies[i];
		thread_infos[i].sched_priority = priorities[i];
		thread_infos[i].time_wait = time_wait;

		pthread_attr_init(&thread_attrs[i]);
		if(thread_infos[i].sched_policy == 0){
			pthread_attr_setschedpolicy(&thread_attrs[i], SCHED_OTHER);
		}
		else{
			pthread_attr_setschedpolicy(&thread_attrs[i], SCHED_FIFO);
		}
		struct sched_param param;
		param.sched_priority = thread_infos[i].sched_priority;
		pthread_attr_setschedparam(&thread_attrs[i], &param);
	}
	for(int i = 0;i < num_threads;i++){
		pthread_create(&threads[i], &thread_attrs[i], thread_func, (void *)&thread_infos[i]);
	}

	for(int i = 0;i < num_threads;i++){
		pthread_join(threads[i], NULL);
	}

	pthread_barrier_destroy(&barrier);
	
	return 0;
}
