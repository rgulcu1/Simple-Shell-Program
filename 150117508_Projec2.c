#include <stdio.h>  //include neccesary libraries
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MAX_LINE 128 /* 128 chars per line, per command, should be enough. */
#define MAX_ARGS 32 //32 arguments maximum
#define builtInFunctionNumber 5 //total built in function number

void setEnvPathVariables(char * paths[]);
char* readPath();
void setBuiltInCommands();
void addProcessKillSignal();
static void catchSigChild(int signo);
void addCtrlZsignal();
static void cathCtrlZ(int signo);
void setup(char inputBuffer[], char *args[],int *background,int *background2);
void setupWithoutRead(char inputBuffer[], char *args[],int *background);
void pushCommand(char new_data[]);
void deleteCommand();
void designArgs(char * args[], char * args2[]);
void run(char* args[], int* background);
void checkRedirection(char * args[],char  redirectionStatus[]);
void outputRedirection(int choice, char* filename);
void inputRedirection(char* filename);
void errorRedirection(char* filename);
int checkCommandIsBuiltIn(char *command);
void execHistoryCommand(char * args[]);
char* getHistoryCommand(int index);
void printHistory();
void execPathCommand(char * args[]);
void displayPaths();
void pushPath(char newPath[]);
void deletePath(char *path);
void execFgCommand(char * args[]);
int isProcessAlive(pid_t pid);
void execExitCommand();
void execJobs();
int setPath(char * args[]);
int checkCommandisExist(char * path, char *command);
void pushProcess(pid_t newpProcessID, char* command);
void deleteProcess(pid_t processID);
void resetRedirection(char  redirectionStatus[]);


char *envPaths[15]; //create array for keep exacutable paths
int sizeofPaths; // for keep total exacutable path number
int sizeofArgs; // keep total size of arg user given
int processCounter=0; /// keeps the total number of  currently running process

//for keep Redirection default values
int tempStdIn;
int tempStdOut;
int tempStdErr;

//node for Command Linked list
struct Command {
    char input[MAX_LINE];
    struct Command* next;
};
//node for Path Linked list
struct Path {
    char path[50];
    struct Path * next;
};
//node for Process Linked list
struct Process{
    pid_t processID;
    char command[30];
    struct  Process * next;
};

typedef  struct Command Command;
typedef  struct Path Path;
typedef struct Process Process;
//initialize heads
Command* commandHead = NULL;
Path * pathHead = NULL;
Process * processHead = NULL;

char * builtInCommands[builtInFunctionNumber]; //keep the built-in commands names



void setEnvPathVariables(char * paths[]){ // for read env Paaths and fill envPath array.
    char *fullpath = readPath(); // read the $PATH
    int pathCounter = 0; // keep number of total path
    int start = -1;
    int endOfPath = 1;
    int i = 0;

    while (endOfPath) { //removes semicolons and seperates paths.
        switch (fullpath[i]) {
            case ':':
                fullpath[i] = '\0';
                if (start != -1) {
                    paths[pathCounter] = &fullpath[start];
                    pathCounter++;
                }
                start = -1;
                break;
            case '\0' :
                if (start != -1) {
                    paths[pathCounter] = &fullpath[start];
                    pathCounter++;
                }
                endOfPath = 0;
                start = -1;
                break;
            default :
                if (start == -1) {
                    start = i;
                }
        }
        i++;
    }
    sizeofPaths = pathCounter; //set global size of Path variable
}

char* readPath(){ // read the $PATH variable
    char *envvar = "PATH";
    // Make sure envar actually exists
    if(!getenv(envvar)){
        fprintf(stderr, "The environment variable %s was not found.\n", envvar);
        exit(1);
    }
    return getenv(envvar);
}

void setBuiltInCommands(){ //Fill the built in command array with command names
    for (int i = 0; i < builtInFunctionNumber ; ++i) {
        builtInCommands[i] = malloc(sizeof(char)*20); //allocate memory
    }
    strcpy(builtInCommands[0],"history");
    strcpy(builtInCommands[1],"path");
    strcpy(builtInCommands[2],"fg");
    strcpy(builtInCommands[3],"exit");
    strcpy(builtInCommands[4],"jobs");
}

