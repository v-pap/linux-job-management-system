#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include "../include/job_list.h"
#define BUF_SIZE 257

int terminate = 0;                                                                  //flag for terminating the pool and the jobs

static void shutdown(int signum)                                                    //signal handler function
{
    terminate = 1;                                                                  //just sets the terminate flag to 1
}

int count_words(char* line)                                                         //this function counts the number of words per line of input
{
    int counter = 1;
    int length = strlen(line);
    for(int i = 0; i < length; i++)
    {
        if(line[i] == ' ')                                                          //just counts the number of whitespace
            counter++;
    }
    return counter;
}

int main(int argc, char *argv[])
{
    int words,                                                                      //variables used in the program
    fdin,
    fdout,
    finished_jobs,
    stat,
    max_jobs,
    job_id;
    pid_t pid;
    char *jms_in = NULL,
    *jms_out = NULL,
    *buf = NULL,
    *buf2 = NULL,
    *path = NULL,
    **argument_vector = NULL,
    *token = NULL;
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = shutdown;
    sigaction(SIGTERM, &act, NULL);

    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i],"-w") == 0 && i+1 < argc)                                 //argument for the input fifo file
        {
            jms_in = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(jms_in, argv[i+1]);
        }
        else if(strcmp(argv[i],"-r") == 0 && i+1 < argc)                            //argument for the output fifo file
        {
            jms_out = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(jms_out, argv[i+1]);
        }
        else if(strcmp(argv[i],"-l") == 0 && i+1 < argc)                            //argument for the path of the programs outputs (if any)
        {
            path = malloc((strlen(argv[i+1])+1) * sizeof(char));
            strcpy(path, argv[i+1]);
        }
        else if(strcmp(argv[i],"-m") == 0 && i+1 < argc)                            //argument for the maximum number of jobs that each pool controls
        {
            max_jobs = atoi(argv[i+1]);
        }
    }

    if ((fdin = open(jms_in, O_RDONLY | O_NONBLOCK)) < 0)                           //opening the input fifo
    {
		perror("fifo jms_in open problem (pool)"); exit(3);
	}

    if ((fdout = open(jms_out, O_WRONLY | O_NONBLOCK)) < 0)
    {
		perror("fifo jms_out open problem (pool)"); exit(3);                        //opening the output fifo
	}

    buf = malloc(BUF_SIZE * sizeof(char));                                          //allocating space for the buffers and setting them to '\0'
    buf2 = malloc(BUF_SIZE * sizeof(char));
    memset(buf, '\0', BUF_SIZE);
    memset(buf2, '\0', BUF_SIZE);
    finished_jobs = 0;
    node_job *head = job_list_init();                                               //initializing the list of jobs

    while(1)                                                                        //main loop of the program
    {
        if(read(fdin, buf, BUF_SIZE) > 0)                                           //reading from the console
        {
            buf[strcspn(buf, "\n")] = 0;                                            //removes the new line character and replaces it with terminating character
            words = count_words(buf);
            token = strtok(buf, " ");                                               //get the first word of the current line on CLI/file
            if(strcmp(token, "submit") == 0)                                        //submit command
            {
                assert(words > 0);
                argument_vector = malloc(sizeof(char*) * (words-1));                //allocating memory for an argument vector for the submitted program
                for(int i = 0; i < words - 2; i++)
                {
                    token = strtok(NULL," ");
                    argument_vector[i] = malloc(sizeof(char) * (strlen(token)+1));  //allocating memory for every argument of the vector
                    strcpy(argument_vector[i], token);
                }
                argument_vector[words - 2] = NULL;                                  //place NULL on the last argument of the vector
                token = strtok(NULL," ");
                assert(token != NULL);
                job_id = atoi(token);
                if((pid = fork()) == 0)                                             //creating a new process with fork() and exec()
                {
                    pid = getpid();
                    time_t now;
                    struct tm ts;
                    time(&now);                                                     //getting time and converting it to a string
                    ts = *localtime(&now);
                    strftime(buf, BUF_SIZE, "%Y%m%d_%H%M%S", &ts);
                    if(path == NULL)                                                //creating the name of the folder of the job depending on if there is a path
                        snprintf(buf2, BUF_SIZE - 1, "sdi1400139_%d_%ld_%s", job_id, (long)pid, buf);
                    else
                        snprintf(buf2, BUF_SIZE - 1, "%s/sdi1400139_%d_%ld_%s", path, job_id, (long)pid, buf);
                    mkdir(buf2, 0777);
                    snprintf(buf, BUF_SIZE - 1, "%s/stdout_%d", buf2, job_id);      //creating the directory and redirecting the stdin and stdout to the appropriate files
                    freopen (buf,"w",stdout);
                    snprintf(buf, BUF_SIZE - 1, "%s/stderr_%d", buf2, job_id);
                    freopen (buf,"w",stderr);
                    if(execvp(argument_vector[0], argument_vector) == -1)
                    {
                        perror("exec pool failure");
                        exit(3);
                    }
                }
                for(int i = 0; i < words - 2; i++)                                  //freeing the allocated memory
                {
                    free(argument_vector[i]);
                }
                free(argument_vector);
                insert_job(head, pid, job_id);                                      //inserting the job to the list
                snprintf(buf, BUF_SIZE - 1, "ADDED %ld %d", (long)pid, job_id);     //and informing the coord
                write(fdout, buf, BUF_SIZE);
            }
            else if(strcmp(token, "suspend") == 0)                                  //suspend command
            {
                token = strtok(NULL, " ");
                assert (token != NULL);
                pid = atoi(token);
                if(pid)                                                             //suspend the job
                {
                    if(kill(pid, SIGSTOP) != 0)
                        printf("kill suspend error\n");
                }
                else
		{
                    printf("id error\n");
		}
                change_status(head, pid, 2);                                        //changing the status of the job on the list
                token = strtok(NULL, " ");
                assert (token != NULL);
                job_id = atoi(token);                                               //and informing the coord
                snprintf(buf, BUF_SIZE - 1, "SUSPENDED %ld %d", (long)pid, job_id);
                write(fdout, buf, BUF_SIZE);
            }
            else if(strcmp(token, "resume") == 0)                                   //resume command
            {
                token = strtok(NULL, " ");
                assert (token != NULL);
                pid = atoi(token);
                if(pid)                                                             //resume the job
                {
                    if(kill(pid, SIGCONT) != 0)
                        printf("kill resume error\n");
                }
                else
		{
                    printf("id error\n");
		}
                change_status(head, pid, 1);                                        //changing the status of the job on the list
                token = strtok(NULL, " ");
                assert (token != NULL);
                job_id = atoi(token);                                               //and informing the coord
                snprintf(buf, BUF_SIZE - 1, "RESUMED %ld %d", (long)pid, job_id);
                write(fdout, buf, BUF_SIZE);
            }
        }

        node_job *temp = head->next;                                                //start with the first node of the job list

        while(temp != NULL)                                                         //iterate all the job nodes of the list
        {
            if(temp->status == 1)                                                   //if the job is active (according to the list)
            {
                if(waitpid(temp->pid, &stat, WNOHANG) != 0)                         //check its status
                {
                    finished_jobs++;
                    temp->status = 0;
                    snprintf(buf, BUF_SIZE - 1, "EXIT %d",temp->pid);               //if the job is done inform the coord
                    write(fdout, buf, BUF_SIZE);
                }
            }
            temp = temp->next;
        }
        if(finished_jobs == max_jobs)                                               //if the pool has managed the maximum number of jobs then it will exit
        {
            strcpy(buf,"ENDED");                                                    //and it will inform the coord
            write(fdout, buf, BUF_SIZE);
            break;
        }
        if(terminate)                                                               //if the pool has been signaled with SIGTERM
        {
            int counter = 0;
            node_job *temp = head->next;                                            //it will iterate all the jobs that it manages and it will kill them
            while(temp != NULL)
            {
                if(temp->status == 1 || temp->status == 2)                          //checks if the job is active or suspended
                {
                    if(temp->pid)
                    {
                        if(kill(temp->pid, SIGTERM) != 0)                           //kills the process
                            printf("kill termination error\n");
                        counter++;
                    }
                }
                temp = temp->next;
            }
            snprintf(buf, BUF_SIZE - 1, "TERMINATED %d",counter);                   //it will inform the coord that it has terminated all the programs (and the quantity of the active ones)
            write(fdout, buf, BUF_SIZE);
            break;
        }
    }
    deallocate_job_list(head);                                                      //deallocates all the used memory
    free(buf);
    close(fdin);
    close(fdout);
    return 0;
}
