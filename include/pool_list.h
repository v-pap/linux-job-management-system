#ifndef _POOL_LIST_H
#define _POOL_LIST_H
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "job_list.h"

typedef struct node_pool node_pool;

struct node_pool
{
    node_pool *next;
    int jobs;
    int id;
    int fdin;
    int fdout;
    int status;
    pid_t pid;
    char *submit;
    node_job *head_job;
};

node_pool* pool_list_init(int max_jobs);
node_pool* new_pool_node(int id, int fdin, int fdout, char* submit, pid_t pid);
void insert(node_pool *head, int id, int fdin, int fdout, char* submit, pid_t pid);
int check_pool(node_pool *head);
int fd_pool(node_pool *head, int id);
void deallocate(node_pool *head);
void status(node_pool *head, int job_id, int fdout);
void status_all(node_pool *head, int fdout, int time_duration);
void show_active(node_pool *head, int fdout);
void active(node_pool *head, int fdout);
void show_finished(node_pool *head, int fdout);
int pool_of_job(node_pool *head, int job_id);
pid_t job_pid_val(node_pool *head, int job_id);
int job_status_val(node_pool *head, int job_id);
void shutdown(node_pool *head);
#endif