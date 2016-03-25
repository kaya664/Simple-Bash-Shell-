#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

//struct for background processes
struct Jobs {
int pid;
int waitingpid;
char process_name[80];
int job_number;
struct Jobs *nextJob;
int finished;
int killed;
};
//first node
struct Jobs *FirstJob=NULL;
//temp node
struct Jobs *temp=NULL; 
//process number for background processes 
int process_number=0;
int findProcessNumber();
struct Jobs *deleteNode(struct Jobs *currP, int c_pid);
struct Jobs *add(struct Jobs *head,int pid,int process_num,char name[80],int waitingpidx);
int searchRunningProcesses(struct Jobs *head, int process_number);
void killProcess(int process_n,struct Jobs *head);
void printJobs(struct Jobs *head,int running);
void checkRunningProcess(struct Jobs *head);
int redirect(char **command[],char *argv,pid_t childpid);
int redirect2(char **command[],char *argv,char *argv2,pid_t childpid);
int pipeCommand(char **command[]);
void findPath(char **command);
int checkStatus(pid_t pid);
void exitShell(struct Jobs *head);
void bonus(char **command[],char *argv);
void bonus2(char **command[],int ct);
void ps_all();

int redirection=0;//keep redirection number i.e for ">" redirection=1
char buf[50];
char *args2[6];//temporary array pointer 
int a=0;
int b=0;//keep index of redirection symbols in args for ">",">>",">&"
int c=0;//keep index of redirection symbols in args for "<"
int d=0;//keep index of pipe symbol(|) in args 
int redandpipe;//is set if args includes any redirection or pipe symbols
int pipecmd=0;// is set if args includes pipe symbol
int pss=0;//is set if args includes "ps_all"
int killl=0;//is set if args includes "kill"
int ampersant=0;//keep index of symbol of ampersant
char path[50];//keep path of the executable command
void setup(char inputBuffer[], char *args[],int *background)
{    
    a=0;
    b=0;
    c=0;
    d=0;
    pipecmd=0;
    redirection=0;
    redandpipe=0;
    ampersant=0;
    
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	//printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	/*for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);*/
 

//find whether args includes redirection or pipe
while(a<ct){
if(strcmp(args[a],">")==0){
 redirection=redirection+1;//if ">" is found increment redirection
 b=a; //keep index of ">" in the args 
 a++;
}else if(strcmp(args[a],">>")==0){
 redirection+=2;//if ">>" is found redirection is set to 2
 b=a;//keep index of ">>" in the args
 a++;
}else if(strcmp(args[a],"<")==0){
 redirection=redirection+3;//if "<" is found redirection is set to 3
 c=a;//set c to keep index of "<" in the args
 a++;
}else if(strcmp(args[a],"")==0){
  redirection+=5;//if ">&" is found redirection is set to 5
  b=a;//keep index of ">&" in the args
  a++;
}else if(strcmp(args[a],"|")==0){
 pipecmd=1;//if "|" is found value of pipecmd is set to 1
 d=a;//keep index of "|" in the args
 a++;
}else if(strcmp(args[a],"&")==0){
  ampersant=a;//if "&" is found value of ampersant is set to value of a
  a++;
}
else{ a++;
     
   }
}
//if pipe exists,temporary array is set
int e=0;
int f=d+1;
while(f<=ct){
args2[e]=args[f];//args2  is temporary array 
f++;
e++;
}

//if args includes pipe,call function of pipeCommand
if(pipecmd==1 && redirection==0){
redandpipe=1;//if redirection symbols or pipe operator is found,set to 1
args[d]=NULL;//args[d] is set as null according to d value that is index of pipe
pid_t childpid;
childpid=fork();//create child process
  if(childpid==0)//code segment of child process 
  pipeCommand(&args);//call function of pipeCommand to execute pipe operator
  else{
  waitpid(childpid,1,0);//wait child process
  kill(childpid,SIGKILL);//kill child process
  }
}
//if args includes only truncated redirection symbol (>),call redirect function
else if((strcmp(args[0],"kill")!=0) && (strcmp(args[0],"ps_all")!=0) && (redirection==1) && (pipecmd==0)){
          redandpipe=1;          
          pid_t childpid=0;
          args[b]=NULL;//args[b] is set as null according to b value that is index of ">"
          redirect(&args,args[b+1],childpid);//call function of redirect to do I/O redirection
}
//if args includes only  redirection symbol (>>) that is used to append the file,call redirect function
else if((strcmp(args[0],"kill")!=0) && (strcmp(args[0],"ps_all")!=0) && (redirection==2)){
   redandpipe=1;
   pid_t childpid=0;
   args[b]=NULL;//args[b] is set as null according to b value that is index of ">>"
   redirect(&args,args[b+1],childpid);//call function of redirect to do I/O redirection
//if args includes only  redirection symbol (<) that is used to get input from  file,call redirect function   
}else if(redirection==3){
    redandpipe=1;   
    pid_t childpid=0;
    args[c]=NULL;//args[c] is set as null according to c value that is index of "<"
    redirect(&args,args[c+1],childpid);//call function of redirect to do I/O redirection
//if args includes only  redirection symbols (< and > ) that is used to get input and put output,call redirect2 function    
}else if(redirection==4){
    redandpipe=1;
    pid_t childpid=0;
    args[c]=NULL;//args[c] is set as null according to c value that is index of "< and >"
    redirect2(&args,args[c+1],args[b+1],childpid);//call function of redirect to do I/O redirection
//if args includes only  redirection symbol (>&) that is used to put error the file,call redirect function    
}else if((strcmp(args[0],"kill")!=0) && (strcmp(args[0],"ps_all")!=0) && (redirection==5)){
    redandpipe=1;
    pid_t childpid=0;
    args[b]=NULL;//args[c] is set as null according to c value that is index of ">&"
    redirect(&args,args[b+1],childpid);//call function of redirect to do I/O redirection
//if args includes "ps_all" and redirection symbols,call redirect function    
}else if((strcmp(args[0],"ps_all")==0)&&(redirection!=0)&&(pipecmd==0)){
  pss=1;//if arguments includes "ps_all" set pss to 1
  redandpipe=1;
  pid_t childpid=0;
  args[b]=NULL;
  redirect(&args,args[b+1],childpid);//call function of redirect to do I/O redirection using output of ps_all commands
//if args includes "kill" statement
}else if((strcmp(args[0],"kill")==0) && (redirection!=0)){
	killl=1;//if arguments includes "kill" set variable of killl to 1
	redandpipe=1;
    pid_t childpid=0;
    args[b]=NULL;
    redirect(&args,args[b+1],childpid);//call function of redirect to do I/O redirection using output of kill commands
 //to execute bonus part,both pipe and I\O operands
 //for executing bonus part,firstly check whether args includes both pipe(|) and I/O symbol (>)
 //call function bonus to execute and I/O redirection   
}else if((pipecmd==1) && (redirection==1)){
  redandpipe=1;   
   args[d]=NULL;
   args[b]=NULL;
   int e=0;
   int f=d+1;
   while(f<=b){ 
   args2[e]=args[f];
    f++;
    e++;
    }
  bonus(&args,args[b+1]);
}

} /* end of setup routine */

