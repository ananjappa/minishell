#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT    "\x1b[0m"

#define CMDLN_MAX 4096
#define TOKEN_MAX 2048

volatile sig_atomic_t signal_val = 0;

void catch_sig(int sig_num) {
    signal_val = sig_num;
}

void freer(char **argv, int *argc) {
    for(int i = 0; i < *argc; i++) {
	free(argv[i]);
    }
    free(argv);
}

char **get_args(char **argv, int *argc){
    // Getting command line arguments for execution.
    char cmdln[CMDLN_MAX];    
    if(fgets(cmdln, sizeof(cmdln), stdin) == NULL) {
        if(errno == EINTR) {
	    errno = 0;
	    //freer(argv, argc);
	    signal_val = 0;
	    return NULL;
	} else if(feof(stdin)) {
	    printf("\n");
	    exit(EXIT_SUCCESS);
        } else {	    
	    fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
	    exit(EXIT_FAILURE);
        }
    }
    // Getting rid of newline
    char *zocmd = strchr(cmdln, '\n');
    if(zocmd != NULL) {
	  *zocmd = '\0';  
    }
    // Getting rid of added space at end of cmd line
    size_t cmd_len = strlen(cmdln);
    int i = 1;
    while(cmdln[cmd_len - i] == 32) {
	    cmdln[cmd_len - i] = '\0';
	    i++;
    }
    // Getting rid of added space at beginning of cmd line
    char *cmdptr = &cmdln[0];
    while(*cmdptr == 32) {
	    cmdptr = (cmdptr + 1);
    }
    // Sepearting each argument of cmd line into sepearte arg string(args)
    if((argv[0] = (char *)malloc(sizeof(char)*CMDLN_MAX)) == NULL) {
	    fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
	    free(argv[0]);
	    free(argv);
	    return NULL;
    }
    int arg = 0, sc = 0, q_check = 0;
    for(int i = 0; i <= strlen(cmdptr); i++) {
	char curr = *(cmdptr + i);
	if(q_check == 0 && curr == 34) {
		q_check = 1;
	} else if(q_check == 1 && curr == 34) {
		q_check = 0;
	}
	if(curr == 32 && q_check != 1) {
	    if(sc == 0) {
		char *pholder = argv[*argc] + arg;
	    	*pholder = '\0';
		if((argv[++(*argc)] = (char *)malloc(sizeof(char)*CMDLN_MAX)) == NULL) {
		    fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
		    freer(argv, argc);
		    return NULL;
		} 
		arg = 0;
		sc = 1;
	    }
	    continue;
	} else if(curr == 0) {
		char *pholder = argv[(*argc)++] + arg;
		*pholder = '\0';
		return argv;
	}
	sc = 0;	
	char *pholder = argv[*argc] + arg;
	*pholder = curr;
	arg++;
    }
    //*argv[*argc] = 0;
    return argv;
}

char **args_format(char ** argv, int *argc) {
    // Removing quotes from arguments and making sure quotes match
    for(int i = 0; i < *argc; i++) {
        char *quote = argv[i], *tilde, *q1;
	int numq = 0, count = 0;
	tilde = strchr(quote, '~');
	q1 = strchr(quote, 34);
	
	// Counting number of quotes
	while((quote = strchr(quote, 34)) != NULL) {
	    numq++;
	    quote = quote + 1;	
	}
	if((q1 == NULL || tilde < q1) && (tilde - argv[i] == 0)) {
	    uid_t uid = getuid();
            struct passwd *pwd;
            if((pwd = getpwuid(uid)) == NULL) {
	        fprintf(stderr, "Error: cannot get passwd entry. %s.\n", strerror(errno));
		freer(argv, argc);
		return NULL;
	    }
    
	    char *hdir = pwd->pw_dir, *str2 = NULL;
	    if(*(argv[i] + 1)) {
	        str2 =  argv[i] + 1;
	    } else {
	        str2 = "\0";
	    }
	    strcat(hdir, str2);
	    strcpy(argv[i], hdir);
	} 	
	
	// Erroring due to odd num of quotes or removing quotes
	if((numq%2) == 1) {
	    fprintf(stderr, "Error: Malformed command.\n");
	    freer(argv, argc);
	    return NULL;
	} else if(numq != 0) {
	    for(int j = 0; j < strlen(argv[i]); j++) {
		char curr = *(argv[i] + j);
		if(curr != 34) {
		    *(argv[i] + count) = curr;	
		    count++;    
		}
	    }
	    // Null termination
	    *(argv[i] + count) = '\0';	
	}
    }

    return argv;
}

int main(){	    
    //catch signal
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = catch_sig;
    if(sigaction(SIGINT, &sig, NULL) == -1) {
	    fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
	    return EXIT_FAILURE;
    }

    while(true) { 
	// Start up ./minishell 
        char buf[PATH_MAX];
        if(getcwd(buf, PATH_MAX) == NULL) {
	    fprintf(stderr, "Error: Cannot get working directory. %s",strerror(errno));
            return EXIT_FAILURE;
        }
        printf("[%s%s%s]$ ", BRIGHTBLUE, buf, DEFAULT);
	fflush(stdout);
        char **argv;
        if((argv = (char **)malloc(CMDLN_MAX*sizeof(char *))) == NULL) {
	        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
		free(argv);
		continue;
        }
        int argc = 0;
        if((argv = get_args(argv, &argc)) == NULL) {
	        printf("\n");
		fflush(stdout);
		free(argv);
		signal_val = 0;
		continue;
        }
        if((argv = args_format(argv, &argc)) == NULL) {
		continue;
	}
        pid_t pid = getpid();
	// cd command
        if(strcmp(argv[0], "cd") == 0) {
	    if(argc > 2) {
	        fprintf(stderr, "Error: Too many arguments to cd\n");
            } else if(argc == 1) {
                uid_t uid = getuid();
                struct passwd *pwd;
                if((pwd = getpwuid(uid)) == NULL) {
		    fprintf(stderr, "Error: cannot get passwd entry. %s.\n", strerror(errno));
		}
		if(chdir(pwd->pw_dir) == -1) {
		    fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", pwd->pw_dir, strerror(errno));
		}
	    } else {
		if(chdir(argv[1]) == -1) {
		    fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", argv[1], strerror(errno));
		}
	    }	    
        } else if(strcmp(argv[0], "exit") == 0) {
	    freer(argv, &argc);
	    break;
        } else if((pid = fork()) == -1) {
	    fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
	} 
	
	if(pid == 0) { 
	    if(*argv[0] == 0) {
		freer(argv, &argc);
		return EXIT_SUCCESS;
	    } else if(argc == 1) {
		if(execlp(argv[0], argv[0], (char *)0) == -1) {
		    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
		    freer(argv, &argc);
		    return EXIT_FAILURE;
	        }
	    } else {
	    	if(execvp(argv[0], argv) == -1) {
	            fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
		    freer(argv, &argc);
		    return EXIT_FAILURE; 
		}
	    }
        }
	if(wait(NULL) == -1 && pid != getpid()) {
		if(errno == EINTR) {
		    if(waitpid(pid, NULL, 0) == -1) {
			fprintf(stderr, "Error: wait() failed. %s\n", strerror(errno));
		    }
		    printf("\n");
		    fflush(stdout);
		    freer(argv, &argc);
		    errno = 0;
		    signal_val = 0;
		} else {   
		    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
		}
		continue;
		
	}
        freer(argv, &argc);
    } 
    return EXIT_SUCCESS;

}
