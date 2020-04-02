
/*
#include <stdio.h>
int main ()
{
   int  var = 20;   /* 实际变量的声明 /
   int  *ip;        /* 指针变量的声明 /
 
   ip = &var;  /* 在指针变量中存储 var 的地址 /
 
   printf("Address of var variable: %p\n", &var  );
 
   /* 在指针变量中存储的地址 /
   printf("Address stored in ip variable: %p\n", ip );
 
   /* 使用指针访问值 /
   printf("Value of *ip variable: %d\n", *ip );
 
   return 0;
}
*/

//  a) 出口条件，即递归“什么时候结束”，这个通常在递归函数的开始就写好;
//  b) 如何由"情况 n" 变化到"情况 n+1", 也就是非出口情况，也就是一般情况——"正在"递归中的情况;
//  c) 初始条件，也就是这个递归调用以什么样的初始条件开始

#define FREE 0         //定义FREE为0
#define DIRLEN 80      //定义dirlen的长度为80
#define END 65535      //定义结束盘号为65535
#define SIZE 1024000   //定义大小为1024000
#define BLOCKNUM 1000  //定义block数量为1000
#define BLOCKSIZE 1024 //定义block的大小为1024
#define MAXOPENFILE 10 //最大打开文件数10
#define ROOTBLOCKNUM 2 //根区块为2

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
// utils
// 这里是一些基础的函数，都是一些会重复利用的操作

// 根据所给的参数对fcb进行初始化
void fcb_init(fcb *new_fcb, const char *filename, unsigned short first, unsigned char attribute);

// 根据所给的参数对一个打开文件项进行初始化
void useropen_init(useropen *openfile, int dirno, int diroff, const char *dir);

// 通过dfs对将已使用的fat块释放
void fatFree(int id);

// 得到一个空闲的fat块
int getFreeFatid();

// 得到一个空闲的打开文件表项
int getFreeOpenlist();

// 得到一个fat表的下一个fat表，如果没有则创建
int getNextFat(int id);

// 检查一个打开文件表下表是否合法
int check_fd(int fd);

// 把一个路径按'/'分割
int spiltDir(char dirs[DIRLEN][DIRLEN], char *filename);

// 把一个路径的最后一个目录从字符串中删去
void popLastDir(char *dir);

// 把一个路径的最后一个目录从字符串中分割出
void splitLastDir(char *dir, char new_dir[2][DIRLEN]);

// 得到某个长度在某个fat中的对应的盘块号和偏移量，用来记录一个打开文件项在其父目录对应fat的位置
void getPos(int *id, int *offset, unsigned short first, int length);

// 把路径规范化并检查
int rewrite_dir(char *dir);

// 根据所给的参数对fcb进行初始化

void fcb_init(fcb *new_fcb, const char *filename, unsigned short first, unsigned char attribute)
{
  strcpy(new_fcb->filename, filename); //这个strcpy(new_fcb->filename, filename)里面的内容是将filename的字符串复制给new_fcb,->意思是把filename的值给new_fcb，
                                       //反正意思就是吧filename的字符串的值通过new_fcb复制给filename
                                       //(pointer_name)->(variable_name)
                                       //Operation: The -> operator in C or C++ gives the value held by variable_name to structure or union variable pointer_name
  new_fcb->first = first;              //将short类型的first文件起始盘块号赋值给new_fcb   first是文件起始盘块号
  new_fcb->attribute = attribute;      //将char类型的attribute赋值给new_fcb attribute是文件属性字段，0为目录，1为文件
  new_fcb->free = 0;                   //标记new_fcb没有被删除
  if (attribute)
    new_fcb->length = 0; //如果attribute==1，则将文件实际长度length=0赋值给new_fcb ，方便编辑文件
  else
    new_fcb->length = 2 * sizeof(fcb); //如果attribute ==0，则是文件夹，将文件实际长度=2*fcb的长度赋值给new_fcb

  //对于文件夹fcb，其count永远等于其fcb的length，
  // 只有文件fcb的count会根据打开方式的不同和读写方式的不同而不同
}

// 根据所给的参数对一个打开文件项进行初始化
void useropen_init(useropen *openfile, int dirno, int diroff, const char *dir)
{
  openfile->dirno = dirno;    //dirno相应打开文件的目录项在父目录文件中的盘块号
  openfile->diroff = diroff;  //diroff相应关闭文件的目录项在父目录中的盘块号
  strcpy(openfile->dir, dir); //将dir字符串的值复制给openfile->dir,然后dir的值给openfile
  openfile->fcbstate = 0;     //是否修改了文件的 FCB 的内容，如果修改了置为 1，否则为 0
  openfile->topenfile = 1;    // 表示该用户打开表项是否为空，若值为 0，表示为空，否则表示已被某打开文件占据
  openfile->count = openfile->open_fcb.length;
}

