#include "../include/pool_list.h"
#define BUF_SIZE 257

node_pool* pool_list_init(int max_jobs)                                                 //initializing the pool list
{
    node_pool *head = calloc(1, sizeof(node_pool));                                     //allocating space for the head
    head->jobs = max_jobs;                                                              //initializing values of the struct
    head->id = -1;
    head->fdin = -1;
    head->fdout = -1;
    head->status = -1;
    head->pid = -1;
    head->submit = NULL;
    head->next = NULL;
    head->head_job = NULL;
    return head;
}

node_pool* new_pool_node(int id, int fdin, int fdout, char* submit, pid_t pid)          //creating a new pool node
{
    node_pool *temp = malloc(sizeof(node_pool));                                        //allocating space for the node
    temp->jobs = 0;                                                                     //initializing values of the struct
    temp->id = id;
    temp->fdin = fdin;
    temp->fdout = fdout;
    temp->status = 1;
    temp->pid = pid;
    temp->submit = calloc((strlen(submit) + 1) , sizeof(char));                         //allocating space and storing the first submit command that will be forwarded to the pool
    strncpy(temp->submit, submit,strlen(submit));
    temp->head_job = job_list_init();                                                   //initializing the job list of the pool
    temp->next = NULL;
    return temp;
}

void insert(node_pool *head, int id, int fdin, int fdout, char* submit, pid_t pid)
{
    node_pool *current = new_pool_node(id, fdin, fdout, submit, pid);                   //creating the new node
    node_pool *temp = head->next;                                                       //and inserting it in the right position of the list
    head->next = current;
    current->next = temp;
}

int check_pool(node_pool *head)
{
    int max_jobs = head->jobs;                                                          //iterate all the nodes to find the right pool for the job
    node_pool *temp = head->next;
    while(temp != NULL)
    {
        if(temp->jobs < max_jobs)                                                       //checking if there is enough space for a new job
        {
            return temp->id;
        }
        temp = temp->next;
    }
    return -1;
}

int fd_pool(node_pool *head, int id)
{
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes to find the fdout file descriptor of the pool
    {
        if(temp->id == id)
        {
            return temp->fdout;
        }
        temp = temp->next;
    }
    return -1;
}

void deallocate(node_pool *head)
{
    node_pool *temp;
    while(head != NULL)                                                                 //deallocate all the memory used for the list
    {
        temp = head;
        head = head->next;
        deallocate_job_list(temp->head_job);
        free(temp);
    }
}

void status(node_pool *head, int job_id, int fdout)
{
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if(job_status(temp->head_job, job_id, fdout))                                   //pass the flow to the job_status function along the fdout file descriptor of the coord (in order to print the status of the job)
            break;
        temp = temp->next;
    }
}

void status_all(node_pool *head, int fdout, int time_duration)
{
    int num_of_printed = 0;
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        num_of_printed = job_status_all(temp->head_job, fdout, num_of_printed, time_duration);             //pass the flow to the status_all function along the fdout file descriptor of the coord (in order to print the status of the job(s))
        temp = temp->next;
    }
}

void show_active(node_pool *head, int fdout)
{
    int num_of_printed = 0;
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        num_of_printed = show_active_jobs(temp->head_job, fdout, num_of_printed);       //pass the flow to the show_active_jobs function along the fdout file descriptor of the coord (in order to print the active job(s))
        temp = temp->next;
    }
    if(num_of_printed == 0)
    {
        write(fdout, "None", BUF_SIZE);
    }
}

void active(node_pool *head, int fdout)
{
    int counter = 0;
    char *buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf, '\0', BUF_SIZE);
    node_pool *temp = head->next;
    write(fdout, "Pool & NumOfJobs:", BUF_SIZE);
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if(temp->status == 1)                                                           //print the active pools and the number of active jobs
        {
            snprintf(buf, BUF_SIZE - 1, "%d.   %ld  %d", ++counter, (long)temp->pid, active_jobs(temp->head_job));
            write(fdout, buf, BUF_SIZE);
        }
        temp = temp->next;
    }

    if(counter == 0)
    {
        write(fdout, "None", BUF_SIZE);
    }
    free(buf);
}

void show_finished(node_pool *head, int fdout)
{
    int num_of_printed = 0;
    node_pool *temp = head->next;
    write(fdout, "Finished jobs:", BUF_SIZE);
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        num_of_printed = show_finished_jobs(temp->head_job, fdout, num_of_printed);     //pass the flow to the show_finished_jobs function along the fdout file descriptor of the coord (in order to print the finished job(s))
        temp = temp->next;
    }
    if(num_of_printed == 0)
    {
        write(fdout, "None", BUF_SIZE);
    }
}

int pool_of_job(node_pool *head, int job_id)
{
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if(inside_pool(temp->head_job, job_id))                                         //pass the flow to the inside_pool function (in order to find the pool of the job)
            return temp->id;
        temp = temp->next;
    }
    return -1;
}

pid_t job_pid_val(node_pool *head, int job_id)
{
    node_pool *temp = head->next;
    pid_t pid = -1;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if((pid = find_job_pid(temp->head_job, job_id)) > 0)                            //pass the flow to the find_job_pid function (in order to find the pid of the job)
            return pid;
        temp = temp->next;
    }
    return -1;
}

int job_status_val(node_pool *head, int job_id)
{
    node_pool *temp = head->next;
    int status = -1;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if((status = status_val(temp->head_job, job_id)) >= 0)                          //pass the flow to the status-val function (in order to find the status of the job)
            return status;
        temp = temp->next;
    }
    return -1;
}

void shutdown(node_pool *head)
{
    node_pool *temp = head->next;
    while(temp != NULL)                                                                 //iterate all the nodes
    {
        if(temp->status == 1)                                                           //if the pool is active
        {
            if(temp->pid)
            {
                if(kill(temp->pid, SIGTERM) != 0)                                       //send a SIGTERM signal to the pool
                    printf("kill termination error\n");
            }
        }
        temp = temp->next;
    }
}
