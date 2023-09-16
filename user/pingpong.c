/*a) 使用pipe()创建管道，详见实验原理；

b) 使用fork()创建子进程，注意根据返回值，判断父子进程；

c) 利用read(), write()函数对管道进行读写。

d) 请在user/pingpong.c中实现。

e) 修改Makefile，将程序添加到UPROGS。*/

#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    
    //通过文件描述符，程序可以按读写文件的形式读写管道
    //int w_len = write(p[1],buffer,n);   //写 buffer的n个字节到文件描述符p[1] 返回n
    //int r_len = read(p[0],buffer,n);    //读 n个字节到buffer 返回读出的number 文件结尾eof返回0
    
    //文件描述符：xv6给文件分配的一个整数的ID 一个内核管理的对象 进程可对这个对象进行读写操作
    // 0:stdin 1:stdout 2:stderr

    //管道的读取是阻塞的 read会一直等待知道有书库刻度 write会一直等待知道管道不满，有空间可以写入 等待的前提是另一端没有完全关闭
    //进程通常支持幽默讴歌管道的一段，使用时需要将另一段关闭

    
    if(argc != 1){
        printf("pingpong don't need arguments!\n");
        exit(-1);
    }
    // a) 使用pipe()创建管道
    int p[2];
    /*int retp;
    //pipe 系统调用创建管道 同时返回两个文件描述符
    retp = pipe(p);  //p[0]:管道读出端 p[1]:写入端*/
    pipe(p);
    int p2[2];
    pipe(p2);

    // b) 使用fork()创建子进程，注意根据返回值，判断父子进程；
/* 子进程读管道，父进程写管道ping */
char buffer[10] = "ping";
    int n = strlen(buffer);
char buffer2[10] = "pong";
        int n2 = strlen(buffer);
int ret = fork();
if (ret == 0) { 
    /* 子进程 */
    close(p[1]); // 关闭写端
    char r_buffer[10];
    int r_len = read(p[0],r_buffer,n);
    close(p[0]); // 读取完成，关闭读端
    printf("%d: received ping\n",getpid());
    if(r_len != 0){
        close(p2[0]);//提前关闭管道中不使用的一段
        write(p2[1],buffer2,n2);
        close(p2[1]); // 写入完成，关闭写端
    }
   // kill(getpid());
} else if (ret>0) { 
    /* 父进程 */
    close(p[0]); // 关闭读端
    
    write(p[1],buffer,n);
    close(p[1]); // 写入完成，关闭写端

    char r_buffer[10];
    close(p2[1]);
    read(p2[0],r_buffer,n2);
        //int r_len = read(p[0],r_buffer,n);

    close(p2[0]); // 读取完成，关闭读端
    printf("%d: received %s\n",getpid(),r_buffer);
       // kill(getpid());

}
    exit(0);
}
