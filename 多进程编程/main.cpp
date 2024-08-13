#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

// 定义信号处理函数
void handler0(int signo) {
    printf("收到...");
}

// 回收僵尸进程
void handler1(int signo) {
    if (signo == SIGCHLD)
    {
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
    
}

// 定义信号处理函数
void handler2(int signo) {
    if (signo == SIGUSR1)
    {
        printf("逆子, 何至于此!!!\n");
        //raise(SIGKILL);     // 向自己发送一个自杀信号 kill(getpid(),SIGKILL)
    }
    
}

// 消息类型的定义
struct msgBuf
{
    long mtype;
    char mtext[1024];
};

union semun {
    int             val;        // 设置信号量的值
    struct semid_ds *buf;       // 关于信号量集属性的操作
    unsigned short  *array;     // 对于信号量集中所有信号量的操作
    struct seminfo  *_buf;      // Buffer for IPC_INFO(Linux-specific)
};

//定义一个关于对信号量初始化函数
int init_sem(int semid, int semno) {
    int val = -1;
    //printf("请输入第%d个信号量的初始值:", semno+1);
    //scanf("%d", &val);
    union semun us;
    //us.val = val;
    us.val = semno;
    if (semctl(semid, semno, SETVAL, us) == -1)
    {
        perror("semctl error");
        return -1;
    }
    return 0;
}

//创建信号量集并初始化:semcount表示本次创建的信号量集中信号灯的个数
int create_sem(int semcount) {
    // 1.创建key值
    key_t key = ftok("/", 'k');
    if (key == -1)
    {
        perror("ftok error");
        return -1;
    }
    // 2.通过key值创建信号量集
    int semid = -1;
    if ((semid = semget(key, semcount, IPC_CREAT|IPC_EXCL|06664)) == -1)
    {
        if (errno == EEXIST)    // 表示信号量集已经存在，直接打开即可
        {
            semid = semget(key, semcount, IPC_CREAT|0664);  // 将信号量集直接打开
            return semid;
        }
        perror("semget error");
        return -1;
    }
    // 3.循环将信号量集中的所有信号量进行初始化
    // 读操作，只有在第一次创建信号量集时进行操作，后面再打开信号量集时
    // 就无需进行初始化操作了
    for (int i = 0; i < semcount; i++)
    {
        init_sem(semid, i);
    }
    // 将信号量集的id返回
    return semid;
}

//申请资源操作，semno表示要被申请资源的信号量编号
int P(int semid, int semno) {
    // 定义一个结构体变量
    struct sembuf buf;
    buf.sem_num = semno;    // 要操作的信号编号
    buf.sem_op  = -1;       // -1表示要申请该信号量的资源
    buf.sem_flg = 0;        // 表示阻塞进行申请
    // 调用semop函数完成资源的申请
    if (semop(semid, &buf, 1) == -1)
    {
        perror("P error");
        return -1;
    }
    return 0;
}

//释放资源操作，semno表示要被释放资源的信号量编号
int V(int semid, int semno) {
    // 定义一个结构体变量
    struct sembuf buf;
    buf.sem_num = semno;    // 要操作的信号编号
    buf.sem_op  = 1;       // 1表示要释放该信号量的资源
    buf.sem_flg = 0;        // 表示阻塞进行释放
    // /调用semop函数完成资源的释放
    if (semop(semid, &buf, 1) == -1)
    {
        perror("V error");
        return -1;
    }
    return 0;
}

//删除信号量集
int delete_sem(int semid) {
    //调用semctl函数完成对该信号量集的删除
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("delete error");
        return -1;
    }
    return 0;
}

#define MSGSZ (sizeof(struct msgBuf)-sizeof(long)) // 正文的大小
#define PAGE_SIZE 4096  // 一页的大小


    /* 进程的种类
    1. 交互进程：它是由shell控制，可以直接和用户进行交互的，
    例如文本编辑器
    2. 批处理进程：内部维护了一个队列，被放入该队列中的进程，会被统一处理。
    例如 g++编译器的一步到位的编译
    3. 守护进程：脱离了终端而存在，随着系统的启动而运行，随着系统的退出而停止。
    例如：操作系统的服务进程*/

    /* 进程pid
    1. PID（Process ID）:进程号，进程号是一个大于等于0的整数值，是进程的唯一标识，不可能重复
    2. PPID(Parent Process ID):父进程号，系统中允许的每个进程，都是拷贝父进程资源得到的
    3. 在linux系统中的 /proc目录下的数字命名的目录其实都是一个进程*/

    /* 特殊的进程
    1. 0号进程（idel）：他是由linux操作系统启动后运行的第一个进程，也叫空闲进程，当没有其他进
    程运行时，会运行该进程。他也是1号进程和2号进程的父进程
    2. 1号进程（init）：他是由0号进程创建出来的，这个进程会完成一些硬件的必要初始化工作，除此
    之外，还会收养孤儿进程
    3. 2号进程（kthreadd）：也称调度进程，这个进程也是由0号进程创建出来的，主要完成任务调度问题
    4. 孤儿进程：当前进程还正在运行，其父进程已经退出了
    说明：每个进程退出后，其分配的系统资源应该由其父进程进行回收，否则会造成资源的浪费
    5. 僵尸进程：当前进程已经退出了，但是其父进程没有为其回收资源*/
    
