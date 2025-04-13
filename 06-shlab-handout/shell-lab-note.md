# 任务要求
实现一个shell, 填充tsh.c中的函数

[writeup](http://csapp.cs.cmu.edu/3e/shlab.pdf)

一打眼看writeup不明白是干啥的，需要先把书上第8章内容过一遍[important]

## 需要实现的函数
- eval: Main routine that parses and interprets the command line. [70 lines]
- builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [25 lines]
- do bgfg: Implements the bg and fg built-in commands. [50 lines]
- waitfg: Waits for a foreground job to complete. [20 lines]
- sigchld handler: Catches SIGCHILD signals. [80 lines]
- sigint handler: Catches SIGINT (ctrl-c) signals. [15 lines]
- sigtstp handler: Catches SIGTSTP (ctrl-z) signals. [15 lines]

## 测试方法
```unix
make test01 # tsh测试trace01.txt
make rtest01 # tshref测试trace01.txt
```

## tsh的功能
如果第一个word是builtin command,则执行相应的函数；否则，该单词将被认为是可执行程序的路径名。在这种情况下，shell 会创建一个子进程，然后在子进程的上下文环境中加载并运行程序。由于解释一条命令行而创建的子进程统称为一个job。一般来说，一个job可以由多个通过 Unix 管道连接的子进程组成。

如果命令行以符号 “&” 结尾，那么该作业将在后台运行，这意味着 shell 在打印提示符并等待下一条命令行之前，不会等待该作业结束。否则，该作业将在前台运行，这意味着 shell 会等待该作业结束后才会等待下一条命令行。因此，在任何时刻，最多只有一个作业可以在前台运行。不过，在后台可以运行任意数量的作业。


# 需要支持的builtin 指令

tsh should support the following built-in commands:

- The quit command terminates the shell.

- The jobs command lists all background jobs.

- The bg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the background. The <job> argument can be either a PID or a JID.

- The fg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the foreground. The <job> argument can be either a PID or a JID.

## update builtin_cmd()

```c
int builtin_cmd(char** argv)
{

    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    else if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }
    else if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    return 0; /* Not a builtin command */
}
```

# 转发SIGINT\SIGTSTP信号
需要转发SIGINT\SIGTSTP信号给fg job, fg job会回收子进程

> When you implement your signal handlers, be sure to send SIGINT and SIGTSTP signals to the entire foreground process group, using ”-pid” instead of ”pid” in the argument to the kill function.
> 在用kill()发送信号时，如果发送 SIGINT or SIGTSTP, 用`-pid`

## udpate sigint_handler()

```c
void sigint_handler(int sig)
{
    pid_t pid = fgpid(jobs);
    if (pid > 0) kill(-pid, SIGINT);
    return;
}
```
## update sigtstp_handler()

```c
void sigtstp_handler(int sig)
{
    pid_t pid = fgpid(jobs);
    if (pid > 0) kill(-pid, SIGTSTP);
    return;
}
```
## update sigchld_handler()
这个函数处理接收到SIGSTOP\SIGTSTP信号的，回收所有子进程

> 在测试trace过程中调节输出语句，以达到和tshref一样的效果

```c
void sigchld_handler(int sig)
{
    pid_t pid;
    int   status;

    if (verbose) printf("sigchld_handler: entering \n");

    /*
     * Reap any zombie jobs.
     * The WNOHANG here is important. Without it, the
     * the handler would wait for all running or stopped BG
     * jobs to terminate, during which time the shell would not
     * be able to accept input.
     */
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        }
        else if (WIFSIGNALED(status)) {
            deletejob(jobs, pid);

            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status)) {
            getjobpid(jobs, pid)->state = ST;
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
        }
    }

    /*
     * Check for normal loop termination.
     * This is a little tricky. For our purposes,
     * the waitpid loop terminates normally for one of
     * two reasons: (a) there are no children left
     * (pid == -1 and errno == ECHILD) or (b) there are
     * still children left, but none of them are zombies (pid == 0).
     */
    if (!((pid == 0) || (pid == -1 && errno == ECHILD))) unix_error("sigchld_handler wait error");

    if (verbose) printf("sigchld_handler: exiting\n");

    return;
}
```
# update eval()

## hint
In eval, the parent must use sigprocmask to block SIGCHLD signals before it forks the child, and then unblock these signals, again using sigprocmask after it adds the child to the job list bycalling addjob. Since children inherit the blocked vectors of their parents, the child must be sure to then unblock SIGCHLD signals before it execs the new program.
> 在eval函数中，父进程必须在创建子进程（调用fork）之前使用sigprocmask函数来阻塞SIGCHLD信号，然后在通过调用addjob函数将子进程添加到作业列表之后，再次使用sigprocmask函数来解除对这些信号的阻塞。由于子进程会继承其父进程的阻塞信号集，所以子进程在执行新程序（调用exec）之前，必须确保解除对SIGCHLD信号的阻塞。

## 为什么要将子进程添加到别的进程组？
writeup hint中有一条：
> When you run your shell from the standard Unix shell, your shell is running in the foreground process
group. If your shell then creates a child process, by default that child will also be a member of the
foreground process group. Since typing ctrl-c sends a SIGINT to every process in the foreground
group, typing ctrl-c will send a SIGINT to your shell, as well as to every process that your shell
created, which obviously isn’t correct.
> 
> 子进程和父进程属于同一个进程组，如果ctrl-c会终止fg进程组下所有进程，包括当前shell运行的进程，这显然是不对的
> 
> Here is the workaround: After the `fork`, but before the `execve`, the child process should call
setpgid(0, 0), which puts the child in a new process group whose group ID is identical to the
child’s PID. This ensures that there will be only one process, your shell, in the foreground process
group. When you type ctrl-c, the shell should catch the resulting SIGINT and then forward it
to the appropriate foreground job (or more precisely, the process group that contains the foreground
job).
> 
> 为了解决上述问题，提出一个workaround方法，将新创建的子进程放入一个新的进程组。确保fg进程组中只有一个进程——your shell。当按下ctrl-c时，shell会捕获生成的SIGINT信号，然后转发它到相应的前台作业（或者更准确地说，包含fg job的进程组。
> 
> 实现方法就是在`fork`之后`execve`之前，子进程调用`setpgid(0, 0)`

## 如何避免父进程和子进程并发问题？

仔细阅读8.5.6章节内容，描述了一个典型案例

## eval()
```c
void eval(char* cmdline)
{
    char* argv[MAXARGS]; /* Argument list execve() */
    char  buf[MAXLINE];  /* Holds modified command line */
    int   bg;            /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL) return; /* Ignore empty lines */

    sigset_t mask_all, mask_one, prev_one;
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);

    if (!builtin_cmd(argv)) {
        sigprocmask(SIG_BLOCK, &mask_one, &prev_one); /* block SIGCHLD*/

        if ((pid = fork()) == 0) { /* Child runs user job */
            setpgid(0, 0);         /* set child process to another group */
            if (execve(argv[0], argv, environ) < 0) {
                sigprocmask(SIG_SETMASK, &prev_one, NULL); /* Unblock SIGCHLD */
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        sigprocmask(SIG_BLOCK, &mask_one, NULL);
        addjob(jobs, pid, bg ? BG : FG, cmdline);
        sigprocmask(SIG_SETMASK, &prev_one, NULL);
        /* Parent waits for foreground job to terminate */
        if (!bg) {
            waitfg(pid);
        }
        else {
            printf("%d %s", pid, cmdline);
        }
    }
    return;
}

```

# do_bgfg()
需要实现bg fg命令,功能就是restart job

> bg/fg后面输入是job id或者process id,如果是带前缀`%`的，则表示job id，否则表示process id.

问题：
- 如何唤醒某个job? -- `kill(pid, SIGCONT)`
- 需要考虑的异常情况？
  - bg/fg后面的参数判断: 是否为数字，是否为`%`开头的数字，是否为`%`开头的数字且在jobs中存在

## 遇到的问题

1. 判断字符是否是数字？-- `isdigit(char c)`
2. 字符串转换为数字？-- `atoi(const char *str)`

## update do_bgfg()

```c
void do_bgfg(char** argv)
{
    char*         cmd = argv[0];
    int           pid;
    int           jid;
    struct job_t* jobp;

    if (strcmp(cmd, "bg") && strcmp(cmd, "fg")) return;

    /* ignore command if no argument */
    if (argv[1] == NULL) {
        printf("%s command needs PID argument\n", cmd);
        return;
    }

    if (argv[1][0] == '%') {
        if ((argv[1] + 1) == NULL) {
            printf("%s command required PID or %%jobid argument\n", cmd);
            return;
        }
        if (!isdigit(argv[1][1])) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
        jid = atoi(argv[1] + 1);

        if (jid > maxjid(jobs)) {
            printf("%%%d: No such job\n", jid);
            return;
        }

        jobp = getjobjid(jobs, jid);
        pid  = jobp->pid;
    }
    else {
        if (argv[1] == NULL) {
            printf("%s command required PID or %%jobid argument\n", cmd);
            return;
        }
        if (!isdigit(argv[1][0])) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
        pid = atoi(argv[1]);
        jid = pid2jid(pid);
        if (jid == 0) {
            printf("(%d) No such process\n", pid);
            return;
        }

        jobp = getjobpid(jobs, pid);
    }

    if (jobp != NULL) {
        if (!strcmp(cmd, "bg")) {
            kill(pid, SIGCONT);
            updatejob(jobs, pid, BG);
            printf("[%d] (%d) %s", jid, pid, jobp->cmdline);
        }
        if (!strcmp(cmd, "fg")) {
            kill(pid, SIGCONT);
            updatejob(jobs, pid, FG);
            waitfg(pid);
        }
    }
    else
        printf("Job %d not found\n", pid);

    return;
}
```