//prints all backround process
void ps_all(){
	temp=NULL;
	//temp used to get linked lists head node of the background jobs list
	temp=FirstJob;
	//calls printJobs function to print running and finished background jobs
	printJobs(temp,1);
	temp=NULL;
	temp=FirstJob;
	printJobs(temp,0);
}
//add a background process to the linked list
struct Jobs *add(struct Jobs *head,int pid,int process_num,char name[],int waitingpidx){
	//if linked list is null
if(head==NULL){
struct Jobs *newNode=(struct Jobs *)malloc(sizeof(struct Jobs));
newNode->pid=pid;
newNode->job_number=process_num;
newNode->nextJob=NULL;
newNode->waitingpid=waitingpidx;
strcpy(newNode->process_name,name);
newNode->finished=0;
newNode->killed=0;
head=newNode;
//process_number++;
}
//else go to the next node
else{
head->nextJob=add(head->nextJob,pid,process_num,name,waitingpidx);
}
//return the linked list
return head;
}
//delete finished and showed background process from linked list
struct Jobs *deleteNode(struct Jobs *currP, int c_pid)
{
  /* See if we are at end of list. */
  if (currP == NULL)
    return NULL;

  /*
   * Check to see if current node is one
   * to be deleted.
   */
  if (currP->pid == c_pid) {
    struct Jobs *tempNextP;

    /* Save the next pointer in the node. */
    tempNextP = currP->nextJob;

    /* Deallocate the node. */
    free(currP);
    process_number--;
    /*
     * Return the NEW pointer to where we
     * were called from.  I.e., the pointer
     * the previous call will use to "skip
     * over" the removed node.
     */
    return tempNextP;
  }

