#include <stdio.h>
#include "exec.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include "jobs.h"

static pid_t SHELL_PGID = -1;
void exec_set_shell_pgid(pid_t pgid){
    SHELL_PGID = pgid;
    return;
}

int run_simple_foreground(struct command *cmd, const char *cmdline) {
    if (!cmd->argv || !cmd->argv[0] || cmd->has_pipe || cmd->background) {
        putchar('\n');
        return 0;
    }

    pid_t pid = fork();
    if(pid < 0){
        //For some reason if the fork fails, we will just return 0
        putchar('\n');
        return 0;
    }

    if(pid == 0){
        //This if branch is for the child process (pid == 0)
        setpgid(0, 0); //put child in its own process group

        signal(SIGINT, SIG_DFL); //reset so crtl c/z exits the child process
        signal(SIGTSTP, SIG_DFL);

        if(cmd->infile != NULL){
            int fd = open(cmd->infile,O_RDONLY);
            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDIN_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        if(cmd->outfile != NULL){
            int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 0644 same as S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH

            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDOUT_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        if(cmd->errfile != NULL){
            int fd = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDERR_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);

        //if we reach here then excevp has failed and we exit the child process
        _exit(0);
    }

    //Parent side (implied pid > 0):
    //Double check and ensure child's PGID is set (to protect against race conditions)
    // If the parent code runs before the child executes a single instruction, the child wouldn't
    // set its own pgid(0,0) so this check is for that 
    setpgid(pid,pid);
    //give terminal control to the child's process group
    tcsetpgrp(STDIN_FILENO, pid);

    // Else pid > 0, we are in the parent process:
    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) { 
        putchar('\n'); 
        return 1; 
    }

    if (WIFSTOPPED(status)) {
        // record this stopped fg command as a job
        pid_t pids[1] = { pid };
        jobs_add(pid, pids, 1, cmdline, STOPPED);
    }

    tcsetpgrp(STDIN_FILENO, SHELL_PGID);

    putchar('\n');
    return 1;
}

int run_single_pipeline(struct command *cmd) {
    if (!cmd->has_pipe || !cmd->argv || !cmd->argv[0] || !cmd->pipe_argv || !cmd->pipe_argv[0]) {
        putchar('\n');
        return 0;
    }

    //If background exists, exit since pipeline + background isn't supported
    if (cmd->background) {
        putchar('\n');
        return 0;
    }

    int fd[2];
    if(pipe(fd) < 0){
        //if pipe somehow return error we exit
        putchar('\n');
        return 0;
    }
    
    //Left side of pipe:
    pid_t left = fork();
    if (left < 0) {
        //if fork fails we exit
        close(fd[0]); 
        close(fd[1]);
        putchar('\n');
        return 0;
    }

    if (left == 0) {
        // signals switch to default
        setpgid(0,0);
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        //If left side in file exists, do this part like normal
        if (cmd->infile) {
            int fdin = open(cmd->infile, O_RDONLY);
            if (fdin < 0) _exit(0);
            if (dup2(fdin, STDIN_FILENO) < 0) _exit(0);
            close(fdin);
        }

        if (cmd->outfile) {
            int fdout = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fdout < 0) _exit(0);
            if (dup2(fdout, STDOUT_FILENO) < 0) _exit(0);
            close(fdout);
        } else {
            if (dup2(fd[1], STDOUT_FILENO) < 0) _exit(0);
        }

        if (cmd->errfile) {
            int fde = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fde < 0) _exit(0);
            if (dup2(fde, STDERR_FILENO) < 0) _exit(0);
            close(fde);
        }

        close(fd[0]);
        close(fd[1]);

        execvp(cmd->argv[0], cmd->argv);
        _exit(0);
    }

    //Right side of pipe:
    pid_t right = fork();
    if (right < 0) {
        // if second fork fails, close fds, wait for left, and exit
        close(fd[0]); 
        close(fd[1]);
        int st; 
        waitpid(left, &st, 0);
        putchar('\n');
        return 0;
    }

    if (right == 0) {
        setpgid(0, left);
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        
        //On the right side (input side), the file takes  priority over pipe
        if (cmd->pipe_infile) {
            int fdin = open(cmd->pipe_infile, O_RDONLY);
            if (fdin < 0) _exit(0);
            if (dup2(fdin, STDIN_FILENO) < 0) _exit(0);
            close(fdin);
        } else {
            if (dup2(fd[0], STDIN_FILENO) < 0) _exit(0);
        }

        if (cmd->pipe_outfile) {
            int fdout = open(cmd->pipe_outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fdout < 0) _exit(0);
            if (dup2(fdout, STDOUT_FILENO) < 0) _exit(0);
            close(fdout);
        }

        if (cmd->pipe_errfile) {
            int fde = open(cmd->pipe_errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fde < 0) _exit(0);
            if (dup2(fde, STDERR_FILENO) < 0) _exit(0);
            close(fde);
        }

        close(fd[0]);
        close(fd[1]);

        execvp(cmd->pipe_argv[0], cmd->pipe_argv);
        _exit(0);
    }
    
    close(fd[0]);
    close(fd[1]);

    // race proof conditions
    setpgid(left,  left);
    setpgid(right, left);

    // hand terminal control to the child job’s pgid (which is left pid)
    tcsetpgrp(STDIN_FILENO, left);

    //parent  needs to wait for both children
    int statusLeft, statusRight;
    waitpid(left,  &statusLeft,  WUNTRACED);
    waitpid(right, &statusRight, WUNTRACED);

    // restore shell control
    tcsetpgrp(STDIN_FILENO, SHELL_PGID);

    putchar('\n');
    return 1;
}

int run_single_background(struct command *cmd, const char *cmdline){
    //only single commands that aren't apart of a pip can be backgrounded
    if(!cmd->argv || !cmd->argv[0] || cmd->has_pipe){
        putchar('\n');
        return 0;
    }

    pid_t pid = fork();
    if(pid<0) {
        putchar('\n');
        return 0;
    }

    if(pid == 0){
        //child process
        setpgid(0, 0);

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if(cmd->infile != NULL){
            int fd = open(cmd->infile,O_RDONLY);
            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDIN_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        if(cmd->outfile != NULL){
            int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 0644 same as S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH

            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDOUT_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        if(cmd->errfile != NULL){
            int fd = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if(fd < 0){
                _exit(0); //if the file is missing, we exit
            }
            if(dup2(fd, STDERR_FILENO) < 0){
                _exit(0);
            }
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        //if we reach here then excevp has failed and we exit the child process
        _exit(0);
    }

    //parent (pid > 0)
    setpgid(pid, pid); // race safety line

    // we don't want to give terminal control to this process since its background
    // we don't want to wait either, we want to add jobs to the table as RUNNING (in background)
    pid_t pids[1] = { pid };
    jobs_add(pid, pids, 1, cmdline, RUNNING);

    putchar('\n');
    return 1;
}