void addProcessKillSignal(){ //handler for SIGCHLD signal
    struct sigaction deadProcess;
    deadProcess.sa_handler = catchSigChild; //execute catchSigChildfunction
    deadProcess.sa_flags = 0;
    if ((sigemptyset(&deadProcess.sa_mask) == -1) ||
        (sigaction(SIGCHLD, &deadProcess, NULL) == -1)) {
        perror("Failed to set SIGINT handler");
        exit(0);
    }
}

static void catchSigChild(int signo) {
    pid_t deadPid ;
    deadPid = waitpid(-1,NULL,WNOHANG); //find the child process id
    if(deadPid >0){ // if child is exist kill child and delete process from process linked list.
        deleteProcess(deadPid);
        kill(deadPid,SIGKILL);
    }

}

void addCtrlZsignal(){
    struct sigaction ctrlZ;
    ctrlZ.sa_handler = cathCtrlZ;
    ctrlZ.sa_flags = 0;
    if ((sigemptyset(&ctrlZ.sa_mask) == -1) ||
        (sigaction(SIGTSTP, &ctrlZ, NULL) == -1)) {
        perror("Failed to set SIGINT handler");
        exit(0);
    }
}

static void cathCtrlZ(int signo) {
   pid_t processID;
   processID = waitpid(-1,NULL,WNOHANG);
   //printf("The main process is not wait child anymore.\n");
}

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background,int *background2)
{
    int doubleComandCheck =0;
    int length, /* # of characters in the command line */
            i,      /* loop index for accessing inputBuffer array */
            start,  /* index where beginning of next command parameter is */
            ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
    pushCommand(inputBuffer);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);           /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

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
                if(inputBuffer[i] == ';') doubleComandCheck = 1;
                if (inputBuffer[i] == '&'){
                    if(doubleComandCheck == 0) *background  = 1;
                    else{
                        *background2 = 1;
                    }
                    start = -1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */
    sizeofArgs = ct;
    if(args[0]==NULL){
        return;
    }
    if(strcmp(args[0],"history")==0) deleteCommand();
} /* end of setup routine */

void setupWithoutRead(char inputBuffer[], char *args[],int *background)
{
    int     i,      /* loop index for accessing inputBuffer array */
            start,  /* index where beginning of next command parameter is */
            ct;     /* index of where to place the next parameter into args[] */

    ct = 0;
    int length = strlen(inputBuffer);
    /* read what the user enters on the command line */

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

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
                    start = -1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */
} //for run command without read input from user

void pushCommand(char new_data[])
{
    /* 1. allocate node */
    Command* new_node = (Command*) malloc(sizeof(Command));

    /* 2. put in the data  */
    strcpy(new_node->input,new_data);

    /* 3. Make next of new node as commandHead */
    new_node->next = commandHead;

    /* 4. move the commandHead to point to the new node */
    commandHead = new_node;
}

void deleteCommand(){
    Command* temp = commandHead;
    if(temp->next != NULL){
        commandHead = commandHead->next;
        free(temp);
    }else{
        commandHead = NULL;
    }
}

void designArgs(char * args[], char * args2[]){ //examination the args array and check the second command
    int counter =0;
    for (int i = 0; i <MAX_ARGS ; ++i) { // walk all args array
        if(args[i] == NULL){ //if null the is no second command
            return;
        }
        if(strcmp(args[i],";")==0){ // if args has ; , the break and kepp the index number
            args[i]=NULL;//null the ;
            counter=i;
            break;
        }
    }
    for (int j = 0; j <MAX_ARGS ; ++j) { //allocate memory for args2 array
        args2[j] = calloc(50,sizeof(char));
    }

    int counter2 =0;
    for (int j = counter+1; j <MAX_ARGS ; ++j) {
        if(args[j] == NULL) break;
        strcpy(args2[counter2],args[j]); // copy the arguments after ;
        args[j]=NULL;
        counter2++;
    }
    args2[counter2] = NULL; //set null after last argumant

}

