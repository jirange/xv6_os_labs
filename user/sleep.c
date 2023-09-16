#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    if(argc != 2){
        printf("sleep needs one argument!\n");
        exit(-1);
    }
    int ticks = atoi(argv[1]);  //将字符串参数转为整数
    sleep(ticks);               //系统调用sleep
    printf("(nothing happens for a little while)\n");
    exit(0);
}

/*printf("argc=%d,argv = %s,%s,%s\n",argc,argv[0],argv[1],argv[2]);
    printf("argv=%d\n",sizeof(*argv));
*/