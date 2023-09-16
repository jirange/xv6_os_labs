#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/*在路径path下的目录树中查找与filename匹配的所有文件，输出文件的相对路径*/

char *fmtname(char *path) {
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ) return p;
  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));//用buf + strlen(p)中的当前位置的后面DIRSIZ - strlen(p)个字节用空格字符替代
  //return buf;
  return p;
}

void find(char *path, char *file_name) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_FILE:    //如果该文件是文件
        if(strcmp(fmtname(path),file_name) == 0){
            printf("%s\n", path);
        }
      break;

    case T_DIR:     //如果该文件是目录
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("find: path too long\n");
        break;
      }
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0) {
          printf("find: cannot stat %s\n", buf);
          continue;
        }
        if(strcmp(fmtname(buf),file_name) == 0){
            printf("%s\n", buf);
        }else{
            if (strcmp(fmtname(buf),".")!=0 && strcmp(fmtname(buf),"..")){    //d) 不要递归进入.和..；
                //c) 使用递归允许find进入到子目录；
                find(buf,file_name); //进入子目录查找
            }
        }
      }
      break;
  }
  close(fd);
}

int main(int argc,char* argv[]){
  //e) 测试时需要创建新的文件和文件夹，可使用make clean清理文件系统，并使用make qemu再编译运行。
  if (argc != 3) {
    printf("The format is incorrect. Please enter the command like:[find path file_name]\n");
    exit(-1);
  }
  find(argv[1],argv[2]);
  exit(0);
}
