#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#define MAXLINE 80
#define MAXARGS 20


//Lab 5 Create Job Array which stores information about processes.
struct job_array {
   int process_id;
   char command[80];
   int job_number;
};
//Lab 5: Declare job array and intialize process count to zero.
struct job_array jobArray[20];
int currentProcess = 0;
int totalProcesses = 0;
/* ----------------------------------------------------------------- */
/*================Isaac Israel====================================== */
/*================Lab 5============================================= */
/*=============== 11/25/2015======================================== */
/* ----------------------------------------------------------------- */
void process_input(int argc, char **argv) {
   int i = 0;
   int pos = 0;
      
   for(i=0; i<argc; i++) {
      if(strcmp(argv[i], ">") == 0) { //If any parsed commands involve ">" then trigger file redirection
         if (i == 0) printf("Error with redirection\n"); //Display errors when ">" is at beginning of end of command.
   	 else if (i == (argc - 1)) printf("Error with redirection\n");
         else {
            pos = i;
            int fileId = open(argv[pos+1],O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            dup2(fileId,1); //Open file, redirect output and,  then close current.
            close(fileId);
            argv[pos]= NULL;
	  }
    } //If any parsed commands involve "<" then trigger file redirection
	else if (strcmp(argv[i], "<") == 0) {
	  if (i == 0) printf("Error with redirection\n"); //Display errors when "<" is at beginning of end of command.
	  else if (i == (argc - 1)) printf("Error with redirection\n");
	  else { 
           pos = i;
           int fileId = open(argv[pos+1],O_RDONLY);
           dup2(fileId,0); //Open file, redirect output and, then close current.
           close(fileId);
           argv[pos]= NULL;
	 }
       }   
  }
  if (execvp(argv[0],argv) == -1) {
    perror("Error with execvp:"); //Throw error if exexvp returns -1.
    exit(0);
  }
}
/* ----------------------------------------------------------------- */
/*                  parse input line into argc/argv format           */
/* ----------------------------------------------------------------- */
int parseline(char *cmdline, char **argv)
{
  int count = 0;
  char *separator = " \n\t";
  argv[count] = strtok(cmdline, separator);
  while ((argv[count] != NULL) && (count+1 < MAXARGS)) {
   argv[++count] = strtok((char *) 0, separator);
  }
  return count;
}
/* ----------------------------------------------------------------- */
/*                  Lab 5: Child Handler                             */
/* ----------------------------------------------------------------- */

void childHandler (int sig) {
   pid_t pid;
   pid = wait(NULL);
   int j;
   for (j = 0; j < 20; j++)
      if(jobArray[j].process_id == pid) {
         printf("[%i]  Done\t %s\n", jobArray[j].job_number, jobArray[j].command);
         jobArray[j].process_id = 0;
         jobArray[j].command[0] = 0;
         jobArray[j].job_number = 0;
         continue;
      }
   totalProcesses--;
}
/* ----------------------------------------------------------------- */
/*                  The main program starts here                     */
/* ----------------------------------------------------------------- */
int main(void)
{
   char cmdline[MAXLINE];
   char *argv[MAXARGS];
   //char *path;
   //char *cwd;
   int argc;
   int status;
   pid_t pid;
   if(signal(SIGCHLD, childHandler) == SIG_ERR)
      _exit(0);
 /* Sigaction declaration */
 struct sigaction handler;
 handler.sa_handler = SIG_IGN;
 sigemptyset(&handler.sa_mask);
 handler.sa_flags = 0;
 sigaction(SIGINT, &handler, NULL);
 /* Loop forever to wait and process commands */
 int x;
 for(x = 0; x < 10; x++) {
  /* Step 1: Name your shell: csc60mshell - m for mini shell */
   printf("csc60mshell : ");
   fgets(cmdline, MAXLINE, stdin);
   char tcmdline[80];
   strcpy(tcmdline,cmdline);
   argc = parseline(cmdline, argv);
   /* Step 1: Handle build-in command: exit, pwd, or cd - if detect one              */
   if(argc == 0)
      continue; //If no command inserted, continue.
   else if(argc == 1 && strcmp(argv[0], "exit") == 0) {
      if (totalProcesses == 0)   
         _exit(0); //Search for exit then exit with flush.
      else
         printf("Please terminate remaining processes.\n");
   }
   else if(argc == 1 && strcmp(argv[0], "pwd") == 0){
      char* cwd; //Search for "pwd" and display output of pwd.
      char buffer[256];
      cwd = getcwd(buffer,sizeof(buffer));
      printf("%s\n", cwd);
   } //Search for CD then check if its by itself or not. 
   else if (strcmp(argv[0], "cd") == 0){
	  if (argc > 1) //Sole CD sends user home.
	    chdir(argv[1]);
	  else //CD Path sends user to path.
	    chdir(getenv("HOME"));
   }  
   else if (strcmp(argv[0], "jobs") == 0){
     printf("Current Jobs:\n");
     int k;
     for (k = 0; k < 20; k++)
        if (jobArray[k].process_id != 0)
           printf("[%i] Running\t %s\n", jobArray[k].job_number, jobArray[k].command);
   }
   else { //If command is not built in or empty, fork process execute command.
    /* Step 1 Check to see if the command is for a background process */
     bool bg = false; //initialize bg to false
     if (strcmp(argv[argc-1],"&") == 0) {
         currentProcess = 0; //If true, set current process to 0
         while (jobArray[currentProcess].process_id != 0)
            currentProcess++; //cycle through the job array for the first unoccupied slot
         bg = true;           //set it as the active process slot
         argv[argc-1] = '\0';
         argc--;
         strcpy(jobArray[currentProcess].command, tcmdline);
         jobArray[currentProcess].job_number = currentProcess;
         totalProcesses++; //Input data into data structure and increment total processes.
      }
   /* Step 2. Fork. */
      pid = fork();
      if (pid == -1){
          perror("Shell Program fork error");
      }
      else if (pid == 0){
      /* I am child process. I will execute the command, call: execvp */
         //Create a new sigaction handler which will use default rules for signals.
         struct sigaction handler1;
         handler1.sa_handler = SIG_DFL; //Set to default rules.
         sigemptyset(&handler1.sa_mask);
         handler1.sa_flags = 0;
         sigaction(SIGINT, &handler1, NULL);
         process_input(argc, argv);
         if (bg) //give background processes a seperate group id.
            setpgid(0,0);
      }
      else {
      /* I am parent process */
         if(bg) { //if running in bg set jobarray to pid
            jobArray[currentProcess].process_id = pid;
            }
         else
            if (wait(&status) == -1)
               perror("Shell Program error");
            else { //Deterement method of child termination.
               if (WIFEXITED(status))
                  printf("Child Termination: Normal Status [%d]\n",WEXITSTATUS(status));
		//passing integer for status
               else if (WIFSIGNALED(status))
                  printf("Child Termination: Killed  Signal [%d]\n",WTERMSIG(status));
               else if (WIFSTOPPED(status))
                  printf("Child Stopped: Child Stopped Signal [%d]\n",WSTOPSIG(status));
               else
                  printf("Child Terminated: Unknown cause.\n");
      }
    }
   }
 }
}
