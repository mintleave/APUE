#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
int main(int argc, const char *argv[]) {
    
    // FILE 结构体
    /*
    struct FILE {
        char *_IO_buf_base;  // 缓冲区的起始地址
        char *_IO_buf_end;   // 缓冲区的终止地址

        int _fileno;         // 文件表述符，用于进行系统调用
    }
    */

    /*
    特殊的FILE指针，这三个指针，全部都是针对于终端文件而言的，当程序启动后，系统默认打开的
    三个特殊文件指针
    stderr   --标准出错指针
    stdin    --标准输入指针
    stdout   --标准输出指针
    */

    // 1.定义一个文件指针
    FILE *fp = NULL;

    // fopen 函数
    /*
    #include <stdio.h>
    FILE *fopen(const char *path, const char *mode);
    r   以只读的形式打开文件，文件光标定位在开头部分
    r+  以读写的形式打开文件，文件光标定位在开头部分
    w   以只写的形式打开文件，如果文件不存在，就创建文件，如果文件存在，就清空，光标定位在开头
    w+  以读写的形式打开文件，如果文件不存在，就创建文件，如果文件存在，就清空，光标定位在开头
    a   以追加的形式打开文件，如果文件不存在就创建文件，光标定位在文件末尾
    a+  以读写的形式打开文件，如果文件不存在，就创建文件，否则，开头读，结尾写     
    */

    // 以只读的形式打开一个不存在的文件，并将结果存入到fp指针中
    // fp = fopen("./file.txt","r");
    // 此时会报错，原因是以只读的形式打开一个存在的文件，是不允许的

    // 以只写的形式打开一个不存在的文件，如果文件不存在就创建一个空的文件，如果文件存在就清空
    if((fp = fopen("./file.txt", "w")) == NULL)
    {
        // 关于错误码的处理函数  strerror,perror
        /*
        #include <errno.h>     -- 错误码所在的头文件
        int errno;
        -- 将错误码对应的错误信息转换处理
        #include <string.h>
        char *strerror(int errnum);
        */
        perror("fopen error\n");
        return -1;
    }
    printf("fopen success!\n");

    fputc('H', fp);
    fputc('e', fp);
    fputc('l', fp);
    fputc('l', fp);
    fputc('o', fp);

    // fclose 函数
    // 成功返回0，失败返回EOF并置位错误码
    fclose(fp);

    if((fp = fopen("./file.txt", "r")) == NULL) {
        perror("fopen error");
        return -1;
    }

    char ch = 0;
    while(1) {
        ch = fgetc(fp);
        if(ch == EOF) {
            break;
        }
        printf("%c", ch);
    }
    printf("\n");

    fclose(fp);

    if((fp = fopen("./file.txt", "a")) == NULL) {
        perror("fopen error");
        return -1;
    }

    char wbuf[128] = "World!";
    
    // fputs函数
    /*
    int fputs(const char *s, FILE *stream);
    成功返回本次写入字符的个数，失败返回EOF
    */
    fputs(wbuf, fp);
    fputc('\n', fp);
    
    fclose(fp);

    if((fp = fopen("./file.txt", "r")) == NULL) {
        perror("fopen error");
        return -1;
    }

    for (int i = 0; i < 5; i++)
    {
        char ch = fgetc(fp);
        printf("%c", ch);
    }

    printf(" ");
    
    // fgets函数
    /*
    char *fgets(char *s, int size, FILE *stream);
    从stream指向的文件中最多读取size-1个字符到s容器中
    遇到回车或文件结束，会结束一次读取，并且会将回车放入容器
    最后自动加上一个字符串结束表识'\0'
    成功返回容器s的起始地址，失败返回NULL
    */
    char rbuf[128] = "";
    while (1) {
        char *res = fgets(rbuf, sizeof(rbuf), fp);
        if (res == NULL)
        {
            break;
        }
         
    }
    printf("%s", rbuf);

    fclose(fp);

    // 关于标准IO的缓冲区问题
    /*
    缓冲区分为三种:
    行缓存: 和终端文件相关的缓冲区叫做行缓存，行缓冲区的大小为1024字节，
    对应的文件指针: stdin,stdout
    全缓存: 和外界文件相关的缓冲区叫做全缓存，全缓冲区的大小为4096字节，
    对应的文件指针: FILE*
    不缓存: 和标准出错相关的缓冲区叫不缓存，不缓存的大小为0字节
    对应的文件指针: stderr
    */

    // 如果缓冲区没有被使用时，大小为0，只有被使用了一次后，缓冲区的大小就被分配了
    printf("行缓存的大小为: %d\n", stdout->_IO_buf_end-stdout->_IO_buf_base);
    if((fp = fopen("./file.txt", "r")) == NULL) {
        perror("fopen error");
        return -1;
    }
    fgetc(fp);
    printf("全缓存的大小为: %d\n", fp->_IO_buf_end-fp->_IO_buf_base);
    fclose(fp);
    perror("error");
    printf("不缓存的大小为: %d\n", stderr->_IO_buf_end-stderr->_IO_buf_base);

    // 缓冲区的刷新时机
    /*
    缓冲区刷新函数: fflush
    #include <stdio.h>
    int fflush(FILE *stream);
    成功返回0，失败返回EOF并置位错误码
    */

    // 行缓存的刷新时机
    /* 1. 缓冲区如果没有达到刷新时机，就不会将数据进行刷新
    printf("Hello World!");
    while(1);
    不会输出数据
    */

    /* 2.当程序结束后，会刷新行缓冲区
    printf("Hello World");
    */

    /* 3.当遇到换行时，会刷新行缓存
    printf("Hello World!\n");
    while(1);
    */

    /* 4.当输入输出发生切换时，也会刷新行缓存
    int num = 0;
    printf("请输入>>>");
    scanf("%d", &num);
    */

    /* 5.当关闭行缓存对应的文件指针时，也会刷新行缓存
    printf("Hello World!");
    fclose(stdout);
    while(1);
    */

    /* 6.使用fflush函数手动刷新缓冲区时，行缓存会被刷新
    printf("Hello World!");
    fflush(stdout);
    while(1);
    */

    /* 7.当缓冲区满了后，会刷新行缓存，行缓存大小：1024字节
    for(int i = 0; i < 1025; i++) {
        printf("A");
    }
    while(1);
    */

    // 全缓存刷新机制
    /* 打开一个文件
    FILE *fp = NULL
    if((fp = open("./file.txt","r+")) == NULL) {
        perror("open error");
        return -1;
    }
    */

    /* 1.当缓冲区刷新时机未到时，不会刷新全缓存
    fputs("hello world", fp);
    while(1);
    */

    /* 2.当遇到换行时，也不会刷新全缓存
    fputs("hello world\n");
    while(1);
    */

    /* 3.当程序结束后，会刷新全缓存
    fputs("hello world\n", fp);
    */

    /* 4.当输入输出发生切换时，会刷新全缓存
    fputs("hello world\n", fp);
    fgetc(fp);
    while(1);
    */

    /* 5.当关闭缓冲区对应的文件指针时，也会刷新全缓存
    fputs(fp);
    fclose(fp);
    while(1);
    */

    /* 6.当手动刷新缓冲区对应的文件指针时，也会刷新全缓存
    fputs("hello world\n", fp);
    fflush(fp);
    while(1);
    */

    /* 7.当缓冲区满了以后，再向缓冲区中存放数据时会刷新全缓冲
    for(int i = 0; i < 4097; i++) {
        fputs('A', fp);
    }
    while(1);
    */

    // 对于不缓存的刷新机制，只要放入数据，立马进行刷新
    /*
    perror("A");
    while(1);
    */

    // 格式化读写: fprintf,fscanf
    /*
    int fprintf(FILE *stream, const char *format, ...);
    成功返回输出的字符个数，失败返回一个负数

    int fscanf(FILE *stream, const char *format,...);
    成功返回读入的项数，失败返回EOF并置位错误码
    */

    if ((fp = fopen("./file.txt", "r")) == NULL)
    {
        perror("open error");
        return -1;
    }
    char buf[128] = "";
    fscanf(fp, "%s", buf);
    fprintf(stdout, "%s\n", buf);
    fclose(fp);

    // 格式串转字符串存入字符数组中：sprintf,snprintf
    /*
    int sprintf(char *str, const char *format,...);
    成功返回转换的字符个数，失败返回EOF
    对于上述函数而言，用一个小的容器去存储一个大的转换后的字符时，
    会出现指针越界的段错误，为了安全起见，引入了snprintf

    int snprintf(char *str, size_t size, const char *format,...);
    如果转换的字符小于size，返回值就是成功转换子符的个数，
    如果大于size则只转换size-1个字符，返回值就是size，失败返回EOF
    */ 
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s", "hello world!");
    printf("buf = %s\n", buf);

    // 模块化读写 fread,fwrite
    /*
    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    从stream指向的文件中，读取nmemb项数据，每一项的大小为size,
    将整个结果放入ptr指向的容器中
    成功返回nmemb，就是成功读取的项数，失败返回小于项数的值，或是0

    size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
    从stream指向的文件中，写入nmemb项数据，每一项的大小为size,
    数据的起始地址为ptr
    成功返回写入的项数，失败返回小于项数的值，或是0
    */

    // 1.字符串的读写
    if ((fp = fopen("./file.txt", "w")) == NULL)
    {
        perror("open error");
        return -1;
    }
    memset(wbuf, 0, sizeof(wbuf));
    snprintf(wbuf, sizeof(wbuf), "%s", "2233");
    fwrite(wbuf, strlen(wbuf), 1, fp);
    fflush(fp);
    fclose(fp);
    if ((fp = fopen("./file.txt", "r")) == NULL)
    {
        perror("open error");
        return -1;
    }
    memset(rbuf, 0, sizeof(rbuf));
    int res = fread(rbuf, 1, sizeof(rbuf), fp);
    rbuf[strlen(rbuf)] = '\n';
    fwrite(rbuf, 1, res+1, stdout);
    fclose(fp);

    // 2.整数的读写
    if ((fp = fopen("./file.txt", "w")) == NULL)
    {
        perror("open error");
        return -1;
    }
    int num = 2233;
    fwrite(&num, sizeof(num), 1, fp);
    fclose(fp);
    if ((fp = fopen("./file.txt", "r")) == NULL)
    {
        perror("open error");
        return -1;
    }
    int key = 0;
    fread(&key, sizeof(key), 1, fp);
    fclose(fp);
    printf("key = %d\n", key);

    // 3.结构体数据的读写
    class stu {
    public:
        char name[20];
        int age;
        double score;
    };
    if ((fp = fopen("./file.txt", "w")) == NULL)
    {
        perror("open error");
        return -1;
    }
    stu s[3] = {{"张三",18,98},{"李四",20,88},{"王五",16,95}};
    fwrite(s, sizeof(stu), 3, fp);
    fclose(fp);
    if ((fp = fopen("./file.txt", "r")) == NULL)
    {
        perror("open error");
        return -1;
    }
    stu tem;
    fread(&tem, sizeof(tem), 1, fp);
    printf("name: %s,age: %d,score: %.2lf\n",tem.name, tem.age, tem.score);
    fclose(fp);

    // 关于文件光标：fseek,ftell,rewind
    /*
    int fssek(FILE *stream, long offset, int whence);
    参数3:
        SEEK_SET：  文件起始位置
        SEEK_CUR：  文件指针当前位置
        SEEK_END：  文件结束位置
    返回值：成功返回0，失败返回-1并置位错误码
    
    long ftell(FILE *stream);
    获取文件指针当前的偏移量
    成功返回文件指针所在的位置，失败返回-1并置位错误码
    eg: fseek(fp, 0, SEEK_END);
        ftell(fp);
        此时该函数的返回值就是文件大小
    
    void rewind(FILE *stream);
    功能：将文件光标定位在开头
    */

    if ((fp = fopen("./file.txt", "w+")) == NULL)
    {
        perror("open error");
        return -1;
    }
    fwrite(s, sizeof(stu), 3, fp);

    // 求此时文件的大小
    printf("此时文件的大小为：%ld\n", ftell(fp));
    // 将光标移动到开头位置
    fseek(fp, 0, SEEK_SET);
    // 获取第二个学生信息
    fseek(fp, sizeof(stu), SEEK_SET);

    fread(&tem, sizeof(stu), 1, fp);
    printf("name: %s,age: %d,score: %.2lf\n",tem.name, tem.age, tem.score);
    fclose(fp);

    return 0;
}