void run(char* args[], int* background){
    char  redirectionStatus[4] = "000\0"; //for keep any rediraction is done.First bit for std_in, second bit for std_out and last bit for std_err
    checkRedirection(args,redirectionStatus); // check the redirection
    int checkBuiltIn = checkCommandIsBuiltIn(args[0]); //check the command is built in.
    if(checkBuiltIn){
        switch(checkBuiltIn){ //find the which built-in command
            case 1 :
                execHistoryCommand(args);
                break;
            case 2 :
                execPathCommand(args);
                break;
            case 3:
                execFgCommand(args);
                break;
            case 4 :
                execExitCommand();
                break;
            case 5 :
                execJobs();
                break;
        }
    }else{
        pid_t childpid;
        childpid = fork(); // fork the child
        if (childpid == -1)  {
            perror("Failed to fork child");
            exit(-1);
        }

        if(childpid == 0){
            int check =setPath(args); //set the args array for ready to given execv.And check the command is exist or not.
            if(check == 1){
                execv(args[0],args);
                perror("Child failed to execv the command");
                exit(-1);
            }else{
                fprintf(stderr,"myshell: %s: command not found\n",args[0]);
            }
        }else{
            pushProcess(childpid,args[0]);
            if(*background == 0 ){
                waitpid(childpid,NULL,0);
                deleteProcess(childpid);
            }
        }
    }
    resetRedirection(redirectionStatus);

}
// check the redirection symbols and if neccesray do redirection and edit the redirectionStatus.
void checkRedirection(char * args[],char  redirectionStatus[]){
    for (int i = 0; i <sizeofArgs ; ++i) {
        if(args[i] != NULL) {
            if (strcmp(args[i], ">") == 0) { // if output redirection
                char *filename = args[i + 1];
                outputRedirection(O_TRUNC, filename); // do redirection with truncate option
                args[i] = NULL;
                args[i + 1] = NULL;
                redirectionStatus[1]='1'; //set the second bit to 1
            } else if (strcmp(args[i], ">>") == 0) { // output rediraction
                char *filename = args[i + 1];
                outputRedirection(O_APPEND, filename); // do redirection with append option
                args[i] = NULL;
                args[i + 1] = NULL;
                redirectionStatus[1]='1'; // set the second bit to 1
            } else if (strcmp(args[i], "<") == 0) { // if input redirection
                char *filename = args[i + 1];
                inputRedirection(filename);
                args[i] = NULL;
                args[i + 1] = NULL;
                redirectionStatus[0]='1'; // set the first bit to 1
            } else if (strcmp(args[i], "2>") == 0) {  // if the error redirection
                char *filename = args[i + 1];
                errorRedirection(filename);
                args[i] = NULL;
                args[i + 1] = NULL;
                redirectionStatus[2]='1'; // set the last bit to 1
            }
        }
    }
}

void outputRedirection(int choice, char* filename){ //output redirection with dup2.
#define CREATE_FLAGS (O_WRONLY | O_CREAT | choice)
    int fd;
    fd = open(filename, CREATE_FLAGS);// if choice is O_TRUNC clear file firs otherwise not clear file
    if (fd == -1) {
        perror("Failed to open");
        return;
    }
    tempStdOut = dup(1); // save default value
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("Failed to redirect standard output");
        return;
    }
    if (close(fd) == -1) {
        perror("Failed to close the file");
        return ;
    }
}
//input redirection with dup2.
void inputRedirection(char* filename){
    int fd;
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open");
        return;
    }
    tempStdOut = dup(0);
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("Failed to redirect standard input");
        return;
    }

    if (close(fd) == -1) {
        perror("Failed to close the file");
        return ;
    }

}
//error redirection with dup2.
void errorRedirection(char* filename){
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)

    int fd;
    fd = open(filename, CREATE_FLAGS);
    if (fd == -1) {
        perror("Failed to open");
        return;
    }
    tempStdOut = dup(2);
    if (dup2(fd, STDERR_FILENO) == -1) {
        perror("Failed to redirect standard error");
        return;
    }

    if (close(fd) == -1) {
        perror("Failed to close the file");
        return ;
    }

}

int checkCommandIsBuiltIn(char *command){ // If command is built-in return the built-in number
    for (int i = 0; i <builtInFunctionNumber ; ++i) {
        if(strcmp(command,builtInCommands[i]) == 0)
            return i+1;
    }
    return 0;
}