  /*
   * -------------- RECURSION-------------------
   * Check the rest of the list, fixing the next
   * pointer in case the next node is the one
   * removed.
   */
  currP->nextJob = deleteNode(currP->nextJob, c_pid);


  /*
   * Return the pointer to where we were called
   * from.  Since we did not remove this node it
   * will be the same.
   */
  return currP;
}
//printing background linked list
void printJobs(struct Jobs *head,int running){
	//if a process is running
if(running==1){
printf("******Background Jobs******\nRUNNING PROCESSES");
//for each element of the linked list
while(head!=NULL){
	//check if the process is finished or not
if(checkStatus(head->pid)==0){
	//if not finished
printf("\n%d %s %d %d",head->job_number,head->process_name,head->pid,head->waitingpid);
}
head=head->nextJob;
}
}
//if a process is finished
else{
printf("FINISHED PROCESSES");
//for each elements of the linked list
while(head!=NULL){
	//check if process is finished
if(checkStatus(head->pid)==1){
  head->waitingpid=1;
  //if first show of the process on finished
  if(head->finished==0){
  head->finished=1;
  printf("\n%d %s %d %d",head->job_number,head->process_name,head->pid,head->waitingpid);
  }
  temp=FirstJob;
  //delete the showed finished process from linked list
  FirstJob=deleteNode(temp,head->pid);
}
head=head->nextJob;
}
}
printf("\n");
}

