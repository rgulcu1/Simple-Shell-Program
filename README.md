# Simple Shell Program

This program written by C. This shell support standart shell commands and some built-in commands.

## Built-in Commands

### * history  
see the list of and execute the last 10 commands. See the following example for the use of these commands.
history -i num, we can execute the command at num index

### * ^Z  
Stop the currently running foreground process, as well as any descendants of that process (e.g., any child processes that it forked). If there is no foreground process, then the signal should have no effect.

### * path 
path is a command not only to show the current pathname list, but also to append or remove several pathnames. In your shell implementation, you may keep a variable or data
structure to deal with pathname list (referred to as "path" variable below). This list helps searching for executables when users enter specific commands.

* path (without arguments) - displays the pathnames currently set. It should show pathnames separated by colons. ex) "/bin:/usr/bin"

* path + /foo/bar - appends the pathname to the "path" variable. Only one pathname will be given to each invocation of the path command. It's okay to add a pathname to the "path" variable even if it already contains the same pathname, i.e., duplicates in the "path" variable are fine.

* path - /foo/bar - removes the pathname from the "path" variable. It shouldremove all duplicates of the given pathname. Only one pathname will be given to each invocation of the path command.

### *fg %num 
Move the background process with process id num to the background.. Note that for this, you have to keep track of all the background processes.

### * exit 
Terminate your shell process. If the user chooses to exit while there are background processes, notify the user that there are background processes still running and do not terminate the shell process unless the user terminates all background processes.

### * jobs 
Print the list of program id and program names currently working backgrounda processes.