void execHistoryCommand(char * args[]){ //run the history command
    char *inputBuffer = calloc(MAX_LINE, sizeof(char));
    int background = 0;

    if(args[1] != NULL && strcmp(args[1],"-i") == 0 ){ // check the hitory command run with -i option or not

        if(args[2] == NULL){ // if -i entered and the number is not give display en error.
            fprintf(stderr,"Missing argument for history command.\n");
            return;
        }
        int index = atoi(args[2]); //convert string to integer

        inputBuffer = getHistoryCommand(index);
        if(index<0 || index > 9 || strcmp(inputBuffer,"")==0) fprintf(stderr,"Error : the given index not valid\n"); //check th index is valid
        else{
            setupWithoutRead(inputBuffer,args,&background); //call setup without read input from user because the input is known
            run(args,&background); //run command again
        }

    }else{ // if history given without input
        if(args[1] == NULL){
            printHistory(); // print the history.
        }else{
            fprintf(stderr,"Illegal argument for history command");
        }

    }
}

char* getHistoryCommand(int index){ //returns the command in the entered index of the linked list
    Command* currentCommand = commandHead;
    for (int i = 0; i <index ; ++i) {
        if(currentCommand->next != NULL){
            currentCommand = currentCommand->next;
        }else{
            return "";
        }
    }
    return currentCommand->input;

}

void printHistory(){ //print the latest 10 command.
    Command* currentCommand = commandHead;

    for (int i = 0; i <10 ; ++i) {
        if(currentCommand != NULL){
            printf("%d ",i);
            printf("%s",currentCommand->input);
            currentCommand = currentCommand->next;
        }
    }
}

void execPathCommand(char * args[]){
    if(args[1] == NULL){ // if run without  option just display Paths
        displayPaths();
    }else{
        if(args[2] == NULL){
            puts("Missing argument for path command");
            return;
        }
        if(strcmp(args[1] , "+")==0) pushPath(args[2]); // if option is + push the  given path
        else if(strcmp(args[1] , "-")==0) deletePath(args[2]);// if option is - delete the  given path
        else puts("Illgegal argument for path command.");
    }
}

void displayPaths(){ // display all path on path liked list
    Path * currentPath = pathHead;
    if(currentPath == NULL) return;

    while (currentPath->next != NULL){
        printf("%s:",currentPath->path);
        currentPath = currentPath->next;
    }
    printf("%s",currentPath->path);
    puts("");
}

void pushPath(char newPath[]){
    if(pathHead == NULL){
        pathHead = (Path*) malloc(sizeof(Path));
        strcpy(pathHead->path,newPath);
        return;
    }
    Path* newNode = (Path*) malloc(sizeof(Path));
    Path* currentPath = pathHead;

    while(currentPath->next != NULL){
        currentPath = currentPath->next;
    }
    strcpy(newNode->path,newPath);
    currentPath->next=newNode;
    newNode->next = NULL;
}

void deletePath(char *path){
    int deleteControl = 0;
    while(1){
        Path *currentPath = pathHead;
        Path *prevPath;

        if(currentPath != NULL && strcmp(currentPath->path,path)==0 ){
            pathHead = currentPath->next;
            free(currentPath);
            deleteControl = 1;
            continue;
        }

        while (currentPath != NULL && strcmp(currentPath->path,path) != 0 ){
            prevPath=currentPath;
            currentPath = currentPath->next;
        }

        if(currentPath == NULL ){
            if(deleteControl == 0) puts("The path do not include the given argument.");
            return;
        }

        prevPath->next = currentPath->next;
        free(currentPath);
        deleteControl = 1;
    }


}

void execFgCommand(char * args[]){
    if(args[1] != NULL){ //check the second argument is exist otherwise print error.
        char firstChar = args[1][0];
        if(firstChar != '%') goto error;
        args[1][0] = ' '; // clear % symbol and conver the integer
        int pid = atoi(args[1]);

        if(isProcessAlive(pid) == 1){ // check the process is alive with given pid.
            printf("Parent are will waiting %d process id\n",pid);
            waitpid(pid,NULL,0); // if process is exist wait the given process to terminate
        }else{ // if there is no process with given pid print error message.
            fprintf(stderr,"There is no background process with %d process id\n",pid);
        }
    }else{
        error:
        fprintf(stderr,"İnvalid usage of fg.\n");
    }
}

int isProcessAlive(pid_t pid){ // check the process with givn id is live or not
    Process * currenProcess = processHead;
    for (int i = 0; i <processCounter ; ++i) {
        if(currenProcess->processID == pid ){
            return 1;
        }
        currenProcess= currenProcess->next;
    }

    return 0;
}

void execExitCommand(){
    if(processCounter != 0){
        fprintf(stderr,"There are %d process on background\n",processCounter);
    } else{
        exit(0);
    }
}

