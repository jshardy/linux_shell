/*************************************************************
* Professor:    Jesse Chaney
* Author:		Joshua Hardy
* Filename:		lab7.c
* Date Created:	1/4/2018
* Date Modified: 1/5/2018 - Finished project
**************************************************************/
/*************************************************************
* Lab/Assignment: Lab7 - Part 2
*
* Overview:
*   Opens a named pipe for rw
*   Data is displayed on both processes(execute twice)
* Input:
*	stdin
* Output:
*   stdout
* Usage:
*   Run two copies in different terminals
*   MASTER sends data first
*   SLAVE recieves data then sends it
************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     //access()
#include <sys/types.h>  //mkfifo()
#include <sys/stat.h>   //mkfifo()
#include <fcntl.h>      //open()
#include <string.h>     //strlen()
#include<signal.h>      //signal()

#define MAX_STR 256

void sig_handler(int signo);

int main(int argc, char **argv)
{
    int master = 0;
    char c = 0;
    char str_line[MAX_STR] = { 0 };
    int fifoa = -1;
    int fifob = -1;
    int bytes = 0;
    char *str_connect = "Connected!";
    signal(SIGINT, sig_handler) ;

    //get commandline arguments
    while ((c = getopt(argc, argv, "hm")) != -1) {
        switch (c)
        {
            case 'h':
                fprintf(stdout, "lab7\nMaster and Slave processes auto deteted");
            break;
        }
    }

    fprintf(stdout, "Either side can type exit\n");
    //are the files there? No? Then we are master. Otherwise we are slave.
    if((access("fifoa", F_OK) == -1) && (access("fifob", F_OK) == -1))
        master = 1;

    //MASTER side
    if(master)
    {
        fprintf(stdout, "MASTER auto detected\n");
        //incase they exist remove them so we don't read previous garbage.
        remove("fifoa");
        remove("fifob");
        //make named pipe
        mkfifo("fifoa", 666);
        mkfifo("fifob", 666);
        //open named pipes
        fifoa = open("fifoa", O_CREAT | O_TRUNC | O_WRONLY);
        fifob = open("fifob", O_CREAT | O_RDONLY);
        fprintf(stdout, "WAITING FOR SECOND PROCESS OF LAB7\nStart second terminal/lab7 process\n\n");
        while(read(fifob, str_line, MAX_STR) == 0)
            ;//create software-block until we're connected
        fprintf(stdout, "CONNECTED!\n\n");

        //start ouput/input loop
        do
        {
            //read line
            bytes = 0;
            fprintf(stdout, ":");
            fgets(str_line, MAX_STR, stdin);

            //write line
            write(fifoa, str_line, strlen(str_line) + 1);

            //want to exit? do it after we wrote so both sides die.
            if(strcmp(str_line, "exit\n") == 0)
                break;

            //loop until we get read data
            while(bytes == 0)
                bytes = read(fifob, str_line, MAX_STR);
            str_line[bytes] = 0;

            //display what we read
            fprintf(stdout, "%s", str_line);
        } while(strcmp(str_line, "exit\n") != 0);   //did we recieve an exit

        //clean up
        close(fifoa);
        close(fifob);
        //delete them for future use.
        remove("fifoa");
        remove("fifob");
    }
    else //slave
    {
        fifob = open("fifob", O_WRONLY | O_TRUNC);
        if(fifob != -1)
        {
            fifoa = open("fifoa", O_RDONLY);
            //send out connected byte
            write(fifob, "1", 1);
            fprintf(stdout, "SLAVE auto detected\nCONNECTED!\n\n");
            do
            {
                //loop until we get data
                bytes = 0;
                while(bytes == 0)
                    bytes = read(fifoa, str_line, MAX_STR);
                str_line[bytes] = 0;

                //display what we read
                fprintf(stdout, "%s", str_line);

                //Did we receive an exit?
                if(strcmp(str_line, "exit\n") == 0)
                    break;

                //get line input for output
                fprintf(stdout, ":");
                fgets(str_line, MAX_STR, stdin);

                write(fifob, str_line, strlen(str_line) + 1);
            } while(strcmp(str_line, "exit\n") != 0);   //did we send an exit?

            //cleanup
            close(fifoa);
            close(fifob);
        }
        else
        {
            fprintf(stdout, "\nNo master process.\nStart with lab7 -m. Then start process two with lab7\n");
            fprintf(stdout, "\nlab7\n-h for this help.\n-m for master process.\nNo arguemnts defaults to slave process.\n");
        }
    }

    return 0;
}

void sig_handler(int signo)
{
    if (signo == SIGINT)
        fprintf(stdout, "\nYou mean to type exit?\n");
}
