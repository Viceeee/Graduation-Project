
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

unsigned char *myvhard;
useropen openfilelist[MAXOPENFILE];
int curdirid; // 指向用户打开文件表中的当前目录所在打开文件表项的位置

unsigned char *blockaddr[BLOCKNUM];
block0 initblock;
fat fat1[BLOCKNUM], fat2[BLOCKNUM];

char str[SIZE];

int main()
{
    //fd是一个抽象指示符，用来给输入输出源权限，每个Unix进程，都有3个fd，分别是
    // 0        stdinput
    // 1        stdoutput
    // 2        stderror
    int fd;

    char command[DIRLEN << 1];//为什么要把DIRLEN左移一位？

    startsys();
    printf("%s %s: ", USERNAME, openfilelist[curdirid].dir);

    while (~scanf("%s", command) && strcmp(command, "exit"))
    {
        if (!strcmp(command, "ls"))
        {
            my_ls();
        }
        else if (!strcmp(command, "mkdir"))
        {
            scanf("%s", command);
            if (rewrite_dir(command))
                my_mkdir(command);
        }
        else if (!strcmp(command, "close"))
        {
            scanf("%d", &fd);
            my_close(fd);
        }
        else if (!strcmp(command, "open"))
        {
            scanf("%s", command);
            if (!rewrite_dir(command))
                continue;
            fd = my_open(command);
            if (0 <= fd && fd < MAXOPENFILE)
            {
                if (!openfilelist[fd].open_fcb.attribute)
                {
                    my_close(fd);
                    printf("%s is dirictory, please use cd command\n", command);
                }
                else
                {
                    printf("%s is open, it\'s id is %d\n", openfilelist[fd].dir, fd);
                }
            }
        }
        else if (!strcmp(command, "cd"))
        {
            scanf("%s", command);
            if (rewrite_dir(command))
                my_cd(command);
        }
        else if (!strcmp(command, "create"))
        {
            scanf("%s", command);
            if (!rewrite_dir(command))
                continue;
            fd = my_create(command);
            if (0 <= fd && fd < MAXOPENFILE)
            {
                printf("%s is created, it\'s id is %d\n", openfilelist[fd].dir, fd);
            }
        }
        else if (!strcmp(command, "rm"))
        {
            scanf("%s", command);
            if (rewrite_dir(command))
                my_rm(command);
        }
        else if (!strcmp(command, "rmdir"))
        {
            scanf("%s", command);
            if (rewrite_dir(command))
                my_rmdir(command);
        }
        else if (!strcmp(command, "read"))
        {
            scanf("%d", &fd);
            my_read(fd);
        }
        else if (!strcmp(command, "write"))
        {
            scanf("%d", &fd);
            my_write(fd);
        }
        else if (!strcmp(command, "sf"))
        {
            for (int i = 0; i < MAXOPENFILE; ++i)
            {
                if (openfilelist[i].topenfile)
                    printf("  %d : %s\n", i, openfilelist[i].dir);
            }
        }
        else if (!strcmp(command, "format"))
        {
            scanf("%s", command);
            my_format();
        }
        else
        {
            printf("command %s : no such command\n", command);
        }

        my_reload(curdirid);
        printf("%s %s: ", USERNAME, openfilelist[curdirid].dir);
    }

    my_exitsys();
    return 0;
}