void execJobs(){ //print the processes currently run.
    if(processHead != NULL){
        Process * currentProces = processHead;
        printf("     Pid    Process Name\n");
        for(int i = 0; i <processCounter ; ++i) {
            printf("[%d]  ",i+1);
            printf("%d    %s\n",currentProces->processID,currentProces->command);
            currentProces = currentProces->next;
        }
    }

}

int setPath(char * args[]) {
    for (int j = 0; j < sizeofPaths; ++j) {
        if (checkCommandisExist(envPaths[j], args[0]) == 1) { //check the command is exit on this path variable
            char *newPath = (char *) malloc(sizeof(char) * 70);
            strcpy(newPath, envPaths[j]);
            strcat(newPath, "/");
            strcat(newPath, args[0]);
            args[0] = calloc(70, sizeof(char));
            strcpy(args[0], newPath);
            strcat(args[0],"\0");
            return 1;
        }
    }

    return 0;
}

int checkCommandisExist(char * path, char * command) {
    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(path);

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
        return 0;
    }

    // for readdir()
    while ((de = readdir(dr)) != NULL){
        if(strcmp(command,de->d_name) == 0 ){
            return 1;
        }
    }
    closedir(dr);
    return 0;
}

void pushProcess(pid_t newpProcessID, char* command){
    processCounter ++; // add 1 to processCounter
    if(processHead == NULL){
        processHead = (Process*) malloc(sizeof(Process));
        processHead->processID=newpProcessID;
        strcpy(processHead->command,command);
        return;
    }
    Process* newProcess = (Process*) malloc(sizeof(Process));
    Process* currentProcess = processHead;

    while(currentProcess->next != NULL){
        currentProcess = currentProcess->next;
    }
    newProcess->processID = newpProcessID;
    strcpy(newProcess->command,command);
    currentProcess->next=newProcess;
    newProcess->next = NULL;
}

void deleteProcess(pid_t processID){
        Process *currentProcess = processHead;
        Process *prevProcess =NULL;

        if(currentProcess != NULL && currentProcess->processID==processID ){
            processHead = currentProcess->next;
            free(currentProcess);
            processCounter--;
            return;
        }
        while (currentProcess != NULL && currentProcess->processID!= processID ){
            prevProcess=currentProcess;
            currentProcess = currentProcess->next;
        }

        if(currentProcess == NULL ){
            return;
        }

        prevProcess->next = currentProcess->next;
        free(currentProcess);
        processCounter--;
};

void resetRedirection(char  redirectionStatus[]){//reset the redirections to default values
    if(redirectionStatus[0] == '1'){
        fflush(stdin);
        dup2(tempStdIn,STDIN_FILENO);
    }
    if(redirectionStatus[1] == '1') {
        fflush(stdout);
        dup2(tempStdOut,STDOUT_FILENO);
    }
    if(redirectionStatus[2] == '1') {
        fflush(stderr);
        dup2(tempStdErr,STDERR_FILENO);
    }
}

int main(void)
{
    char *inputBuffer; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    int background2;
    char *args[MAX_ARGS]; /*command line arguments */
    char *args2[MAX_ARGS]; //keep for second command at one line

    //I allocate memory and clean my args array
    for (int j = 0; j <MAX_ARGS ; ++j) {
        args[j] = calloc(50,sizeof(char));
    }
    args2[0]=NULL; //args2 firs arg initially null


    setEnvPathVariables(envPaths); //fill envPAth array with env path variables
    setBuiltInCommands(); //fill builtInCommand array with built-in command Names

    addProcessKillSignal(); //Process kill signal is to move in when a process send SIGCHLD.
    addCtrlZsignal(); // when user press ctrl-z and SIGTSTP send.



    while(1) {
        inputBuffer = calloc(MAX_LINE, sizeof(char)); // clear the input buffer
        background = 0;//set zero again background varşables
        background2 = 0;
        printf("myshell > ");
        fflush(stdout); //clear buffers
        fflush(stdin);
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background,&background2);
        if(args[0] == NULL) continue; //if no command is entered the shell restart loop.
        designArgs(args,args2);//check args for double command
        run(args,&background);
        if(args2[0] != NULL) run(args2,&background2);
    }

        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,

        otherwise it will invoke the setup() function again. */




}

