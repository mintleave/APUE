#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// 定义互斥锁
pthread_mutex_t mutex;
// 定义无名信号量
sem_t sem;
// 定义条件变量
pthread_cond_t cond;

// 定义一个全局变量
int num = 520;

// 定义信息结构体，用于向线程体传递数据
struct Info
{
    int    num;
    char   name[20];
    double score;
};


// 定义线程体函数
void *task0(void *arg) {
    printf("我是分支线程!");
    sleep(1);
}

void *task1(void *arg) {
    printf("我是分支线程: num = %d\n", *(int*)arg);
    sleep(1);
}

void *task2(void *arg) {
    Info buf = *((Info*)arg);
    printf("分支线程中:num = %d, name = %s, score = %.2lf\n",
        buf.num, buf.name, buf.score);
}

void *task3(void *arg) {
    printf("分支线程,tid = %#x\n", pthread_self());
    pthread_exit(NULL);
}

void *task4(void *arg) {
    while (num > 0)
    {
        sleep(1);
        // 获取锁资源
        pthread_mutex_lock(&mutex);
        num -= 10;
        printf("张三取了10, 剩余%d\n", num);
        // 释放锁资源
        pthread_mutex_unlock(&mutex);
    }
}

void *task5(void *arg) {
    while (num > 0)
    {
        sleep(1);
        // 获取锁资源
        pthread_mutex_lock(&mutex);
        num -= 10;
        printf("李四取了10, 剩余%d\n", num);
        // 释放锁资源
        pthread_mutex_unlock(&mutex);
    }
}

// 创建生产者线程
void *task6(void *arg) {
    int _num = 5;
    while (_num--)
    {
        sleep(1);
        printf("我生产了一辆特斯拉\n");
        // 释放无名信号量资源
        sem_post(&sem);
    }
    // 退出线程
    pthread_exit(NULL);
}

// 创建消费者线程
void *task7(void *arg) {
    int _num = 5;
    while (_num--)
    {
        // 申请无名信号量资源
        sem_wait(&sem);
        printf("我消费了一辆特斯拉,很开心\n");
    }
    // 退出线程
    pthread_exit(NULL);
}

void *task8(void *arg) {
    sleep(3);
    printf("我生产了三辆特斯拉\n");
    // 唤醒所有消费者线程
    pthread_cond_broadcast(&cond);
    //退出线程
    pthread_exit(NULL);
}

void *task9(void *arg) {
    // 获取锁资源
    pthread_mutex_lock(&mutex);
    // 进入休眠队列，等待生产者的唤醒
    pthread_cond_wait(&cond, &mutex);
    printf("%#x:消费了一辆特斯拉\n", pthread_self());
    // 释放锁资源
    pthread_mutex_unlock(&mutex);
    // 退出线程
    pthread_exit(NULL);
}