// 通过dfs得到一个空闲的fat块
void fatFree(int id)
{
  //是个dfs算法，递归
  if (id == END) //边界条件,这个id是在fatfree中的id
    return;

  if (fat1[id].id != END) //fat1的blocknumber，调用FAT数据结构的id来确定此时fatfree的id在fat1的那个地方
    fatFree(fat1[id].id); //deep-first search 深搜算法，去到最底层的blocknum，
  //只要没到最后一个id的fatfree函数都卡在这里，妙啊。
  //else
  fat1[id].id = FREE;
  //应该省略了一个return fatfree(fat[id].id)?
}

// 得到一个空闲的fat块，这个和之前的哪个函数有关系的
int getFreeFatid()
{
  for (int i = 5; i < BLOCKNUM; ++i)//为什么是从i=5开始,
  // i++和++i的区别是i++会创建一个临时变量使用这个临时变量来计算，而++i不会创建临时变量
  {
    if (fat1[i].id == FREE)//和上面的哪个函数有关系，假如fat1的全部都是空闲的了，则返回一个数字
      return i;
  }
  return END;
}

int getFreeOpenlist()
{
  for (int i = 0; i < MAXOPENFILE; ++i)
    if (!openfilelist[i].topenfile)
      return i;
  return -1;
}

int getNextFat(int id)
{
  if (fat1[id].id == END)
    fat1[id].id = getFreeFatid();
  return fat1[id].id;
}

int check_fd(int fd)
{
  if (!(0 <= fd && fd < MAXOPENFILE))
  {
    SAYERROR;
    printf("check_fd: %d is invaild index\n", fd);
    return 0;
  }
  return 1;
}

int spiltDir(char dirs[DIRLEN][DIRLEN], char *filename)
{
  int bg = 0;
  int ed = strlen(filename);
  if (filename[0] == '/')
    ++bg;
  if (filename[ed - 1] == '/')
    --ed;

  int ret = 0, tlen = 0;
  for (int i = bg; i < ed; ++i)
  {
    if (filename[i] == '/')
    {
      dirs[ret][tlen] = '\0';
      tlen = 0;
      ++ret;
    }
    else
    {
      dirs[ret][tlen++] = filename[i];
    }
  }
  dirs[ret][tlen] = '\0';

  return ret + 1;
}

void popLastDir(char *dir)
{
  int len = strlen(dir) - 1;
  while (dir[len - 1] != '/')
    --len;
  dir[len] = '\0';
}

void splitLastDir(char *dir, char new_dir[2][DIRLEN])
{
  int len = strlen(dir);
  int flag = -1;
  for (int i = 0; i < len; ++i)
  {
    if (dir[i] == '/')
      flag = i;
  }
  if (flag == -1)
  {
    SAYERROR;
    printf("splitLastDir: can\'t split %s\n", dir);
    return;
  }

  int tlen = 0;
  for (int i = 0; i < flag; ++i)
  {
    new_dir[0][tlen++] = dir[i];
  }
  new_dir[0][tlen] = '\0';
  tlen = 0;
  for (int i = flag + 1; i < len; ++i)
  {
    new_dir[1][tlen++] = dir[i];
  }
  new_dir[1][tlen] = '\0';
}

void getPos(int *id, int *offset, unsigned short first, int length)
{
  int blockorder = length >> 10;
  *offset = length % 1024;
  *id = first;
  while (blockorder)
  {
    --blockorder;
    *id = fat1[*id].id;
  }
}

int rewrite_dir(char *dir)
{
  int len = strlen(dir);
  if (dir[len - 1] == '/')
    --len;
  int pre = -1;
  for (int i = 0; i < len; ++i)
    if (dir[len] == '/')
    {
      if (pre != -1)
      {
        if (pre + 1 == i)
        {
          printf("rewrite_dir: %s is invaild, please check!\n", dir);
          return 0;
        }
      }
      pre = i;
    }
  char newdir[len];
  if (dir[0] == '/')
  {
    strcpy(newdir, "~");
  }
  else
  {
    strcpy(newdir, openfilelist[curdirid].dir);
  }
  strcat(newdir, dir);
  strcpy(dir, newdir);
  return 1;
}