//our shell supports I/O redirection on stder,stdin and/or stdout
//command keeps adress of args array
//argv keeps name of the file that will be  opened
int redirect(char **command[],char *argv,pid_t childpid){

if((pss==1)||(killl==1)){//if user input includes "ps_all" or "kill" do followings

if(redirection==1){//if redirection is 1 i.e user input includes ">"
int  temp2=dup(STDOUT_FILENO);//redirect stdout to temp	
int fd=open(argv,O_WRONLY | O_CREAT | O_TRUNC, CREATE_MODE);//file is cretaed if it does not exist and truncated if it does
dup2(fd,STDOUT_FILENO);//redirect stdout to file
if((pss==1)&&(killl==0)){//if "ps_all" exists call ps_all function
ps_all();
}else{//else outpıt of the kill command is redirected
  temp=NULL;
  temp=FirstJob;
  killProcess(fgkillToInt(*command[0]),temp);
}
close(fd);//close the file
dup2(temp2,STDOUT_FILENO);//again redirect first stdout,turn stdout from file to terminal
return 0;
}
else if(redirection==2){//if redirection is 1 i.e user input includes ">>"
int  temp2=dup(STDOUT_FILENO);//below  operations is done 	
int fd=open(argv,CREATE_FLAGS, CREATE_MODE);//file is cretaed if it does not exist and appended to if it does
dup2(fd,STDOUT_FILENO);
if((pss==1)&&(killl==0)){
ps_all();
}else{
  temp=NULL;
  temp=FirstJob;
  killProcess(fgkillToInt(*command[0]),temp);
}
close(fd);
dup2(temp2,STDOUT_FILENO);
return 0;
}
else if(redirection==5){//if redirection is 1 i.e user input includes ">&"
int  temp2=dup(STDERR_FILENO);	
int fd=open(argv,O_WRONLY | O_CREAT | O_TRUNC, CREATE_MODE);
dup2(fd,2);//wirte the standaart error to the file
if((pss==1)&&(killl==0)){
ps_all();
}else{
  temp=NULL;
  temp=FirstJob;
  killProcess(fgkillToInt(*command[0]),temp);
}
close(fd);
dup2(temp2,2);
return 0;
}
}
else {

int fd;
childpid=fork();

if(childpid==0){

int out;
   if(redirection==1){
    findPath(&*command[0]);
    fd = open(argv, O_WRONLY | O_CREAT | O_TRUNC, CREATE_MODE);
   }
   
   if(redirection==2){
    findPath(&*command[0]);
   fd = open(argv, CREATE_FLAGS, CREATE_MODE);
   }

   if(redirection==5){
   fd = open(argv, O_WRONLY | O_CREAT | O_TRUNC, CREATE_MODE);
   
    if (fd == -1) {
       perror("Failed to open my.file");
       return 1;
    }
    if (dup2(fd, 2) == -1) {
      perror("Failed to redirect standard output");
      return 1;
    }
    if (close(fd) == -1) {
      perror("Failed to close the file");
      return 1;
    }
    
    findPath(&*command[0]);
   
    goto x;
   }
   if(redirection==3){
     findPath(&*command[0]);
     fd=open(argv,O_RDONLY);//uses cpntents of the file
   
    if (fd == -1) {
       perror("Failed to open my.file");
       return 1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
      perror("Failed to redirect standard output");
      return 1;
    }
    if (close(fd) == -1) {
      perror("Failed to close the file");
      return 1;
    }
   }
   else{
     if (fd == -1) {
       perror("Failed to open my.file");
       return 1;
     }
     if (dup2(fd, STDOUT_FILENO) == -1) {
      perror("Failed to redirect standard output");
      return 1;
     }
     if (close(fd) == -1) {
      perror("Failed to close the file");
      return 1;
     }
   }
x:
	if(execv(path , &*command[0] )<0){
     perror("Error execv");
     exit(0);
    }

}else{
waitpid(childpid,1,WUNTRACED);
kill(childpid,SIGKILL);//kill child process
return 0;
}
}
}
//executes entered command which will read input from file and stdout of the command is redirected to the file
int redirect2(char **command[],char *argv,char *argv2,pid_t childpid){

findPath(&*command[0]);//find the path to execute entered command
childpid=fork();
if(childpid==0){

   int in,out;

   in = open(argv, O_RDONLY );//read from file
   out = open(argv2, O_WRONLY | O_CREAT | O_TRUNC , CREATE_MODE);//file is cretaed if it does not exist and truncated if it does
   dup2(in, STDIN_FILENO);
   dup2(out, STDOUT_FILENO);
   
   close(in);//close the file
   close(out);//close the file


if(execv(path , &*command[0] )<0){//execute the commands
perror("Error execv");
exit(0);
}
}else{
waitpid(childpid,1,0);
kill(childpid,SIGKILL);
return 0;
}
}
//our shell supports pipe operator 
//pipe operator is implemented in the pipeCommand function 
int pipeCommand(char **command[]){
char y[50];
//check whether ps_all exist left side of the pipe operator or not
if(strcmp(args2[0],"ps_all")==0){
  printf("%s: command not found\n",args2[0]);
  return 1;
}

strcpy(y,"/usr/bin/which ");

strcat(y,args2[0]);

char path2[50];//find the path of second command to execute
FILE *proc2 = popen(y,"r");
while ( !feof(proc2) && fgets(path2,sizeof(path),proc2) ){

}
pclose(proc2);//close the proc
char * test2;
test2 = strtok (path2,"\n");
while(test2 != NULL){
test2= strtok (NULL, " ");
}
 pid_t childpid,childpid2;
   int fd[2];
 
   if ((pipe(fd) == -1) || ((childpid = fork()) == -1)) {
      perror("Failed to setup pipeline");
      return 1;
   }
      
   if (childpid == 0) {                                  /* ls is the child */
      if (dup2(fd[1], STDOUT_FILENO) == -1) 
         perror("Failed to redirect stdout of ls");
      else if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) 
         perror("Failed to close extra pipe descriptors on ls");
      else {
         if(strcmp(*command[0],"ps_all")==0){
            ps_all();
            exit(0);
         }else{
          findPath(&*command[0]); 
          execv(path, &*command[0] );//execute the command
          perror("Failed to exec ls");
         }
         
      }
      return 1; 
   }
  
   
    
     if (dup2(fd[0], STDIN_FILENO) == -1)               /* sort is the parent */
       perror("Failed to redirect stdin of sort");
     else if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) 
       perror("Failed to close extra pipe file descriptors on sort"); 
     else {
      execv(path2 , &args2[0]);
      perror("Failed to exec sort");
     }
     return 1;
    
}
//check whether backround processes is run or finished or not
int checkStatus(pid_t pid){
pid_t childpid;
childpid=waitpid(pid,NULL,WNOHANG);//use wnohang
if(childpid==pid||childpid<0)
return 1;
else 
return 0;
}
//finds the path of executable commands
void findPath(char **command){
   
  char x[50];
  strcpy(x,"/usr/bin/which ");
  strcat(x,command[0]);
   
  FILE *proc = popen(x,"r");
  while ( !feof(proc) && fgets(path,sizeof(path),proc) ){

  }
  pclose(proc);
  char * test;
  test = strtok (path,"\n");
  while(test != NULL){
  test= strtok (NULL, " ");
 }
 
 
}
//searchs the running processes to kill or get to wait
int searchRunningProcesses(struct Jobs *head, int process_n){
	//for each element in linked list
  while(head!=NULL){
  	//if process number given is equal to the process
    if(head->job_number==process_n||head->pid==process_n){
      if(head->waitingpid!=0){
        return 0;
      }
      head->waitingpid=1;
      head->killed=1;
      //return the pid of the process which going to be killed
      return head->pid;
    }
    head=head->nextJob;
  }
  //else return 0
  return 0;
}
//killing process
void killProcess(int process_n,struct Jobs *head){
	//check whether process is running or not
  int pidkilled=searchRunningProcesses(head,process_n);
   if(pidkilled!=0){
   	//if process on running, kill the process
   kill(pidkilled,SIGKILL);
   printf("The process with pid : %d is killed succesfully\n",pidkilled);
   }
   else{
   	//if there is no process to kill
    printf("There is no process to kill\n");
   }
}
//exit from shell
void exitShell(struct Jobs *head){
	int ex=0;
	//if there are processes which running
	while(head!=NULL){
		if(head->waitingpid==0){
			//make ex = 1
            ex=1;
		}
		head=head->nextJob;
	}
	//if ex = 1, so there are processes running
	if(ex==1){
		head=FirstJob;
    printf("There processes on run\n");
    //print the running processes to the user
    while(head!=NULL){
      if(head->waitingpid==0){
        printf("\n%d %s %d %d",head->job_number,head->process_name,head->pid,head->waitingpid);
      }
      head=head->nextJob;
    }
    printf("\n");
  }
  // if there are no processes running just exit
  else{
    exit(0);
  }
}
//get background process to the foreground
void fg(struct Jobs *head, int process_n){
	//search for running process
int pidrunning=searchRunningProcesses(head,process_n);
if(pidrunning!=0){
	//if running so get wait
   waitpid(pidrunning,NULL,WUNTRACED);
   //printf("The process with pid : %d is killed succesfully\n",pidkilled);
   //delete the node from background process list
   deleteNode(head,pidrunning);
   }
   //else there are no processes on running, so print result
   else{
    printf("There is no process to get foreground with process number %d\n",process_n);
   }
}
//find new process number for new background processes to add linked list
int findProcessNumber(){
	temp=FirstJob;
	int pnum=100;
	//find the minimum number of the list
    while(temp!=NULL){
    	if(temp->job_number<pnum){
           pnum=temp->job_number;
    	}
    	temp=temp->nextJob;
    }
    //if pnum is no equal to the 0 return 0
    if(pnum!=0){
    	return 0;
    }
    pnum+=1;
    temp=FirstJob;
    //check the list for suitable process number
    while(temp!=NULL){
    	if(pnum==temp->job_number){
    		pnum++;
    		temp=FirstJob;
    	}
    	temp=temp->nextJob;
    }
    return pnum;
}
//for processes such as  kill %number or fg %number, find the integer value of number
int fgkillToInt(char a[100]){
    char *test;
    char buffed[10];
    strncpy(buffed, *a+1, 2);
    buffed[1]='\0';
    strcpy(a,buffed);
    return atoi(a);
}
//our shell supports pipe with I/O redirections
//firstly pipe operator handle then stdout of the pipe commands is directed to the file
void bonus(char **command[],char *argv){
  
  int out,tempstdout;
  tempstdout=dup(1);
  out = open(argv, O_WRONLY | O_CREAT | O_TRUNC, CREATE_MODE);
  dup2(out,STDOUT_FILENO);
  
  char y[50];
  strcpy(y,"/usr/bin/which ");
  strcat(y,args2[0]);
  char path2[50];
   FILE *proc2 = popen(y,"r");
  while ( !feof(proc2) && fgets(path2,sizeof(path),proc2) ){

   }
   pclose(proc2);
   char * test2;
   test2 = strtok (path2,"\n");
   while(test2 != NULL){
   test2= strtok (NULL, " ");
   }
   pid_t childpid,childpid2;
   childpid2=fork();
   if(childpid2==0){
      int fd[2];
    if ((pipe(fd) == -1) || ((childpid = fork()) == -1)) {
      perror("Failed to setup pipeline");
      return ;
   }
      
   if (childpid == 0) {                                  /* ls is the child */
      if (dup2(fd[1], STDOUT_FILENO) == -1) 
         perror("Failed to redirect stdout of ls");
      else if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) 
         perror("Failed to close extra pipe descriptors on ls");
      else {
         if(strcmp(*command[0],"ps_all")==0){
           ps_all();
            exit(0);
         }else{
          findPath(&*command[0]); 
          execv(path, &*command[0] );
          perror("Failed to exec ls");
         }
         
      }
       return;
   }
  
   
    
     if (dup2(fd[0], STDIN_FILENO) == -1)               /* sort is the parent */
       perror("Failed to redirect stdin of sort");
     else if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) 
       perror("Failed to close extra pipe file descriptors on sort"); 
     else {
      execv(path2 , &args2[0]);
      perror("Failed to exec sort");
      exit(0);
     }

     
   return;
}
else{
  waitpid(childpid2,1,0);
  kill(childpid2,SIGKILL);
  close(out);
  dup2(tempstdout,1);
}
   }
  
