#include "../include/job_list.h"
#define BUF_SIZE 257

node_job* job_list_init()                                               //initializing the job list
{
    node_job *head = malloc(sizeof(node_job));                          //allocating space for the head
    head->next = NULL;                                                  //initializing values of the struct
    head->pid = -1;
    head->status = -1;
    return head;
}

node_job* new_job_node(pid_t pid, int id)                               //creating a new job node
{
    node_job *temp = malloc(sizeof(node_job));                          //allocating space for the node
    temp->status = 1;                                                   //initializing values of the struct
    temp->next = NULL;
    temp->pid = pid;
    temp->id = id;
    temp->duration = 0;
    temp->last_time_active = time(NULL);                                //saving the time in which the job started
    temp->first_time_active = temp->last_time_active;
    return temp;
}

void insert_job(node_job *head, pid_t pid, int id)
{
    node_job *current = new_job_node(pid, id);                          //creating the new node
    node_job *temp = head->next;                                        //and inserting it in the right position of the list
    head->next = current;
    current->next = temp;
}

void deallocate_job_list(node_job *head)
{
    node_job *temp;                                                     //deallocate all the memory used for the list
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void change_status(node_job *head, pid_t pid, int status)
{
    node_job *temp = head->next;
    while(temp != NULL)                                                 //iterate all the nodes
    {
        if(temp->pid == pid)                                            //checks if we found the job node we are looking for
        {
            if((temp->status == 1) && (status == 2))                    //if we are going to suspend the job we calculate the time that it was active and we store it
                temp->duration += (time(NULL) - temp->last_time_active);
            else if(temp->status == 2 && status == 1)                   //if we are going to resume the job we update the last_time_active with the current time
                temp->last_time_active = time(NULL);
            temp->status = status;
            break;
        }
        temp = temp->next;
    }
}

int job_status(node_job *head, int id, int fdout)
{
    char *buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf,'\0', BUF_SIZE);
    node_job *temp = head->next;
    while(temp != NULL)                                                 //iterate all the nodes
    {
        if(temp->id == id)                                              //checks if we found the job node we are looking for
        {
            if(temp->status == 0)                                       //and then we print the right message to the console
                snprintf(buf, BUF_SIZE - 1, "JobID %d Status:  Finished",temp->id);
            else if(temp->status == 1)
                snprintf(buf, BUF_SIZE - 1, "JobID %d Status:  Active (running for %lld seconds)", temp->id, (long long)(temp->duration + (time(NULL) - temp->last_time_active)));
            else if(temp->status == 2)
                snprintf(buf, BUF_SIZE - 1, "JobID %d Status:  Suspended",temp->id);
            else
                snprintf(buf, BUF_SIZE - 1, "Error while acquiring status");
            write(fdout, buf, BUF_SIZE);
            free(buf);
            return 1;
        }
        temp = temp->next;
    }
    free(buf);
    return 0;
}

int job_status_all(node_job *head, int fdout, int num_of_printed, int time_duration)
{
    char *buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf,'\0', BUF_SIZE);
    node_job *temp = head->next;
    while(temp != NULL)                                                 //iterate all the nodes
    {                                                                   //checks if the job satisfies the time limit (if there is any)
        if((time_duration == -1) || (time_duration >= (time(NULL) - temp->first_time_active)))
        {
            num_of_printed++;
            if(temp->status == 0)                                       //and then we print the right message to the console
                snprintf(buf, BUF_SIZE - 1, "%d.   JobID %d Status:  Finished", num_of_printed, temp->id);
            else if(temp->status == 1)
                snprintf(buf, BUF_SIZE - 1, "%d.   JobID %d Status:  Active (running for %lld seconds)", num_of_printed, temp->id,  (long long)(temp->duration + (time(NULL) - temp->last_time_active)));
            else if(temp->status == 2)
                snprintf(buf, BUF_SIZE - 1, "%d.   JobID %d Status:  Suspended", num_of_printed, temp->id);
            else
                snprintf(buf, BUF_SIZE - 1, "Error while acquiring status");
            write(fdout, buf, BUF_SIZE);
        }
        temp = temp->next;
    }
    free(buf);
    return num_of_printed;
}

int show_active_jobs(node_job *head, int fdout, int num_of_printed)
{
    char *buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf,'\0', BUF_SIZE);
    node_job *temp = head->next;
    while(temp != NULL)                                                 //iterate all the nodes
    {
        if(temp->status == 1)                                           //checks if the job is active
        {                                                               //and then we print the right message to the console
            snprintf(buf, BUF_SIZE, "%d.   JobID %d", ++num_of_printed, temp->id);
            write(fdout, buf, BUF_SIZE);
        }
        temp = temp->next;
    }
    free(buf);
    return num_of_printed;
}

int active_jobs(node_job *head)
{
    int count = 0;
    node_job *temp = head->next;
    while(temp != NULL)                                                 //iterate all the nodes
    {
        if(temp->status == 1)                                           //checks if the job is active
        {
            count++;                                                    //and updates the counter
        }
        temp = temp->next;
    }
    return count;
}

int show_finished_jobs(node_job *head, int fdout, int num_of_printed)
{
    char *buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf,'\0', BUF_SIZE);
    node_job *temp = head->next;
    while(temp != NULL)                                             //iterate all the nodes
    {
        if(temp->status == 0)                                       //checks if the job is finished
        {                                                           //and then we print the right message to the console
            snprintf(buf, BUF_SIZE - 1, "%d.   JobID %d", ++num_of_printed, temp->id);
            write(fdout, buf, BUF_SIZE);
        }
        temp = temp->next;
    }
    free(buf);
    return num_of_printed;
}

int inside_pool(node_job *head, int id)
{
    node_job *temp = head->next;
    while(temp != NULL)                                             //iterate all the nodes
    {
        if(temp->id == id)                                          //checks if the job is the one we are looking for
        {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

pid_t find_job_pid(node_job *head, int id)
{
    node_job *temp = head->next;
    while(temp != NULL)                                             //iterate all the nodes
    {
        if(temp->id == id)
        {
            return temp->pid;                                       //checks if the job is the one we are looking and only then it returns the pid of the job
        }
        temp = temp->next;
    }
    return -1;
}

int status_val(node_job *head, int id)
{
    node_job *temp = head->next;
    while(temp != NULL)                                             //iterate all the nodes
    {
        if(temp->id == id)
        {
            return temp->status;                                    //checks if the job is the one we are looking and only then it returns the status of the job
        }
        temp = temp->next;
    }
    return -1;
}
