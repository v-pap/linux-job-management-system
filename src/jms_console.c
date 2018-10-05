#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#define BUF_SIZE 257

int main(int argc, char *argv[])
{
    int flag_input = 0,
    fdin,
    fdout;
    char *jms_in,
    *jms_out,
    *buf;
    FILE *fpin;
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
        else if(strcmp(argv[i],"-o") == 0 && i+1 < argc)                        //argument for input file
        {
            fpin = fopen(argv[i+1],"r");
            flag_input = 1;
        }
    }

    if(flag_input == 0)                                                         //if no input file is given then we use the stdin
    {
        fpin = stdin;
    }

	if ((fdin = open(jms_in, O_RDONLY | O_NONBLOCK)) < 0)                       //opening the input fifo
    {
		perror("fifo jms_in open problem"); exit(3);
	}

    if ((fdout = open(jms_out, O_WRONLY | O_NONBLOCK)) < 0)                     //opening the output fifo
    {
		perror("fifo jms_in open problem"); exit(3);
	}

    buf = malloc(BUF_SIZE * sizeof(char));
    memset(buf, '\0', BUF_SIZE);

    if(fgets(buf,BUF_SIZE,fpin) != NULL)                                        //getting the first line of input
    {
        write(fdout, buf, BUF_SIZE);
    }

    while(1)                                                                    //main loop of the program
    {
        if(read(fdin, buf, BUF_SIZE) > 0)
        {
            if(strcmp(buf,"EXIT_CONSOLE") == 0)                                 //if we receive EXIT_CONSOLE the console terminates
                break;
            else if(strcmp(buf,"OK") != 0)
            {
                printf("%s\n",buf);                                             //if we receive OK then we get the next line of input from the file/stdin
                continue;                                                       //printing the input from coord
            }
            if(fgets(buf,BUF_SIZE,fpin) != NULL)
            {
                if(strncmp(buf, "exit", 4) == 0)                                //if we receive an exit from the user we quit
                    break;
                write(fdout, buf, BUF_SIZE);
            }
        }
    }
    close(fdin);                                                                //deallocating the memory we used and deleting the fifo files
    close(fdout);
    unlink(jms_in);
    unlink(jms_out);
    free(jms_in);
    free(jms_out);
    free(buf);
    fclose(fpin);
}