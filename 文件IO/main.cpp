#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char const *argv[])
{
    // 文件描述符
    /*
    1.文件描述符的本质是一个大于等于0的整数
    2.在一个进程中，能够打开的文件描述符是有限的，一般是1024个，
    [0,1023]可以通过指令ulimit -a进行查看，通过ulimit -n 数字进行更改
    3.特殊的文件描述符：0,1,2 这三个文件描述符在一个进程启动时就默认被打开了，
    分别表示标准输入，标准输出，标准错误
    */

    // 分别输出标准输入，标准输出，标准出错文件指针对应的文件描述符
    printf("stdin ->_fileno  = %d\n", stdin ->_fileno);
    printf("stdout->_fileno  = %d\n", stdout->_fileno);
    printf("stderr->_fileno  = %d\n", stderr->_fileno);

    // 打开文件：open
    /*
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>

    int open(const char *pathname, int flags);
    int open(const char *pathname, int flags, mode_t mode);
    参数2：打开模式
        以下三种模式必须选其一：O_RDONLY(只读)，O_WRONLY(只写)，O_RDWR(读写)
        以下的打开方式可以有零个或多个
        O_CREAT：如果文件不存在，则创建文件，如果文件存在，则打开文件
        flag中包含了该模式，则函数的第三个参数必须加上
        O_APPEND：以追加的形式打开文件，光标定位在结尾
        O_TRUNC ：清空文件
        O_EXCL：常跟O_CREAT一起使用，确保本次要创建一个新文件，
        如果文件已经存在，则open函数报错
        eg:
            "w" : O_WRONLY|O_CREAT|O_TRUNC
            "W+": O_RDWR|O_CREAT|O_TRUNC
            "r" : O_RDONLY
            "r+": O_RDWR
            "a" : O_WRONLY|O_CREAT|O_APPEND
            "a+": O_RDWR|O_CREAT|O_APPEND
    参数3：
        如果参数2中的flag中有O_CREAT时，表示创建新文件，
        参数3就必须给定，表示新创建的文件的权限
        如果当前参数给定了创建的文件权限，最终的结果也不一定是参数3的值，
        系统会用你给定的参数3的值，与系统的umask的值取反的值进行位与运算后，
        才是最终创建文件的权限（mode & ~umask)
        当前终端的umask的值，可以通过指令 umask 来查看，一般默认为为 0022，表示当
        前进程所在的组中对该文件没有写权限，其他用户对该文件也没有写权限
        通过指令：umask 数字可以进行更改，这种方式只对当前终端有效
        普通文件的权限一般为：0644，表示当前用户没有可执行权限，当前组中其他用户和其
        他组中的用户都只有读权限
        目录文件的权限一般为：0755，表示当前用户具有可读、可写、可执行，当前组中其他
        用户和其他组中的用户都没有可写权限
        注意：如果不给权限，那么当前创建的权限会是一个随机值
        返回值：成功返回打开文件的文件描述符，失败返回-1并置位错误码
    */

    // 关闭文件：close
    /*
    #include <unistd.h>

    int close(int fd);
    成功返回0，失败返回-1并置位错误码
    */

    // 1.定义文件描述符，对于文件IO而言，句柄就是文件描述符
    int fd = -1;

    if ((fd = open("./file.txt", O_WRONLY|O_CREAT, 0644)) == -1)
    {
        perror("open error");
        return -1;
    }

    printf("open success, fd = %d\n", fd);

    close(fd);

    // 读写操作：read,write
    /*
    #include <unistd.h>
    ssize_t read(int fd, void *buf, size_t count);
    成功返回读取的字符个数，这个个数可能会小于count的值，失败返回-1并置位错误码
    
    #include <unistd.h>
    ssize_t write(int fd, const void *buf, size_t count);
    成功返回写入的字符个数，这个个数可能会小于count的值，失败返回-1并置位错误码
    */

    fd = -1;
    if ((fd = open("./file.txt", O_WRONLY|O_CREAT, 0644)) == -1)
    {
        perror("open error");
        return -1;
    }

    char wbuf[128] = "hello world";
    write(fd, wbuf, strlen(wbuf));
    close(fd);

    if ((fd = open("./file.txt", O_RDONLY)) == -1)
    {
        perror("open error");
        return -1;
    }
    
    char rbuf[128] = "";
    int res = read(fd, rbuf, sizeof(rbuf));
    rbuf[strlen(rbuf)] = '\n';
    write(1, rbuf, res+1);

    close(fd);

    // 关于光标的操作：lseek
    /*
    #include <sys/types.h>
    #include <unistd.h>
    
    off_t lseek(int fd, off_t offset, int whence);
    参数3：偏移的起始位置
        SEEK_SET：文件起始位置
        SEEK_CUR：文件指针当前位置
        SEEK_END：文件结束位置
    返回值：光标现在所在的位置*/

    fd = -1;
    if ((fd = open("./file.txt", O_RDWR|O_CREAT|O_TRUNC, 0644)) == -1)
    {
        perror("open error");
        return -1;
    }
    write(fd, wbuf, strlen(wbuf));

    lseek(fd, 6, SEEK_SET);

    memset(rbuf, 0, sizeof(rbuf));
    res = read(fd, rbuf, sizeof(rbuf));
    rbuf[strlen(rbuf)] = '\n';
    write(1, rbuf, res+1);
    
    close(fd);
    
    return 0;
}