#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
	/* allocate memory for task */
	os_task_t *task = (os_task_t *)malloc(sizeof(os_task_t));

	if (task == NULL) {
		printf("[ERROR] Not enought memory\n");
		return NULL;
	}

	/* set structure arguments */
	task->argument = arg;
	task->task = f;

	return task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
	/* create new node */
	os_task_queue_t *newNode = (os_task_queue_t *)malloc(sizeof(os_task_queue_t));

	if (newNode == NULL) {
		printf("[ERROR] Not enought memory\n");
		return;
	}

	newNode->task = t;

	/* use lock to add one task at a time */
	pthread_mutex_lock(&tp->taskLock);
	/* adding task in first position to avoid traversing the whole list */
	os_task_queue_t *prevFirst = tp->tasks;

	tp->tasks = newNode;
	newNode->next = prevFirst;
	pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
	/* modify queue head */
	os_task_queue_t *head = tp->tasks;
	os_task_t *task = head->task;

	tp->tasks = head->next;
	/* free queue node */
	free(head);

	return task;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
	os_threadpool_t *threadpool = (os_threadpool_t *)malloc(sizeof(os_threadpool_t));

	/* set structure params */
	threadpool->tasks = NULL;
	threadpool->num_threads = nThreads;
	threadpool->should_stop = 0;

	pthread_mutex_t mutex;

	pthread_mutex_init(&mutex, NULL);
	threadpool->taskLock = mutex;

	/* start n Threads */
	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);

	threadpool->threads = threads;

	for (int i = 0; i < nThreads; i++) {
		/* each thread will run the loop function */
		int res = pthread_create(&threads[i], NULL, thread_loop_function, (void *)threadpool);
		/* check create result */
		if (res) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	return threadpool;
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
	os_threadpool_t *tp = (os_threadpool_t *)args;

	/* loop in function while it should not stop or there are still task to be proccessed */
	while (!tp->should_stop) {
		os_task_t *task = NULL;

		/* check for new tasks */
		pthread_mutex_lock(&tp->taskLock);
		if (tp->tasks != NULL) {
			/* extract task */
			task = get_task(tp);
		}
		pthread_mutex_unlock(&tp->taskLock);

		if (task != NULL)
			task->task(task->argument);
	}

	pthread_exit(NULL);
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
	/* busy waiting untill proccesing is done */
	while (!processingIsDone(tp));

	tp->should_stop = 1;

	/* join threads */
	for (int i = 0; i < tp->num_threads; i++) {
		int r = pthread_join(tp->threads[i], NULL);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}
}
