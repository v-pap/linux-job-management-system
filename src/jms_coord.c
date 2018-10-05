#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "../include/pool_list.h"
#define BUF_SIZE 257                                                            //defining buffer size (for read write and sprintf)

int main(int argc, char *argv[])
{
    int max_jobs = 5,                                                           //variables used in the program
    fdin,
    fdout,
    flag_console = 0,
    flag_shutdown = 0,
    id_of_pool,
    num_of_pools = 0,
    fd_pool_in,
    fd_pool_out,
    num_of_jobs = 0,
    job_id,
    active_jobs = 0,
    active_pools = 0,
    loop = 1;
    pid_t pid,
    job_pid;
    char *jms_in = NULL,
    *jms_out = NULL,
    *path = NULL,
    *buf = NULL,
    *buf2 = NULL,
    *token = NULL,
    *max_jobs_string = NULL;
    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i],"-w") == 0 && i+1 < argc)                             //argument for the input fifo file
        {
            jms_in = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(jms_in, argv[i+1]);
        }
        else if(strcmp(argv[i],"-r") == 0 && i+1 < argc)                        //argument for the output fifo file
        {
            jms_out = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(jms_out, argv[i+1]);
        }
        else if(strcmp(argv[i],"-l") == 0 && i+1 < argc)                        //argument for the path of the programs outputs (if any)
        {
            path = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(path, argv[i+1]);
            mkdir(path, 0777);
        }
        else if(strcmp(argv[i],"-n") == 0 && i+1 < argc)                        //argument for the maximum number of jobs that each pool controls
        {
            max_jobs_string = malloc((strlen(argv[i+1]) + 1) * sizeof(char));
            strcpy(max_jobs_string, argv[i+1]);
            max_jobs = atoi(argv[i+1]);
        }
    }

    if(mkfifo(jms_in, 0666) == -1)                                              //creating the fifos for input and output
    {
		if(errno != EEXIST)
        {
            perror("mkfifo jms_in error");
            exit(6);
        }
    }

    if(mkfifo(jms_out, 0666) == -1)
    {
		if(errno != EEXIST)
        {
            perror("mkfifo jms_out error");
            exit(6);
        }
    }

	if ((fdin = open(jms_in, O_RDONLY | O_NONBLOCK)) < 0)                       //opening the input fifo
    {
		perror("fifo jms_in open problem"); exit(3);
	}

    buf = malloc(BUF_SIZE * sizeof(char));                                      //allocating space for the buffers and setting them to '\0'
    buf2 = malloc(BUF_SIZE * sizeof(char));
    memset(buf, '\0', BUF_SIZE);
    memset(buf2, '\0', BUF_SIZE);

    node_pool *head = pool_list_init(max_jobs);                                 //initializing the list of pools
    char *pool_in = malloc(sizeof(char) * BUF_SIZE);
    char *pool_out = malloc(sizeof(char) * BUF_SIZE);
    memset(pool_in, '\0', BUF_SIZE);
    memset(pool_out, '\0', BUF_SIZE);

    printf("ready\n");
    fflush(stdout);

    while(loop)                                                                 //main loop of the program
    {
        if(read(fdin, buf, BUF_SIZE ) > 0)                                      //reading from the console
        {
            if (flag_console == 0)                                              //in case this is the first time we read from the console
            {
                if ((fdout = open(jms_out, O_WRONLY | O_NONBLOCK)) < 0)         //we open the fifo in order to write to the console
                {
                    perror("fifo jms_out open problem"); exit(3);
                }
                flag_console = 1;
            }
            buf[strcspn(buf, "\n")] = 0;                                        //removes the new line character and replaces it with terminating character
            strcpy(buf2, buf);                                                  //copy string in order to be intact after strtok
            token = strtok(buf, " ");                                           //get the first word of the string we read from the console
            if(token == NULL)                                                   //if it is empty
            {
                write(fdout, "OK", BUF_SIZE);                                   //we send an ok signal to the console
            }
            else if(strcmp(token, "submit") == 0)                               //submit command
            {
                token = strtok(NULL, " ");                                      //strtok just iterates between the insert arguments and then we save the ones we need
                assert (token != NULL);
                id_of_pool = check_pool(head);                                  //we check which pool has enough space for the new job
                if(id_of_pool == -1)                                            //if there is not an available pool then we create a new one
                {
                    ++num_of_pools;
                    snprintf(pool_in, BUF_SIZE - 1, "pool_in_%d.fifo",num_of_pools);        //initializing the names of the fifos
                    snprintf(pool_out, BUF_SIZE - 1, "pool_out_%d.fifo",num_of_pools);
                    if(mkfifo(pool_in, 0666) == -1)                             //creating the fifos
                    {
                        if(errno != EEXIST)
                        {
                            perror("mkfifo pool_in error");
                            exit(6);
                        }
                    }

                    if(mkfifo(pool_out, 0666) == -1)
                    {
                        if(errno != EEXIST)
                        {
                            perror("mkfifo pool_out error");
                            exit(6);
                        }
                    }

                    if ((fd_pool_in = open(pool_in, O_RDONLY | O_NONBLOCK)) < 0)        //opening the fifo
                    {
                        perror("fifo fd_pool_in open problem"); exit(3);
                    }

                    if((pid = fork()) == 0)                                  //creating a new process with fork() and exec()
                    {
                        if(path == NULL)                                     //we check if there is an argument for the path
                        {
                            if(execlp("./pool", "pool", "-r", pool_in, "-w", pool_out, "-m", max_jobs_string, NULL) == -1)
                            {
                                perror("exec pool failure"); exit(3);
                            }
                        }
                        else                                                //and we use the appropriate exec()
                        {
                            if(execlp("./pool", "pool", "-r", pool_in, "-w", pool_out, "-m", max_jobs_string, "-l", path, NULL) == -1)
                            {
                                perror("exec pool failure"); exit(3);
                            }
                        }
                    }

                    fd_pool_out = -1;                                       //we insert the pool into the list and we save the first submit for future use
                    snprintf(buf, BUF_SIZE - 1, "%s %d", buf2, ++num_of_jobs);
                    insert(head, num_of_pools, fd_pool_in, fd_pool_out, buf, pid);

                }
                else                                                        //if there is an available pool we pass the submit request
                {
                    snprintf(buf, BUF_SIZE - 1, "%s %d", buf2, ++num_of_jobs);
                    write(fd_pool(head, id_of_pool), buf, BUF_SIZE);
                }
            }
            else if(strcmp(token, "status") == 0)                           //status command
            {
                token = strtok(NULL, " ");
                assert (token != NULL);
                status(head, atoi(token), fdout);                           //we give control to the status function
                write(fdout, "OK", BUF_SIZE);                               //and then we send OK to the console
            }
            else if(strcmp(token, "status-all") == 0)                       //status-all command
            {
                token = strtok(NULL, " ");
                if(token != NULL)                                           //check if there is a time argument and choose the appropriate status-all function
                {
                    status_all(head, fdout, atoi(token));
                }
                else
                {
                    status_all(head, fdout, -1);
                }
                write(fdout, "OK", BUF_SIZE);
            }
            else if(strcmp(token, "show-active") == 0)                      //show-active command
            {
                show_active(head, fdout);                                   //we give control to the show-active function
                write(fdout, "OK", BUF_SIZE);
            }
            else if(strcmp(token, "show-pools") == 0)                       //show-pools command
            {
                active(head, fdout);                                        //we give control to the show-pools function
                write(fdout, "OK", BUF_SIZE);
            }
            else if(strcmp(token, "show-finished") == 0)                    //show-finished command
            {
                show_finished(head, fdout);                                 //we give control to the show-finished function
                write(fdout, "OK", BUF_SIZE);
            }
            else if(strcmp(token, "suspend") == 0)                          //suspend command
            {
                token = strtok(NULL, " ");
                assert (token != NULL);
                job_id = atoi(token);
                id_of_pool = pool_of_job(head, job_id);                     //find the id of the pool in which the job is managed by
                if(id_of_pool == -1)                                        //error checking
                {
                    write(fdout, "No job with this id", BUF_SIZE);
                    write(fdout, "OK", BUF_SIZE);
                    continue;
                }
                int status = job_status_val(head, job_id);                  //check the status of the job
                if(status != 1)                                             //error checking
                {
                    write(fdout, "Process already suspended/terminated (or doesn't exist)", BUF_SIZE);
                    write(fdout, "OK", BUF_SIZE);
                    continue;
                }
                snprintf(buf2, BUF_SIZE - 1, "suspend %ld %d", (long)job_pid_val(head, job_id), job_id);        //prepare the command that it will send to the pool
                write(fd_pool(head, id_of_pool), buf2, BUF_SIZE);           //send the command
            }
            else if(strcmp(token, "resume") == 0)                           //resume function
            {
                token = strtok(NULL, " ");
                assert (token != NULL);
                job_id = atoi(token);
                id_of_pool = pool_of_job(head, job_id);                     //find the id of the pool in which the job is managed by
                if(id_of_pool == -1)                                        //error checking
                {
                    write(fdout, "No job with this id", BUF_SIZE);
                    write(fdout, "OK", BUF_SIZE);
                    continue;
                }
                int status = job_status_val(head, job_id);                  //check the status of the job
                if(status != 2)                                             //error checking
                {
                    write(fdout, "Process already active/terminated (or doesn't exist)", BUF_SIZE);
                    write(fdout, "OK", BUF_SIZE);
                    continue;
                }
                snprintf(buf2, BUF_SIZE - 1, "resume %ld %d", (long)job_pid_val(head, job_id), job_id);         //prepare the command that it will send to the pool
                write(fd_pool(head, id_of_pool), buf2, BUF_SIZE);           //send the command
            }
            else if(strcmp(token, "shutdown") == 0)                         //shutdown function
            {
                flag_shutdown = 1;                                          //change the shut_down flag
                shutdown(head);                                             //give control to the shutdown function
            }
            else                                                            //error checking
            {
                write(fdout, "Wrong input. Try again.", BUF_SIZE);
                write(fdout, "OK", BUF_SIZE);
            }

        }

        node_pool *temp = head->next;                                       //start with the first node of the pool list
        while(temp != NULL)                                                 //iterate all the pool nodes of the list
        {
            active_pools = 0;                                               //counter of the active pools
            if(temp->status == 1)                                           //check if the pool is active
            {
                active_pools++;
                if(temp->fdout == -1)                                       //if we have not yet established communication with the pool
                {
                    snprintf(pool_out, BUF_SIZE - 1, "pool_out_%d.fifo", temp->id);     //we check if the pool has opened the fifos
                    if ((temp->fdout = open(pool_out, O_WRONLY | O_NONBLOCK)) < 0)
                    {
                        temp = temp->next;                                  //if not we check the next node
                        continue;
                    }
                    sprintf(buf, "%s", temp->submit);                       //we send the submit command we saved before
                    write(temp->fdout, buf, BUF_SIZE);
                    free(temp->submit);                                     //we free up the space we used
                    temp->submit = NULL;
                }

                if(read(temp->fdin, buf, BUF_SIZE) > 0)                     //we read from the pool the internal commands/status updates
                {
                    strcpy(buf2, buf);
                    token = strtok(buf, " ");
                    assert(token != NULL);
                    if(strcmp(token,"ADDED") == 0)                          //ADDED command (means that the pool added a new job)
                    {
                        token = strtok(NULL, " ");
                        assert(token != NULL);
                        temp->jobs++;
                        job_pid = atoi(token);
                        token = strtok(NULL, " ");
                        assert(token != NULL);
                        job_id = atoi(token);
                        insert_job(temp->head_job, job_pid, job_id);        //insert job into the list of jobs of the specific pool
                        write(fdout, "OK", BUF_SIZE);
                    }
                    else if(strcmp(token,"EXIT") == 0)                      //EXIT command (means that a job is done)
                    {
                        token = strtok(NULL, " ");
                        change_status(temp->head_job, atoi(token), 0);      //remove the job from the list of jobs of the specific pool
                    }
                    else if(strcmp(token,"ENDED") == 0)                     //ENDED command (means that the pool has concluded)
                    {
                        temp->status = 0;
                        close(temp->fdin);                                  //we close the fifos and the file descriptors and changes the status of the pool
                        close(temp->fdout);
                        snprintf(pool_in, BUF_SIZE - 1, "pool_in_%d.fifo",temp->id);
                        snprintf(pool_out, BUF_SIZE - 1, "pool_out_%d.fifo",temp->id);
                        unlink(pool_in);
                        unlink(pool_out);
                    }
                    else if(strcmp(token,"SUSPENDED") == 0)                 //SUSPENDED command (means that a job has been suspended)
                    {
                        token = strtok(NULL, " ");
                        pid = atoi(token);
                        token = strtok(NULL, " ");
                        job_id = atoi(token);
                        change_status(temp->head_job, pid, 2);              //we change the status of the job
                        snprintf(buf, BUF_SIZE - 1, "Sent suspend signal to JobID %d", job_id);         //and then we sent a message to the console
                        write(fdout, buf, BUF_SIZE);
                        write(fdout, "OK", BUF_SIZE);
                    }
                    else if(strcmp(token,"RESUMED") == 0)               //RESUME command (means that a job has been resumed)
                    {
                        token = strtok(NULL, " ");
                        pid = atoi(token);
                        token = strtok(NULL, " ");
                        job_id = atoi(token);
                        change_status(temp->head_job, pid, 1);          //we change the status of the job
                        snprintf(buf, BUF_SIZE - 1, "Sent resume signal to JobID %d", job_id);          //and then we sent a message to the console
                        write(fdout, buf, BUF_SIZE);
                        write(fdout, "OK", BUF_SIZE);
                    }
                    else if(strcmp(token,"TERMINATED") == 0)        //TERMINATED command (means that a pool has been terminated after a signal was sent by the coord)
                    {
                        token = strtok(NULL, " ");
                        active_jobs += atoi(token);                 //adding the jobs that were active at the time we terminated them
                        temp->status = 0;
                        close(temp->fdin);                          //we close the fifos and the file descriptors and changes the status of the pool
                        close(temp->fdout);
                        snprintf(pool_in, BUF_SIZE - 1, "pool_in_%d.fifo",temp->id);
                        snprintf(pool_out, BUF_SIZE - 1, "pool_out_%d.fifo",temp->id);
                        unlink(pool_in);
                        unlink(pool_out);
                    }
                }
            }

            temp = temp->next;                                      //iterate the next pool node
            if(flag_shutdown && (temp == NULL) && (active_pools == 0))          //we check if we are ready to terminate the coord (after receiving a shutdown command from the console)
            {
                snprintf(buf, BUF_SIZE - 1, "Served %d jobs, %d were still in progress", num_of_jobs, active_jobs);     //sent a message to the console about the final stats
                write(fdout, buf, BUF_SIZE);
                write(fdout, "EXIT_CONSOLE", BUF_SIZE);
                loop = 0;
            }
        }

    }

    deallocate(head);                                                           //deallocating the memory we used
    free(buf);
    free(buf2);
    free(pool_in);
    free(pool_out);
    free(max_jobs_string);
    free(jms_in);
    free(jms_out);
    free(path);
}