int main(int argc, const char *argv[]) {

    /* 进程的创建：fork
    #include <unistd.h>
    
    pid_t fork(void);
    返回值：成功在父进程中得到子进程的pid，在子进程中的到0，
    失败返回-1并置位错误码*/
    
    pid_t pid = -1;
    pid = fork();

    if (pid > 0)
    {
        printf("我是父进程...\n");
        wait(NULL);
    } 
    else if (pid == 0)
    {
        printf("我是子进程...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("fork error");
        return -1;
    }
    
    /* 父子进程号的获取：getpid, getppid
    #include <sys/types.h>
    #include <unistd.h>
    
    pid_t getpid(void);
    pid_t getppid(void);*/

    /* 进程退出：exit, _exit
    上述两个函数都可以完成进程的退出，区别是在退出进程时，是否刷新标准IO的缓冲
    exit属于库函数，使用该函数退出进程时，会刷新标准IO的缓冲区后退出
    exit属于系统调用（内核提供的函数），使用该函数退出进程时，不会刷新标准IO的缓冲区
    
    #include <unilib.h>
    
    void exit(int status);
    参数：进程退出时的状态，会将status与 0377进行位与运算后，返回给回收资源的进程
    
    #include <unistd.h>

    void _exit(int status);
    参数：进程退出时的状态，会将status与 0377进行位与运算后，返回给回收资源的进程*/

    /* 进程资源回收：wait, waitpid
    wait是阻塞回收任意一个子进程的资源函数
    waitpid可以阻塞，也可以非阻塞完成对指定的进程号进程资源回收
    
    #include <sys/types.h>
    #include <sys/wait.h>
    
    pid_t wait(int *status);
    功能：阻塞回收子进程的资源
    参数：接收子进程退出时的状态，获取子进程退出时的状态与0377进行位与后的结果，如果不愿意
    接收，可以填NULL
    返回值：成功返回回收资源的那个进程的pid号，失败返回-1并置位错误码
    
    pid_t waitpid(pid_t, pid, int *status, int options);
    功能：可以阻塞也可以非阻塞回收指定进程的资源
    参数1：进程号
        >0:表示回收指定的进程，进程号位pid（常用）
        =0：表示回收当前进程所在进程组中的任意一个子进程
        =-1：表示回收任意一个子进程（常用）
        <-1:表示回收指定进程组中的任意一个子进程，进程组id为给定的pid的绝对值
    参数2：接收子进程退出时的状态，获取子进程退出时的状态与0377进行位与后的结果，如果不愿
    意接收，可以填NULL
    参数3：是否阻塞
        0：表示阻塞等待 ：waitpid(-1, &status, 0);等价于 wait(&status)
        WNOHANG：表示非阻塞
    返回值：
        >0: 返回的是成功回收的子进程pid号
        =0:表示本次没有回收到子进程
        =-1：出错并置位错误码*/
    
    pid = -1;
    pid = fork();

    if (pid > 0)
    {
        // 父进程程序代码
        // 输出当前进程号，子进程号，父进程号
        printf("我是父进程,self pid = %d, child pid = %d, parent pid = %d\n",getpid(),pid,getppid());
        sleep(8);
        //wait(NULL);
        waitpid(-1, NULL, WNOHANG);
        printf("子进程资源已经回收\n");
    }
    else if (pid == 0)
    {
        // 子进程程序代码
        // 输出当前进程的进程号，父进程进程号
        printf("我是子进程,self pid = %d, parent pid = %d\n",getpid(), getppid());

        printf("==========");  // 没有加换行，不会自动刷新
        sleep(3);
        exit(EXIT_SUCCESS);    // 刷新缓冲区并退出进程
        //_exit(EXIT_SUCCESS); // 不刷新缓冲区退出进程
    }
    else
    {
        perror("fork error");
        return -1;
    }
    
    /* 进程间通信
    1. IPC :interprocess communication 进程间通信
    2. 使用内核空间来完成多个进程间相互通信，根据使用的容器或方式不同，分为三类通信机制
    3. 进程间通信方式分类
        1.内核提供的通信方式(传统的通信方式)
            无名管道
            有名管道
            信号
        2.system V提供的通信方式
            消息队列
            共享内存
            信号量(信号灯集)
        3.套接字通信：socket 网络通信(跨主机通信)*/

    /* 无名管道
    管道的原理：管道是一种特殊的文件，该文件不用于存储数据，只用于进程间通信。管道分为有名
    管道和无名管道
        文件类型：bcd-lsp
            b:块设备文件
            c：字符设备文件
            d：目录文件，文件夹
            -：普通文件
            l：链接文件
            s：套接字文件（网络编程）
            p：管道文件  

    会在内存中创建出该管道，不存在于文件系统，随着进程结束而消失
    无名管道仅适用于亲缘进程间通信，不适用于非亲缘进程间通信*/

    /*无名管道的API
    #include <unistd.h>
    
    int pipe(int fildes[2]);
    功能：创建一个无名管道，并返回该管道的两个文件描述符
    参数：是一个整型数组，用于返回打开的管道的两端的文件描述符， fildes[0]表示读端
    fildes[1]表示写端
    返回值：成功返回0，失败返回-1并置位错误码*/

    int fildes[2];

    if (pipe(fildes) == -1)
    {
        perror("pipe error");
        return -1;
    }

    printf("fildes[0] = %d, fildes[1] = %d\n", fildes[0],fildes[1]);  // 3,4

    pid = -1;
    pid = fork();

    if (pid > 0)
    {
        // 父进程
        // 不用读端，就关闭
        close(fildes[0]);
        char wbuf[128] = "hello world";
        write(fildes[1], wbuf, strlen(wbuf));
        // 关闭写端
        close(fildes[1]);
        wait(NULL);
    }
    else if (pid == 0)
    {
        // 子进程
        // 不用写端，就关闭
        close(fildes[1]);
        char rbuf[128] = "";
        read(fildes[0], rbuf, sizeof(rbuf));
        printf("接收父进程的数据为:%s\n", rbuf);
        // 关闭读端
        close(fildes[0]);
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("fork error");
        return -1;
    }
    
    /* 管道通信的特点
    1.管道可以实现自己给自己发消息
    2.对管道中数据的操作是一次性的，当管道中的数据被读取后，
    就从管道中消失了，再读取时会被阻塞
    3.管道文件的大小：64K
    4.由于返回的是管道文件的文件描述符，所以对管道的操作只能是文件IO相关函数，
    但是，不可以使用lseek对光标进行偏移，必须做到先进先出
    5.管道的读写特点：
        当读端存在时：写端有多少写多少，直到写满64k后，在write处阻塞
        当读端不存在时：写端再向管道中写入数据时，会发生管道破裂，内核空间会向用户空间发射一个
        SIGPIPE信号，进程收到该信号后，退出
        当写端存在时：读端有多少读多少，没有数据，会在read出阻塞
        当写端不存在时：读端有多少读多少，没有数据，不会在read处阻塞了
    6.管道通信是半双工通信方式
        单工：只能进程A向B发送消息
        半双工：同一时刻只能A向B发消息
        全双工：任意时刻，AB可以互相通信*/
    
    /* 有名管道
    1.顾名思义就是有名字的管道文件，会在文件系统中创建一个真实存在的管道文件
    2.既可以完成亲缘进程间通信，也可以完成非亲缘进程间通信
    3.有名管道API
        #include <sys/types.h>
        #include <sys/stat.h>
        
        int mkfifo(const char *pathname, mode_t mode);
        功能：创建一个管道文件，并存在与文件系统中
        参数1：管道文件的名称
        参数2：管道文件的权限，内容详见open函数的mode参数
        返回值：成功返回0，失败返回-1并置位错误码
        注意：管道文件被创建后，其他进程就可以进行打开读写操作了，
        但是，必须要保证当前管道文件的
        两端都打开后，才能进行读写操作，否则函数会在open处阻塞*/

    if (mkfifo("./fifo", 0664) == -1)
    {
        perror("mkfifo error");
        return -1;
    }
    printf("管道创建成功\n");

    pid = -1;
    pid = fork();

    if (pid > 0)
    {
        int fd = -1;
        if ((fd = open("./fifo",O_WRONLY)) == -1)
        {
            perror("open error");
            return -1;
        }

        char wbuf[128] = "hello world";
        write(fd, wbuf, strlen(wbuf));
        sleep(1);
        close(fd);
        wait(NULL);
    }
    else if (pid == 0)
    {
        int fd = -1;
        if ((fd = open("./fifo", O_RDONLY)) == -1)
        {
            perror("open error");
            return -1;
        }
        
        char rbuf[128] = "";
        bzero(rbuf, sizeof(rbuf));
        read(fd, rbuf, sizeof(rbuf));
        printf("接受的数据为:%s\n", rbuf);
        close(fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("fork error");
        return -1;
    }
    
    /* 信号
    1.相关概念    
        1.信号是软件模拟硬件的中断功能，信号是软件实现的，中断是硬件实现的
        中断：停止当前正在执行的事情，去做另一件事情
        2.信号是linux内核实现的，没有内核就没有信号的概念
        3.用户可以给进程发信号：例如键入ctrl+c
        内核可以向进程发送信号：例如SIGPIPE
        一个进程可以给另一个进程发送信号，需要通过相关函数来完成
        4.信号通信是属于异步通信工作
            同步：表示多个任务有先后顺序的执行，例如去银行办理业务
            异步：表示多个任务没有先后顺序执行，例如你在敲代码，你妈妈在做饭
    2.对应信号的处理方式有三种：捕获、忽略、默认
    3.对信号的处理函数：signal
        #include <signal.h>
        typedef void (*sighandler_t)(int);
        
        sighandler_t signal(int signum, sighandler_t handler);
        功能：将信号与信号处理方式绑定到一起
        参数1：要处理的信号
        参数2：处理方式
            SIG_IGN：忽略
            SIG_DFL：默认，一般信号的默认操作都是杀死进程
            typedef void (*sighandler_t)(int)：用户自定义的函数
        返回值：成功返回处理方式的起始地址，失败返回SIG_ERR并置位错误码
        注意：只要程序与信号绑定一次，后续但凡程序收到该信号，对应的处理方式就会立即响应*/

    /* 1.尝试忽略SIGKILL信号,函数报错，错误原因参数不合法
    if(signal(SIGKILL, SIG_IGN) == SIG_ERR)
    {
        perror("signal error");
        return -1;
    }*/

   /* 2.尝试捕获SIGKILL信号,执行报错，错误原因参数不合法
    if(signal(SIGKILL, handler) == SIG_ERR)
    {
        perror("signal error");
        return -1;
    }*/

    /* 3.尝试默认相关信号的操作,执行报错，错误原因参数不合法
    if(signal(SIGKILL, SIG_DFL) == SIG_ERR)
    {
        perror("signal error");
        return -1;
    }*/    

    /*当子进程退出后，会向父进程发送一个SIGCHLD的信号，我们可以将其捕获，
    在信号处理函数中将子进程资源回收*/
    if (signal(SIGCHLD, handler1) == SIG_ERR)
    {
        perror("signal error");
        return -1;
    }
    
    // 创建10个僵尸进程
    for (int i = 0; i < 10; i++)
    {
        if (fork() == 0)
        {
            exit(EXIT_SUCCESS);
        }
        
    }

    /*信号发送函数: kill, raise
    #include <signal.h>
    
    int kill(pid_t pid, int sig);
    功能：向指定进程或进程组发送信号
    参数1：进程号或进程组号
        >0:表示向执行进程发送信号
        =0：向当前进程所在的进程组中的所有进程发送信号
        =-1：向所有进程发送信号
        <-1:向指定进程组发送信号，进程组的ID号为给定pid的绝对值
    参数2：要发送的信号
    返回值：成功返回0，失败返回-1并置位错误码
    
    #include <signal.h>
    
    int raise(int sig);
    功能：向自己发送信号 等价于：kill(getpid(), sig);
    参数：要发送的信号
    返回值：成功返回0，失败返回非0数*/

    if (signal(SIGUSR1, handler2) == SIG_ERR)
    {
        perror("signal error");
        return -1;
    }
    
    pid = -1;
    pid = fork();
    if (pid > 0)
    {
        // 父进程
        for (int i = 0; i < 3; i++)
        {
            printf("我真的还想再活五百年\n");
            sleep(1);
        }
        wait(NULL);
    }
    else if (pid == 0)
    {
        // 子进程
        sleep(2);
        printf("红尘已经看破,叫上父亲一起死吧\n");
        kill(getppid(), SIGUSR1);
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("fork error");
        return -1;
    }
    
    // 总结：信号可以完成多个进程间通知作用，但是，不能进行数据传输功能
    
    // system V 提供的进程间通信概述
    /* 1.对于内核提供的三种通信方式，对于管道而言，只能实现单向的数据通信，对于信号通信而言，只
    能完成多进程之间消息的通知，不能起到数据传输的效果。为了解决上述问题，引入的系统 V进程间通信
    2.system V提供的进程间通信方式分别是：消息队列、共享内存、信号量（信号灯集）
    3.有关system V进程间通信对象相关的指令
        ipcs            可以查看所有信息（消息队列，共享内存，信号量）
        ipcs -q         可以查看消息队列的信息
        ipcs -m         可以查看共享内存的信息
        ipcs -s         可以查看信号量的信息
        ipcrm -q/m/s ID 可以删除指定ID的IPC对象
    4.上述的三种通信方式，也是借助内核空间完成的相关通信，原理是在内核空间创建出相关的对象容
    器，在进行进程间通信时，可以将信息放入对象中，另一个进程就可以从该容器中取数据了。
    5.与内核提供的管道、信号通信不同：system V的ipc对象实现了数据传递的容器与程序相分离，也
    就是说，即使程序以己经结束，但是放入到容器中的数据依然存在，除非将容器手动删除*/

    // 消息队列实现的API
    /* 1.创建key值
    #include <sys/types.h>
    #include <sys/ipc.h>
    
    key_t ftok(const char *pathname, int proj_id);  //ftok("/", 'k);
    功能：通过给定的文件以及给定的一个随机值，创建出一个4字节整数的key值，用于system V
    IPC对象的创建
    参数1：一个文件路径，要求是已经存在的文件路径，提供了key值3字节的内容，其中，文件的设备
    号占1字节，文件的inode号占2字节
    参数2：一个随机整数，取后8位（1字节）跟前面的文件共同组成key值，必须是非0的数字
    返回值：成功返回key值，失败返回-1并置位错误码
    
    2.通过key值，创建消息队列
    #include <sys/types.h>
    #include <sys/ipcs.h>
    #include <sys/msg.h>
    
    int msgget(key_t key, int msgflg);
    功能：通过给定的key值，创建出一个消息队列的对象，并返回消息队列的句柄ID，后期可以通过该
    ID操作整个消息队列
    参数1：key值，该值可以是IPC_PRIVATE,也可以是ftok创建出来的，前者只用于亲缘进程间的通信
    参数2：创建标识
    IPC_CREAT:创建并打开一个消息队列，如果消息队列已经存在，则直接打开
    IPC_EXCL:确保本次创建处理的是一个新的消息队列，如果消息队列已经存在，则报错，错误码位EEXIST
    0664：该消息队列的操作权限
        eg: IPC_CREAT|0664 或者 IPC_CREAT|IPC_EXCL|0664
    返回值：成功返回消息队列的ID号，失败返回-1并置位错误码
    
    3.向消息队列中存放数据
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/msg.h>
    
    int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
    功能：向消息队列中存放一个指定格式的消息
    参数1：打开的消息队列的id号
    参数2：要发送的消息的起始地址，消息一般定义为一个结构体类型，由用户手动定义
    struct msgbuf {
        long mtype;         //  message type, must be > 0 
        char mtext[1];      //  messaage data
        ...
    }
    参数3：消息正文的大小
    参数4：是否阻塞的标识
        0：标识阻塞形式向消息队列中存放消息，如果消息队列满了，就在该函数处阻塞
        IPC_NOWAIT:标识非阻塞的形式向消息队列中存放消息，如果消息队列满了，直接返回
    返回值：成功返回0，失败返回-1并置位错误码
    
    4.从消息队列中读取消息
    ssize_t msgrcv(int msgid, void* msgp, size_t msgsz, long msgtyp, int msgflg);
    功能：从消息队列中取数指定类型的消息放入给定的容器中
    参数1：打开的消息队列的id号
    参数2：要接收的消息的起始地址，消息一般定义为一个结构体类型，由用户手动定义
    struct msgbuf {
        long mtype;         //  message type, must be > 0 
        char mtext[1];      //  messaage data
        ...
    }
    参数3：消息正文的大小
    参数4：要接收的消息类型
        0:表示每次都取消息队列中的第一个消息，无论类型
        >0:读取队列中第一个类型为msgtyp的消息
        <0:读取队列中的一个消息，消息为绝对值小于msgtyp的第一个消息
            eg： 10-->8-->3-->6-->5-->20-->2
            -5: 会从队列中绝对值小于5的类型的消息中选取第一个消息，就是3
    参数5：是否阻塞的标识
        0：标识阻塞形式向消息队列中读取消息，如果消息队列空了，就在该函数处阻塞
        IPC_NOWAIT:标识非阻塞的形式向消息队列中读取消息，如果消息队列空了，直接返回
    返回值：成功返回实际读取的正文大小，失败返回-1并置位错误码
    
    5.销毁消息队列
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/msg.h>
    
    int msgctl(int msgid, int cmd, struct msgid_ds *buf);
    功能：对给定的消息队列执行相关的操作，该操作由cmd参数而定
    参数1：消息队列的ID号
    参数2：要执行的操作
        IPC_RMID：删除一个消息队列，当cmd为该值时，第三个参数可以省略填NULL即可
        IPC_STAT：表示获取当前消息队列的属性，此时第三个参数就是存放获取的消息队列属性
        的容器起始地址
        IPC_SET：设置当前消息队列的属性，此时第三个参数就是要设置消息队列的属性数据的起
        始地址
    参数3：消息队列数据容器结构体，如果第二个参数为IPC_RMID,则该参数忽略填NULL即可，如果
    是 IPC_STAT、 IPC_SET填如下结构体：
        struct msqid_ds {
            struct ipc_perm msg_perm;   // Ownership and permissions 消息队列的拥有者和权限
            time_t msg_stime;           // Time of last msgsnd(2)  最后一次发送消息的时间
            time_t msg_rtime;           // Time of last msgrcv(2)  最后一次接收消息的时间
            time_t msg_ctime;           // Time of last change  最后一次状态改变的时间
            unsigned long __msg_cbytes; // Current number of bytes in queue(nonstandard)  已用字节数
            msgqnum_t msg_qnum;         // Current number of messages inqueue  消息队列中消息个数
            msglen_t msg_qbytes;        // Maximum number of bytes allowedin queue 最大消息个数
            pid_t msg_lspid;            // PID of last msgsnd(2)  最后一次发送消息的进程pid
            pid_t msg_lrpid;            // PID of last msgrcv(2)  最后一次读取消息的进程pid
        };
        该结构体的第一个成员类型如下：
        struct ipc_perm {
            key_t __key;            // Key supplied to msgget(2)  key值
            uid_t uid;              // Effective UID of owner  当前进程的uid
            gid_t gid;              // Effective GID of owner  当前进程的组ID
            uid_t cuid;             // Effective UID of creator  消息队列创建者的用户id
            gid_t cgid;             // Effective GID of creator  消息队列创建者的组id
            unsigned short mode;    // Permissions  消息队列的权限
            unsigned short __seq;   // Sequence number  队列号
        };
    返回值：成功返回0，失败返回-1并置位错误码*/

    // 1.创建key值，用于创建出一个消息队列
    key_t key = ftok("/", 'k');
    if (key == -1)
    {
        perror("ftok error");
        return -1;
    }
    printf("key = %#x\n", key);

    // 2.通过key值创建出一个消息队列,并返回该消息队列的id
    int msqid = -1;
    if ((msqid = msgget(key, IPC_CREAT|0664)) == -1)
    {
        perror("msgget error");
        return -1;
    }
    printf("msgqid = %d\n", msqid);

    pid = -1;
    pid = fork();
    if (pid > 0) {
        // 3.向消息队列中存放消息
        struct msgBuf wbuf;
        wbuf.mtype = 1;
        sprintf(wbuf.mtext, "%s", "Hello world(msq)!\0");
        msgsnd(msqid, &wbuf, MSGSZ, 0);
        printf("消息存入成功!\n");
        wait(NULL);
    } else if (pid == 0)
    {
        // 4.接收消息
        struct msgBuf rbuf;
        // 清空容器
        bzero(&rbuf, sizeof(rbuf));
        // 读取消息
        // 参数4：消息类型
        // 参数5：是否阻塞读取
        msgrcv(msqid, &rbuf, MSGSZ, 1, 0);
        printf("接收的消息为:%s\n", rbuf.mtext);
        exit(EXIT_SUCCESS);
    } else {
        perror("fork error");
        return -1;
    }

    // 删除消息队列
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl error");
        return -1;
    }
    
    // 共享内存
    /* 共享内存的API
    1.创建key值
    #include <sys/types.h>
    #include <sys/ipc.h>
    
    key_t ftok(const char *pathname, int proj_id);
    功能：通过给定的文件以及给定的一个随机值，创建出一个4字节整数的key值，用于system V
    IPC对象的创建
    参数1：一个文件路径，要求是已经存在的文件路径，提供了key值3字节的内容，其中，文件的设备
    号占1字节，文件的inode号占2字节
    参数2：一个随机整数，取后8位（1字节）跟前面的文件共同组成key值，必须是非0的数字
    返回值：成功返回key值，失败返回-1并置位错误码
    
    2.通过key值创建共享内存段
    #include <sys/ipc.h>
    #include <sys/shm.h>
    
    int shmget(key_t key, size_t size, int shmflg);
    功能：申请指定大小的物理内存，映射到内核空间，创建出共享内存段
    参数1：key值，可以是IPC_PRIVATE,也可以是ftok创建出来的key值
    参数2：申请的大小，是一页（4096字节）的整数倍，并且向上取整
    参数3：创建标识
        IPC_CREAT:创建并打开一个共享内存，如果共享内存已经存在，则直接打开
        IPC_EXCL:确保本次创建处理的是一个新的共享内存，如果共享内存已经存在，则报错，错
        误码位EEXIST
        0664：该共享内存的操作权限
        eg: IPC_CREAT|0664 或者 IPC_CREAT|IPC_EXCL|0664
    返回值：成功返回共享内存段的id，失败返回-1并置位错误码
    
    3.将共享内存段的地址映射到用户空间
    #include <sys/tyeps>
    #include <sys/shm.h>
    
    void *shmat(int shmid, const void *shmaddr, int shmflg);
    功能：将共享内存段映射到用户空间
    参数1：共享内存的id号
    参数2：物理内存的起始地址，一般填NULL，由系统自动选择一个合适的对齐页
    参数3：对共享内存段的操作
        0：表示读写操作
        SHM_RDONLY:只读
    返回值：成功返回用于操作共享内存的指针，失败返回(void*)-1并置位错误码
    
    4.释放共享内存的映射关系
    int shmdt(const void *shmaddr);
    功能：将进程与共享内存的映射取消
    参数：共享内存的指针
    返回值：成功返回0，失败返回-1并置位错误码
    
    5.共享内存的控制函数
    #include <sys/ipc.h>
    #include <sys/shm.h>
    
    int shmctl(int shmid, int cmd, struct shmid_ds *buf);
    功能：根据给定的不同的cmd执行不同的操作
    参数1：共享内存的ID
    参数2：要操作的指令
        IPC_RMID:删除共享内存段，第三个参数可以省略
        IPC_STAT:获取当前共享内存的属性
        IPC_SET:设置当前共享内存的属性
    参数3：如果参数2为IPC_RMID，则参数3可以省略填NULL，如果参数2为另外两个，参数3填如下
    结构体变量
    
    struct shmid_ds {
        struct ipc_perm shm_perm; // Ownership and permissions 
        size_t shm_segsz;         // Size of segment (bytes) 
        time_t shm_atime;         // Last attach time 
        time_t shm_dtime;         // Last detach time 
        time_t shm_ctime;         // Last change time 
        pid_t shm_cpid;           // PID of creator 
        pid_t shm_lpid;           // PID of last shmat(2)/shmdt(2) 
        shmatt_t shm_nattch;      // No. of current attaches 
        ...
    };
    该结构体的第一个成员结构体：
    struct ipc_perm {
        key_t __key; // Key supplied to shmget(2) 
        uid_t uid; // Effective UID of owner 
        gid_t gid; // Effective GID of owner 
        uid_t cuid; // Effective UID of creator 
        gid_t cgid; // Effective GID of creator 
        unsigned short mode; // Permissions + SHM_DEST and SHM_LOCKED flags 
        unsigned short __seq; // Sequence number 
    };
    返回值：成功返回0，失败饭hi-1并置位错误码*/

    // 通过key值创建共享内存段
    int shmid = -1;
    if ((shmid = shmget(key, PAGE_SIZE, IPC_CREAT|0664)) == -1)
    {
        perror("shmget error");
        return -1;
    }
    printf("shmid = %d\n", shmid);

    // 将共享内存映射到用户空间
    // NULL表示让系统自动寻找对齐页
    // 0表示对该共享内存段的操作是读写操作打开
    char *addr = (char *)shmat(shmid, NULL, 0);
    if (addr == (void*)-1)
    {
        perror("shmat error");
        return -1;
    }

    // 读写操作
    pid = -1;
    pid = fork();
    if (pid > 0) {
        printf("addr = %p\n", addr);
        sprintf(addr, "%s", "Hello world(shm)!\0");
        printf("写入结束\n");
        // 取消映射
        if (shmdt(addr) == -1)
        {
            perror("shmdt error");
            return -1;
        }
        printf("父进程取消映射\n");
        wait(NULL);
    }
    else if (pid == 0)
    {
        sleep(2);
        printf("读取的数据为:%s\n", addr);
        // 取消映射
        if (shmdt(addr) == -1)
        {
            perror("shmdt error");
            return -1;
        }
        printf("子进程取消映射\n");
        exit(EXIT_SUCCESS);
    }

    // 删除共享内存段
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl error");
        return -1;
    }

    // 注意
    /* 1.共享内存是多个进程共享同一个内存空间，使用时可能会产生竞态，为了解决这个问题，共享
    内存一般会跟信号量一起使用，完成进程的同步功能
       2.共享内存VS消息队列：消息队列能够保证数据的不丢失性，而共享内存能够保证数据的时效性
       3.对共享内存的读取操作不是一次性的，当读取后，数据依然存放在共享内存中
       4.使用共享内存，跟正常使用指针是一样的，使用时，无需再进行用户空间与内核空间的切换
    了，所以说，共享内存是所有进程间通信方式中效率最高的一种通信方式.*/ 

    // 信号量(信号灯集)
    /* 信号量:本质上维护一个value值，是一个整数，有进程申请信号量资源时，
    如果该信号量资源大于0，则进行递减操作，如果当前信号量的值为0，则申请
    资源的进程处于阻塞状态，直到另一个进程将该value值增加到大于0，当有进程
    对该信号量进行释放操作时，这个value值会递增*/

    /* 信号量相关的API
    1.创建key值
    #include <sys/types.h>
    #include <sys/ipc.h>
    
    key_t ftok(const char *pathname, int proj_id);
    功能：通过给定的文件以及给定的一个随机值，创建出一个4字节整数的key值，用于system V
    IPC对象的创建
    参数1：一个文件路径，要求是已经存在的文件路径，提供了key值3字节的内容，其中，文件的设备
    号占1字节，文件的inode号占2字节
    参数2：一个随机整数，取后8位（1字节）跟前面的文件共同组成key值，必须是非0的数字
    返回值：成功返回key值，失败返回-1并置位错误码
    
    2.通过key值创建信号量级
    #include <sys/types>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    
    int semget(key_t key, int nsems, int semflg);
    功能：通过给定的key值创建一个信号量集
    参数1：key值，该值可以是IPC_PRIVATE,也可以是ftok创建出来的，
    前者只用于亲缘进程间的通信
    参数2：信号量数组中信号量的个数
    参数3：创建标识
        IPC_CREAT:创建并打开一个信号量集，如果信号量集已经存在，则直接打开
        IPC_EXCL:确保本次创建处理的是一个新的信号量集，如果信号量集已经存在，则报错，错
        误码位EEXIST
        0664：该信号量集的操作权限
        eg: IPC_CREAT|0664 或者 IPC_CREAT|IPC_EXCL|0664
    返回值：成功返回信号量集的id，失败返回-1并置位错误码
    
    关于信号量集的操作：P(申请资源),V(释放资源)
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    
    int semop(int semid, struct sembuf *sops, size_t nsops);
    功能：完成对信号量数组的操作
    参数1：信号量数据ID号
    参数2：有关信号量操作的结构体变量起始地址，该结构体中包含了操作的信号量编号和申请还是释
    放的操作
    struct sembuf
    {
        unsigned short sem_num; // semaphore number  要操作的信号量的编号
        short sem_op; // semaphore operation  要进行的操作，大于0表示释放资源，小于0表示申请资源
        short sem_flg; // operation flags  操作标识位，0标识阻塞方式，IPC_NOWAIT表示非阻塞
    }
    参数3：本次操作的信号量的个数
    返回值：成功返回0，失败返回-1并置位错误码
    
    关于信号量集的控制函数
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    
    int semctl(int semid, int semnum, int cmd, ...);
    功能：执行有关信号量集的控制函数，具体控制内容取决于cmd
    参数1：信号量集的ID
    参数2：要操作的信号量的编号，编号是从0开始
    参数3：要执行的操作
        IPC_RMID:表示删除信号量集，cmd为该值时，参数2可以忽略，参数4可以不填
        SETVAL：表示对参数2对应的信号量进行设置操作（初始值）
        GETVAL：表示对参数2对应的信号量进行获取值操作
        SETALL：设置信号量集中所有信号量的值
        GETALL:获取信号量集中的所有信号量的值
        IPC_STAT:表示获取当前信号量集的属性
        IPC_SET:表示设置当前信号量集的属性
    参数4：根据不同的cmd值，填写不同的参数值，所以该处是一个共用体变量
    union semun {
        int val; // Value for SETVAL  设置信号量的值
        struct semid_ds *buf; // Buffer for IPC_STAT, IPC_SET  关于信号量集属性的操作
        unsigned short *array; // Array for GETALL, SETALL  对于信号量集中所有信号量的操作
        struct seminfo *__buf; // Buffer for IPC_INFO(Linux-specific) 
    };
    返回值：成功时：SETVAL、IPC_RMID返回0，GETVAL返回当前信号量的值，失败返回-1并置位
    错误码
    例如:
        1.给0号信号量设置初始值为1
        union semun us;         // 定义一个共用体变量
        us.val = 1;             // 对该共用体变量赋值
        semctl(semid, 0, SETVAL, us);   // 该函数就完成了对0号信号量设置初始值为1的操作
        
        2.删除信号量集
        semctl(semid, 0, IPC_RMID);
        
        3.将上述函数进行二次封装，封装成为只有信号量集的创建，申请资源，释放资源，销毁信号量集
        //创建信号量集并初始化:semcount表示本次创建的信号量集中信号灯的个数
        int create_sem(int semcount);
        //申请资源操作，semno表示要被申请资源的信号量编号
        int P(int semid, int semno);
        //释放资源操作，semno表示要被释放资源的信号量编号
        int V(int semid, int semno);
        //删除信号量集
        int delete_sem(int semid);*/

    //  使用信号量集完成共享内存中两个进程对共享内存使用的同步问题

    int semid = create_sem(2);
    shmid = -1;
    if ((shmid = shmget(key, PAGE_SIZE, IPC_CREAT|0664)) == -1)
    {
        perror("shmget error");
            return -1;
    }
    addr = (char *)shmat(shmid, NULL, 0);
    if (addr == (void *)-1)
    {
        perror("shmat error");
        return -1;
    }

    pid = -1;
    pid = fork();
    if (pid > 0)
    {
        // 调用自定义函数，申请0号信号量的资源
        for (int i = 0; i < 3; i++)
        {
            P(semid, 1);
            sprintf(addr, "%s", "Hello world(sem)!\0");
            printf("父进程写入消息为:%s\n", addr);
            // 调用自定义函数，释放1号信号量的资源 
            V(semid, 0);
        }
        wait(NULL);
    }
    else if (pid == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            // 调用自定义函数完成对1号信号量资源的申请
            P(semid, 0);
            printf("子进程读取消息为：%s\n", addr);
            // 调用自定义函数，释放0号信号量的资源 
            V(semid, 1);
        }
        // 调用自定义函数，删除信号量集
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("fork error");
        return -1;
    }
    // 调用自定义函数，删除信号量集
    delete_sem(semid);
    // 取消映射
    if (shmdt(addr) == -1)
    {
        perror("取消映射\n");
        return -1;
    }
    // 删除共享内存段
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl error");
        return -1;
    }
    

    return 0;
}