int main(void)
{ 
            //*path=NULL;
            path[0]='\0';
            int pid;
            
            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */          
             while (1){  // shell loop
              
         //    pid = fork();//fork to print the 333sh: message
             pss=0;
             killl=0;
             path[0]='\0';
            background = 0;//initialize the background variable
             printf(" 333sh: ");
            fflush(stdout);
          
                       //setup part
                        setup(inputBuffer, args, &background);
             //exit(0);
           //if there are no read and pipe processes
           if(!redandpipe){ 
             //if input command is ps_all
             if((strcmp(args[0],"ps_all")==0)&&(redirection==0)){
             ps_all();
             }
             //if input command is kill
			             else if(strcmp(args[0],"kill")==0){
			  temp=NULL;
			  temp=FirstJob;
			      char buffed[10];
			      buffed[0]='\0';
			    strncpy(buffed,(*(&args[1])+1), 10);
			    
              //kill with %
			  if(strstr(args[1],"%")!=NULL){
			     killProcess(atoi(buffed),temp);
			  }
			  //kill with integer
			  else{
			    killProcess(atoi(args[1]),temp);
			  }
			}
			//if command is exit
			else if(strcmp(args[0],"exit")==0){
			  temp=NULL;
			  temp=FirstJob;
			  exitShell(temp);
			}
			//if command is fg
			else if(strcmp(args[0],"fg")==0){
			  temp=NULL;
			  temp=FirstJob;
			   char *test;
			   char buffed[10];
			      buffed[0]='\0';
			    strncpy(buffed,(*(&args[1])+1), 10);
			   fg(temp,atoi(buffed));
			}
			//if there is no built in process commands, just use execv to execute the user command
             else{
            // find the ampersand
               if(args[1]!=NULL && strcmp(args[ampersant],"&")==0){
         
	             args[ampersant]=NULL;
	           } 
               int pidd;
               findPath(args);
           
               //fork
               pidd = fork();
               if(pidd == 0){
                //child executes the command
                if(execv(path, &args[0])<0){
                perror("Error execv");
                exit(0);
                } 
               }
               else if(pidd<0){
	           printf("error");
               }
               else {
               	//parent checks if the process is background or foreground
                 if(background==0){
                 	//if foreground, wait process
                 waitpid(pidd,NULL,WUNTRACED);
                 }
                 else{
                 	//else add the process to the background linked list
                 	process_number=findProcessNumber();
                 FirstJob=add(FirstJob,pidd,process_number,args[0],0);
                 }
                }
            }
           }
                        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
                       (3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
          }
}

