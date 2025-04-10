# 任务要求
实现一个shell, 填充tsh.c中的函数

[writeup](http://csapp.cs.cmu.edu/3e/shlab.pdf)

一打眼看writeup不明白是干啥的，需要先把书上第8章内容过一遍[important]

需要实现的函数有：
```c
/* Here are the functions that you will implement */
void eval(char* cmdline);
int  builtin_cmd(char** argv);
void do_bgfg(char** argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
```

我写了一个test.sh文件，用来同时运行tsh和tshref，方便比较
```unix
./test 01 # 测试trace01.txt
```

# trace01.txt


直接运行test.sh，然后查看结果对比
```unix
unix$ ./test.sh 01
---------------test tsh-------------

./sdriver.pl -t trace01.txt -s ./tsh -a "-p"
#
# trace01.txt - Properly terminate on EOF.
#
---------------test tshref-------------

#
# trace01.txt - Properly terminate on EOF.
#

```

# trace02.txt & trace03.txt


需要执行quit内建指令

## 需要支持的builtin 指令
tsh should support the following built-in commands:
– The quit command terminates the shell.
– The jobs command lists all background jobs.
– The bg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the background. The <job> argument can be either a PID or a JID.
– The fg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the foreground. The <job> argument can be either a PID or a JID.

## 完善eval和builtin_cmd函数
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

    if (!builtin_cmd(argv)) {
        if ((pid = fork()) == 0) { /* Child runs user job */
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) {
            int status;
            if (waitpid(pid, &status, 0) < 0) unix_error("waitfg: waitpid error");
        }
        else
            printf("%d %s", pid, cmdline);
    }
    return;
}
```

```c
int builtin_cmd(char** argv)
{

    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    else if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) { /* Ignore singleton & */
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

## 测试结果
```unix
unix$ ./test.sh 02
---------------test tsh-------------

./sdriver.pl -t trace02.txt -s ./tsh -a "-p"
#
# trace02.txt - Process builtin quit command.
#
---------------test tshref-------------

#
# trace02.txt - Process builtin quit command.
#
```

```
./sdriver.pl -t trace03.txt -s ./tsh -a "-p"
#
# trace03.txt - Run a foreground job.
#
tsh> quit
---------------test tshref-------------

#
# trace03.txt - Run a foreground job.
#
tsh> quit
```

# trace04.txt

在trace03的基础上依然运行通过

# trace05.txt
在输入jobs命令会输出jobs列表

注意到`parseline`函数中， `Return true if the user has requested a BG job, false if the user has requested a FG job.`

所以需要进一步调整eval函数
## update eval函数

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

    if (!builtin_cmd(argv)) {
        if ((pid = fork()) == 0) { /* Child runs user job */
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) {
            int status;
            if (waitpid(pid, &status, 0) < 0) unix_error("waitfg: waitpid error");
        }
        else {
            addjob(jobs, pid, bg ? BG : FG, cmdline); //增加这一句！
            printf("%d %s", pid, cmdline);
        }
    }
    return;
}
```

## 测试结果

```unix
---------------test tsh-------------

./sdriver.pl -t trace05.txt -s ./tsh -a "-p"
#
# trace05.txt - Process jobs builtin command.
#
tsh> ./myspin 2 &
41012 ./myspin 2 &
tsh> ./myspin 3 &
41014 ./myspin 3 &
tsh> jobs
[1] (41012) Running ./myspin 2 &
[2] (41014) Running ./myspin 3 &
---------------test tshref-------------

#
# trace05.txt - Process jobs builtin command.
#
tsh> ./myspin 2 &
41062 ./myspin 2 &
tsh> ./myspin 3 &
41064 ./myspin 3 &
tsh> jobs
[1] (41062) Running ./myspin 2 &
[2] (41064) Running ./myspin 3 &
```

# trace06.txt ~ trace08.txt

需要发送SIGINT信号给前台信号

> When you implement your signal handlers, be sure to send SIGINT and SIGTSTP signals to the entire foreground process group, using ”-pid” instead of ”pid” in the argument to the kill function.

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
当按下

# trace09.txt
需要实现bg fg命令

