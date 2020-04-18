
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define OS_LINUX

// 这里可以修改使用者的用户名
const char USERNAME[] = "xiaosong";

#define FREE 0
#define DIRLEN 80
#define END 65535
#define SIZE 1024000
#define BLOCKNUM 1000
#define BLOCKSIZE 1024
#define MAXOPENFILE 10
#define ROOTBLOCKNUM 2

#define SAYERROR printf("ERROR: ")
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

#ifdef OS_LINUX
#define GRN "\x1B[32m"
#define RESET "\x1B[0m"
#endif //OS_LINUX

typedef struct FAT
{
  unsigned short id; //定义fat的ID
} fat;

typedef struct FCB
{
  char free; // 此fcb是否已被删除，因为把一个fcb从磁盘块上删除是很费事的，所以选择利用fcb的free标号来标记其是否被删除
  char exname[3];
  char filename[DIRLEN];
  unsigned short time;
  unsigned short data;
  unsigned short first;    // 文件起始盘块号
  unsigned long length;    // 文件的实际长度
  unsigned char attribute; // 文件属性字段：为简单起见，我们只为文件设置了两种属性：值为 0 时表示目录文件，值为 1 时表示数据文件
} fcb;

// 对于文件夹fcb，其count永远等于其fcb的length，
// 只有文件fcb的count会根据打开方式的不同和读写方式的不同而不同
typedef struct USEROPEN
{
  fcb open_fcb;     // 文件的 FCB 中的内容
  int count;        // 读写指针在文件中的位置
  int dirno;        // 相应打开文件的目录项在父目录文件中的盘块号
  int diroff;       // 相应打开文件的目录项在父目录文件的dirno盘块中的起始位置
  char fcbstate;    // 是否修改了文件的 FCB 的内容，如果修改了置为 1，否则为 0
  char topenfile;   // 表示该用户打开表项是否为空，若值为 0，表示为空，否则表示已被某打开文件占据
  char dir[DIRLEN]; // 打开文件的绝对路径名，这样方便快速检查出指定文件是否已经打开
} useropen;

typedef struct BLOCK0
{
  unsigned short root;
  char information[200];
  unsigned char *startblock;
} block0;

unsigned char *myvhard; //我的虚拟硬
useropen openfilelist[MAXOPENFILE];
int curdirid; // 指向用户打开文件表中的当前目录所在打开文件表项的位置

unsigned char *blockaddr[BLOCKNUM];
block0 initblock;
fat fat1[BLOCKNUM], fat2[BLOCKNUM];

char str[SIZE];

// others
void startsys()
{
  // 各种变量初始化
  myvhard = (unsigned char *)malloc(SIZE); //malloc函数是计算size即硬盘的内存的地址值并且返回给char类型的指针myvhard一个地址值，
                                           //2020-4-2至此
  for (int i = 0; i < BLOCKNUM; ++i)

    blockaddr[i] = i * BLOCKSIZE + myvhard;//给blockaddr赋值，blocksize加上size硬盘的内存地址，不断重复，直至初始化结束

  for (int i = 0; i < MAXOPENFILE; ++i)

    openfilelist[i].topenfile = 0;//首先初始化这个openfilelist数组令其=0

  // 准备读入 myfsys 文件信息
  FILE *fp = fopen("myfsys", "rb");//
  char need_format = 0;

  // 判断是否需要格式化
  if (fp != NULL)
  {
    unsigned char *buf = (unsigned char *)malloc(SIZE);
    fread(buf, 1, SIZE, fp);
    memcpy(myvhard, buf, SIZE);
    memcpy(&initblock, blockaddr[0], sizeof(block0));
    if (strcmp(initblock.information, "10101010") != 0)
      need_format = 1;
    free(buf);
    fclose(fp);
  }
  else
  {
    need_format = 1;
  }

  // 不需要格式化的话接着读入fat信息
  if (!need_format)
  {
    memcpy(fat1, blockaddr[1], sizeof(fat1));
    memcpy(fat2, blockaddr[3], sizeof(fat2));
  }
  else
  {
    printf("myfsys 文件系统不存在，现在开始创建文件系统\n");
    my_format();
  }

  // 把根目录fcb放入打开文件表中，设定当前目录为根目录
  curdirid = 0;
  memcpy(&openfilelist[curdirid].open_fcb, blockaddr[5], sizeof(fcb));
#ifdef DEBUG_INFO
  printf("starsys: %s\n", openfilelist[curdirid].open_fcb.filename);
#endif // DEBUG_INFO
  useropen_init(&openfilelist[curdirid], 5, 0, "~/");
}

void my_exitsys()
{
  // 先关闭所有打开文件项
  for (int i = 0; i < MAXOPENFILE; ++i)
    my_close(i);

  memcpy(blockaddr[0], &initblock, sizeof(initblock));
  memcpy(blockaddr[1], fat1, sizeof(fat1));
  memcpy(blockaddr[3], fat1, sizeof(fat1));
  FILE *fp = fopen("myfsys", "wb");

#ifndef DEBUG_DONT_SAVEFILE
  fwrite(myvhard, BLOCKSIZE, BLOCKNUM, fp);
#endif // DEBUG_DONT_SAVEFILE

  free(myvhard);
  fclose(fp);
}

void my_save(int fd)
{
  if (!check_fd(fd))
    return;

  useropen *file = &openfilelist[fd];
  if (file->fcbstate)
    fat_write(file->dirno, (unsigned char *)&file->open_fcb, file->diroff, sizeof(fcb));
  file->fcbstate = 0;
  return;
}

void my_close(int fd)
{
  if (!check_fd(fd))
    return;
  if (openfilelist[fd].topenfile == 0)
    return;

  // 若内容有改变，把fcb内容写回父亲的磁盘块中
  if (openfilelist[fd].fcbstate)
    my_save(fd);

  openfilelist[fd].topenfile = 0;
  return;
}

void my_cd(char *dirname)
{
  int fd = my_open(dirname);
  if (!check_fd(fd))
    return;
  if (openfilelist[fd].open_fcb.attribute)
  {
    my_close(fd);
    printf("%s is a file, please use open command\n", openfilelist[fd].dir);
    return;
  }

  // 得到的fd是文件夹的话，就把原来的目录关了,把现在的目录设为当前目录
  my_close(curdirid);
  curdirid = fd;
}

// others
// 启动系统，做好相关的初始化
void startsys();
// 退出系统，做好相应备份工作
void my_exitsys();
// 将一个打开文件的fat信息储存下来
void my_save(int fd);
// 关闭一个打开文件
void my_close(int fd);
// 利用my_open把当前目录切换到指定目录下
void my_cd(char *dirname);
