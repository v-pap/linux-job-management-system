#ifndef _JOB_LIST_H
#define _JOB_LIST_H
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

typedef struct node_job node_job;

struct node_job
{
    node_job *next;
    int status;
    pid_t pid;
    int id;
    time_t duration;
    time_t last_time_active;
    time_t first_time_active;
};

node_job* job_list_init();
node_job* new_job_node(pid_t pid, int id);
void insert_job(node_job *head, pid_t pid, int id);
void deallocate_job_list(node_job *head);
void change_status(node_job *head, pid_t pid, int status);
int job_status(node_job *head, int id, int fdout);
int job_status_all(node_job *head, int fdout, int num_of_printed, int time_duration);
int show_active_jobs(node_job *head, int fdout, int num_of_printed);
int active_jobs(node_job *head);
int show_finished_jobs(node_job *head, int fdout, int num_of_printed);
int inside_pool(node_job *head, int id);
pid_t find_job_pid(node_job *head, int id);
int status_val(node_job *head, int id);
#endif