int main(int argc, const char *argv[]) {
    /* 多线程基础
    1.进程是资源分配的基本单位，而线程是任务器进行任务调度的最小单位
    2.一个进程可以拥有多个线程，同一个进程中的多个线程共享进程的资源
    3.每个进程至少有一个线程：主线程
    4.只要有一个线程中退出了进程，那么所有的线程也就结束了
    主线程结束后，整个进程也就结束了*/

    /* 多线程编程
    由于C库没有提供有关多线程的相关操作，对于多线程编程要依赖于第三方库
    头文件：#include<pthread.h>
    编译时：需要加上 -lpthread 选项，链接上对于的线程支持库*/

    /* 创建线程：pthread_create
    #include <pthread.h>
    
    int pthread_creat(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine)(void *), void *arg);
    功能：创建一个分支线程
    参数1：线程号，通过参数返回，用法：在外部定义一个该类型的变量，将地址传递入函数，调用结
    束后，该变量中即是线程号
    参数2：线程属性，一般填NULL，让系统使用默认属性创建一个线程
    参数3：是一个回调函数，一个函数指针，需要向该参数中传递一个函数名，作为线程体执行函数
    该函数由用户自己定义，参数是void*类型，返回值也是void *类型
    参数4： 是参数3的参数，如果不想向线程体内传递数据，填NULL即可
    返回值：成功返回0，失败返回一个错误码（非linux内核的错误码，是线程支持库中定义的一个错
    误码）*/

    pthread_t tid = -1;
    if (pthread_create(&tid, NULL, task0, NULL) != 0)
    {
        printf("tid creat error\n");
        return -1;
    }
    printf("pthread_creat success\n");
    printf("我是主线程!\n");
    sleep(1);
    // 向线程体传递单个数据
    tid = -1;
    num = 520;
    if (pthread_create(&tid, NULL, task1, &num) != 0)
    {
        printf("tid creat error\n");
        return -1;
    }
    printf("pthread_creat success\n");
    printf("我是主线程!\n");
    num = 1314;
    printf("主线程中num = %d\n", num);
    sleep(1);
    // 向线程体传入多个数据
    tid = -1;
    char name[20] = "zhangsan";
    double score = 99.5;
    Info buf = {num, "zhangsan", score};
    if (pthread_create(&tid, NULL, task2, &buf) != 0)
    {
        printf("tid creat error\n");
        return -1;
    }
    sleep(1);

    /* 线程号获取: pthread_self
    #include <pthread.h>
    pthread_t pthread_self(void);
    功能：获取当前线程的线程号
    参数：无
    返回值：返回调用线程的id号，不会失败*/

    /* 线程退出函数
    #include <pthread.h>
    void pthread_exit(void *retval);
    功能：退出当前线程
    参数：表示退出时的状态，一般填NULL
    返回值：无*/

    /* 线程的资源回收: pthread_join
    #include <pthread.h>
    int pthread_join(pthread_t thread, void *retval);
    功能：阻塞回收指定线程的资源
    参数1：要回收的线程线程号
    参数2：线程退出时的状态，一般填NULL
    返回值：成功返回0，失败返回一个错误码*/

    /* 线程分离态: pthread_detach
    #include <pthread.h>
    
    int pthread_datach(pthread_t thread);
    功能：将指定线程设置成分离态，被设置成分离态的线程，退出后，资源由系统自动回收
    参数：要分离的线程号
    返回值：成功返回0，失败返回一个错误码*/

    tid = -1;
    if (pthread_create(&tid, NULL, task3, NULL) != 0)
    {
        printf("tid creat error\n");
        return -1;
    }
    
    // 阻塞回收线程资源
    pthread_join(tid, NULL);
    // 非阻塞回收
    //pthread_detach(tid);

    /* 线程的同步互斥机制
    1.由于同一个进程的多个线程会共享进程的资源，这些被共享的资源称为临界资源
    2.多个线程对公共资源的抢占问题，访问临界资源的代码段称为临界区
    3.多个线程抢占进程资源的现象称为竞态
    4.为了解决竞态，我们引入了同步互斥机制*/

    /* 互斥锁
    互斥锁的本质也是一个特殊的临界资源，当该临界资源被某个线程所拥有后，其他线程就不能拥有
    该资源，直到，拥有该资源的线程释放掉互斥锁后，其他线程才能进行抢占（同一时刻，一个互斥锁只
    能被一个线程所拥有）
    
    相关API函数
    1.创建一个互斥锁：只需定义一个pthread_mutex_t 类型的变量即创建了一个互斥锁
    pthread_mutex_t mutex;

    2.初始化互斥锁
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 静态初始化
    int pthread_mutex_init(pthread_mutex_t *resrict mutex, const
                    pthread_mutexattr_t *restrict attr);
    功能：初始化互斥锁变量
    参数1：互斥锁变量的地址，属于地址传递
    参数2：互斥锁属性，一般填NULL，让系统自动设置互斥锁属性
    返回值：成功返回0，失败返回错误码
    
    3.获取锁资源
    int pthread_mutex_lock(pthread_mutex_t *mutex);
    功能：获取锁资源,如果要获取的互斥锁已经被其他线程锁定，那么该函数会阻塞，直到能够获取锁资源
    参数：互斥锁地址，属于地址传递
    返回值：成功返回0，失败返回一个错误码
    
    4.释放锁资源
    int pthread_mutex_unlock(pthread_mutex_t *mutex);
    功能：释放对互斥锁资源的拥有权
    参数：互斥锁变量的地址
    返回值：成功返回0，失败返回一个错误码
    
    5.销毁互斥锁
    int pthread_mutex_destroy(pthread_mutex_t *mutex);
    功能：销毁互斥锁
    参数：互斥锁变量的地址
    返回值：成功返回0，失败返回一个错误码*/

    // 初始化互斥锁
    pthread_mutex_init(&mutex, NULL);

    num = 100;

    // 创建两个分支线程
    pthread_t tid1, tid2;
    if (pthread_create(&tid1, NULL, task4, NULL) != 0)
    {
        printf("tid1 create error\n");
        return -1;
    }
    if (pthread_create(&tid2, NULL, task5, NULL) != 0)
    {
        printf("tid2 create error\n");
        return -1;
    }

    printf("主线程: tid1 = %#x, tid2 = %#x\n", tid1, tid2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // 销毁互斥锁
    //pthread_mutex_destroy(&mutex);

    /*  无名信号量
    1.线程同步：就是多个线程之间有先后顺序得执行，这样在访问临界资源时，就不会产生抢占现象了
    2.同步机制常用于生产者消费者模型：消费者任务要想执行，必须先执行生产者线程，多个任务有顺
    序执行
    3.无名信号量：本质上也是一个特殊的临界资源，内部维护了一个value值，当某个进行想要执行之
    前，先申请该无名信号量的value资源，如果value值大于0，则申请资源函数解除阻塞，继续执行后续
    操作。如果value值为0，则当前申请资源函数会处于阻塞状态，直到其他线程将该value值增加到大于0*/

    /* 无名信号量的相关API函数接口
    1.创建无名信号量：只需定义一个sem_t 类型的变量即可
    sem_t sem;

    2.初始化无名信号量
    #include <semaphore.h>
    int sem_init(sem_t *sem, int pshared, unsigned int value);
    功能：初始化无名信号量，最主要是初始化value值
    参数1：无名信号量的地址
    参数2：判断进程还是线程的同步
        0：表示线程间同步
        非0：表示进程间同步，需要创建在共享内存段中
    参数3：无名信号量的初始值
    返回值：成功返回0，失败返回-1并置位错误码
    
    3.申请无名信号量的资源(P操作)
    #include <semaphore.h>
    int sem_wait(sem_t *sem);
    功能：阻塞申请无名信号量中的资源，成功申请后，会将无名信号量的value进行减1操作，如果当
    前无名信号量的value为0，则阻塞
    参数：无名信号量的地址
    返回值：成功返回0，失败返回-1并置位错误码
    
    5.销毁无名信号量
    #include <semaphore.h>
    int sem_destroy(sem_t *sem);
    功能：销毁无名信号量
    参数：无名信号量地址
    返回值：成功返回0，失败返回-1并置位错误码*/

    // 初始化无名信号量,第一个0表示用于线程间通信，第二个0表示初始值为0
    sem_init(&sem, 0, 0);
    pthread_t tid3, tid4;
    if (pthread_create(&tid3, NULL, task6, NULL) != 0)
    {
        printf("tid3 create error\n");
        return -1;
    }
    if (pthread_create(&tid4, NULL, task7, NULL) != 0)
    {
        printf("tid4 create error\n");
        return -1;
    }

    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
    
    sem_destroy(&sem);

    /* 条件变量
    1.条件变量本质上也是一个临界资源，他维护了一个队列，当消费者线程想要执行时，先进入队列中
    等待生产者的唤醒。执行完生产者，再由生产者唤醒在队列中的消费者，这样就完成了生产者和消费者
    之间的同步关系。
    2.但是，多个消费者在进入休眠队列的过程是互斥的，所以，在消费者准备进入休眠队列时，需要使
    用互斥锁来进行互斥操作
    */

    /* 条件变量的API函数接口
    1.创建一个条件变量，只需定义一个pthread_cond_t类型的全局变量即可
    pthread_cond_t cond;
    
    2.初始化条件变量
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER; //静态初始化
    int pthread_cond_init((pthread_cond_t *restrict cond,const pthread_condattr_t
                    *restrict attr);
    功能：初始化条件变量
    参数1：条件变量的起始地址
    参数2：条件变量的属性，一般填NULL
    返回值：成功返回0，失败返回一个错误码
    
    3.消费者线程进入等待队列
    int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t
                *restrict mutex);
    功能：将线程放入休眠等待队列，等待其他线程的唤醒
    参数1：条件变量的地址
    参数2：互斥锁，由于多个消费者线程进入等待队列时会产生竞态，为了解决竞态，需要使用一个互斥锁
    返回值：成功返回0，失败返回错误码
    
    4.生产者线程唤醒休眠队列中的任务
    int pthread_cond_broadcast(pthread_cond_t *cond);
    功能：唤醒条件变量维护的队列中的所有消费者线程
    参数：条件变量的地址
    返回值：成功返回0，失败返回错误码
    
    int pthread_cond_signal(pthread_cond_t *cond);
    功能：唤醒条件变量维护的队列中的第一个进入队列的消费者线程
    参数：条件变量的地址
    返回值：成功返回0，失败返回错误码
    
    5.销毁条件变量
    int pthread_cond_destroy(pthread_cond_t *cond);
    功能：销毁一个条件变量
    参数：条件变量的地址
    返回值：成功返回0，失败返回错误码*/

    // 初始化条件变量
    pthread_cond_init(&cond, NULL);
    // 初始化互斥锁
    pthread_mutex_init(&mutex, NULL);
    
    pthread_t tid5, tid6, tid7, tid8;
    if (pthread_create(&tid5, NULL, task8, NULL) != 0)
    {
        printf("tid5 create error\n");
        return -1;
    }
    if (pthread_create(&tid6, NULL, task9, NULL) != 0)
    {
        printf("tid6 create error\n");
        return -1;
    }
    if (pthread_create(&tid7, NULL, task9, NULL) != 0)
    {
        printf("tid7 create error\n");
        return -1;
    }
    if (pthread_create(&tid8, NULL, task9, NULL) != 0)
    {
        printf("tid3 create error\n");
        return -1;
    }

    pthread_join(tid5, NULL);
    pthread_join(tid6, NULL);
    pthread_join(tid7, NULL);
    pthread_join(tid8, NULL);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return 0;
}