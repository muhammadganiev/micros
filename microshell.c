#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

static int write_error(char *str, char *arg)
{
    if (str)
    {
        while (*str)
        {
            write(2, str++, 1);
        }
    }
    if (arg)
    {
        while (*arg)
        {
            write(2, arg++, 1);
        }
    }
    write(2, "\n", 1);
    return (1);
}

void    microshell_cd(char **argv, int i)
{
    if (i != 2)
        write_error("error: cd: bad arguments", NULL);
    else if (chdir(argv[1]) != 0)
        write_error("error: cd: cannot change directory to ", argv[1]);
}

static int sys_exec(char **argv, char **env, int temp, int i)
{
    argv[i] = NULL;
    dup2(temp, STDIN_FILENO);
    close (temp);
    execve(argv[0], argv, env);
    return(write_error("error: cannot execute ", argv[0]));
}

int microshell_fork(char **argv, char **env, int temp, int i)
{
    if (fork() == 0)
    {
        if (sys_exec(argv, env, temp, i))
            return (1);
    }
    else
    {
        close(temp);
        while (waitpid(-1, NULL, WUNTRACED) != -1)
            ;
    }
    return (0);
}

int microshell_pipe(char **argv, char **env, int temp, int *fd, int i)
{
    pipe(fd);
    if (fork() == 0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (sys_exec(argv, env, temp, i))
            return (1);
    }
    else
    {
        close(fd[1]);
        close(temp);
    }
    return (0);
}

int main(int argc, char **argv, char **env)
{
    int i = 0;
    int temp = dup(STDIN_FILENO);
    int fd[2];
    (void)argc;
    while (argv[i] && argv[i + 1])
    {
        argv = &argv[i] + 1;
        i = 0;
        while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
            i++;
        if (strcmp(argv[0], "cd") == 0)
            microshell_cd(argv, i);
        else if (i != 0 && (argv[i] == NULL || strcmp(argv[i], ";") == 0))
        {
            if (microshell_fork(argv, env, temp, i) != 0)
                return (1);
            else
                temp = dup(STDIN_FILENO);
        }
        else if (i != 0 && strcmp(argv[i], "|") == 0)
        {
            if (microshell_pipe(argv, env, temp, fd, i) != 0)
                return (1);
            else
                temp = fd[0];
        }
    }
    close(temp);
    return (0);
}