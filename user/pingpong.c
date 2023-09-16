#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){

    if(argc != 1){
        printf("pingpong don't need arguments!\n");
        exit(-1);
    }
    // a) 使用pipe()创建管道
    int p[2];
    //pipe 系统调用创建管道 同时返回两个文件描述符 //p[0]:管道读出端 p[1]:写入端*/
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
        close(p2[0]); // 读取完成，关闭读端
        printf("%d: received %s\n",getpid(),r_buffer);
        // kill(getpid());

    }
    exit(0);
}
