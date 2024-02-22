#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
os_graph_t *graph;
os_threadpool_t *tp;
pthread_mutex_t sumLock;
pthread_mutex_t visitedLock;

void prosses_node(void *arg)
{
	os_node_t *node = (os_node_t *)arg;

	/* check and set visited status */
	pthread_mutex_lock(&visitedLock);
	if (graph->visited[node->nodeID]) {
		pthread_mutex_unlock(&visitedLock);
		return;
	}

	graph->visited[node->nodeID] = 1;
	pthread_mutex_unlock(&visitedLock);

	/* add to sum */
	pthread_mutex_lock(&sumLock);
	sum += node->nodeInfo;
	pthread_mutex_unlock(&sumLock);

	/* add a task for each unvisited neighbour */
	for (int i = 0; i < node->cNeighbours; i++) {
		pthread_mutex_lock(&visitedLock);
		if (!graph->visited[node->neighbours[i]])
			add_task_in_queue(tp, task_create(graph->nodes[node->neighbours[i]], &prosses_node));
		pthread_mutex_unlock(&visitedLock);
	}
}

int processingIsDone(os_threadpool_t *tp)
{
	if (tp->tasks != NULL)
		return 0;

	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: ./main input_file\n");
		exit(1);
	}

	FILE *input_file = fopen(argv[1], "r");

	if (input_file == NULL) {
		printf("[Error] Can't open file\n");
		return -1;
	}

	graph = create_graph_from_file(input_file);
	if (graph == NULL) {
		printf("[Error] Can't read the graph from file\n");
		return -1;
	}

	// TODO: create thread pool and traverse the graf
	tp = threadpool_create(MAX_TASK, MAX_THREAD);
	pthread_mutex_init(&sumLock, NULL);
	pthread_mutex_init(&visitedLock, NULL);

	for (int i = 0; i < graph->nCount; i++) {
		pthread_mutex_lock(&visitedLock);
		if (!graph->visited[i])
			add_task_in_queue(tp, task_create(graph->nodes[i], &prosses_node));

		pthread_mutex_unlock(&visitedLock);
	}

	threadpool_stop(tp, &processingIsDone);
	pthread_mutex_destroy(&visitedLock);
	pthread_mutex_destroy(&sumLock);

	printf("%d", sum);
	return 0;
}
