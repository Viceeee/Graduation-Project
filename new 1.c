
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


// utils
// 这里是一些基础的函数，都是一些会重复利用的操作
// 根据所给的参数对fcb进行初始化
void fcb_init(fcb *new_fcb, const char* filename, unsigned short first, unsigned char attribute);
// 根据所给的参数对一个打开文件项进行初始化
void useropen_init(useropen *openfile, int dirno, int diroff, const char* dir);
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
void fcb_init(fcb *new_fcb, const char* filename, unsigned short first, unsigned char attribute) {
  strcpy(new_fcb->filename, filename); //这个strcpy(new_fcb->filename, filename)里面的内容是将filename的字符串复制给new_fcb,->意思是把filename的值给new_fcb
									   //(pointer_name)->(variable_name) 
									   //Operation: The -> operator in C or C++ gives the value held by variable_name to structure or union variable pointer_name 
  new_fcb->first = first;   		   //将short类型的first文件起始盘块号赋值给new_fcb   first是文件起始盘块号
  new_fcb->attribute = attribute;  	   //将char类型的attribute赋值给new_fcb attribute是文件属性字段，0为目录，1为文件
  new_fcb->free = 0;				   //标记new_fcb没有被删除
  if (attribute) new_fcb->length = 0;  //如果attribute==1，则将文件实际长度length=0赋值给new_fcb ，方便编辑文件
  else new_fcb->length = 2 * sizeof(fcb);//如果attribute ==0，则是文件夹，将文件实际长度=2*fcb的长度赋值给new_fcb
  //对于文件夹fcb，其count永远等于其fcb的length，
  // 只有文件fcb的count会根据打开方式的不同和读写方式的不同而不同
}

// 根据所给的参数对一个打开文件项进行初始化
void useropen_init(useropen *openfile, int dirno, int diroff, const char* dir) {
  openfile->dirno = dirno;//dirno相应打开文件的目录项在父目录文件中的盘块号
  openfile->diroff = diroff;//diroff相应关闭文件的目录项在父目录中的盘块号
  strcpy(openfile->dir, dir);//将dir字符串的值复制给openfile->dir,然后dir的值给openfile
  openfile->fcbstate = 0; //是否修改了文件的 FCB 的内容，如果修改了置为 1，否则为 0
  openfile->topenfile = 1;// 表示该用户打开表项是否为空，若值为 0，表示为空，否则表示已被某打开文件占据
  openfile->count = openfile->open_fcb.length;
}

// 得到一个空闲的fat块
void fatFree(int id) {
  if (id == END) return;
  if (fat1[id].id != END) fatFree(fat1[id].id);
  fat1[id].id = FREE;
}




int getFreeFatid() {
  for (int i = 5; i < BLOCKNUM; ++i) if (fat1[i].id == FREE) return i;
  return END;
}

int getFreeOpenlist() {
  for (int i = 0; i < MAXOPENFILE; ++i) if (!openfilelist[i].topenfile) return i;
  return -1;
}

int getNextFat(int id) {
  if (fat1[id].id == END) fat1[id].id = getFreeFatid();
  return fat1[id].id;
}

int check_fd(int fd) {
  if (!(0 <= fd && fd < MAXOPENFILE)) {
    SAYERROR;
    printf("check_fd: %d is invaild index\n", fd);
    return 0;
  }
  return 1;
}

int spiltDir(char dirs[DIRLEN][DIRLEN], char *filename) {
  int bg = 0; int ed = strlen(filename);
  if (filename[0] == '/') ++bg;
  if (filename[ed - 1] == '/') --ed;

  int ret = 0, tlen = 0;
  for (int i = bg; i < ed; ++i) {
    if (filename[i] == '/') {
      dirs[ret][tlen] = '\0';
      tlen = 0;
      ++ret;
    }
    else {
      dirs[ret][tlen++] = filename[i];
    }
  }
  dirs[ret][tlen] = '\0';

  return ret+1;
}

void popLastDir(char *dir) {
  int len = strlen(dir) - 1;
  while (dir[len - 1] != '/') --len;
  dir[len] = '\0';
}

void splitLastDir(char *dir, char new_dir[2][DIRLEN]) {
  int len = strlen(dir);
  int flag = -1;
  for (int i = 0; i < len; ++i) if (dir[i] == '/') flag = i;

  if (flag == -1) {
    SAYERROR;
    printf("splitLastDir: can\'t split %s\n", dir);
    return;
  }

  int tlen = 0;
  for (int i = 0; i < flag; ++i) {
    new_dir[0][tlen++] = dir[i];
  }
  new_dir[0][tlen] = '\0';
  tlen = 0;
  for (int i = flag + 1; i < len; ++i) {
    new_dir[1][tlen++] = dir[i];
  }
  new_dir[1][tlen] = '\0';
}

void getPos(int *id, int *offset, unsigned short first, int length) {
  int blockorder = length >> 10;
  *offset = length % 1024;
  *id = first;
  while (blockorder) {
    --blockorder;
    *id = fat1[*id].id;
  }
}

int rewrite_dir(char *dir) {
  int len = strlen(dir);
  if (dir[len-1] == '/') --len;
  int pre = -1;
  for (int i = 0; i < len; ++i) if (dir[len] == '/') {
    if (pre != -1) {
      if (pre + 1 == i) {
        printf("rewrite_dir: %s is invaild, please check!\n", dir);
        return 0;
      }
    }
    pre = i;
  }
  char newdir[len];
  if (dir[0] == '/') {
    strcpy(newdir, "~");
  }
  else {
    strcpy(newdir, openfilelist[curdirid].dir);
  }
  strcat(newdir, dir);
  strcpy(dir, newdir);
  return 1;
}
