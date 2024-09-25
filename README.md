# Minishell

This program is the replication of a shell. The main loop of this function
prints the current working directory (`man 2 getcwd`), and then waits for user input. 

The program will be called from the command line and will take no arguments. 
The prompt always displays the current working directory in square brackets
followed immediately by a $ and a space.

    $ ./minishell

    [/home/user/minishell]$


1. `cd`

    If `cd` is called with no arguments or the one argument ~, it
    changes the working directory to the user's home directory.
    
    If `cd` is called with one argument that isn't ~, it attempts to open the directory specified in that argument.
    It also supports ~ with trailing directory names, as in

        cd ~/Desktop
        
    cd also supports changing into a directory whose name contains spaces. 
    Your shell would have to support enclosing the direcotry name in double quotes, as in

        cd "some folder name"
        cd "some"" folder ""name"

    If a quote is missing as in

        cd "Desktop

    it should prints an error message and returns `EXIT_FAILURE`.

2. `exit`

    The `exit` command causes the shell to terminate and return
    `EXIT_SUCCESS`. Using `exit` is the only way to stop shell's
    execution normally. 


All other commands will be executed using `exec`. When an unknown
command is entered, the program forks. The child program will exec
the given command and the parent process will wait for the child
process to finish before returning to the prompt.