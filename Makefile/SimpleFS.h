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
    //  把一个路径的最后一个目录从字符串中删去
    void popLastDir(char *dir);
    // 把一个路径的最后一个目录从字符串中分割出
    void splitLastDir(char *dir, char new_dir[2][DIRLEN]);
    // 得到某个长度在某个fat中的对应的盘块号和偏移量，用来记录一个打开文件项在其父目录对应fat的位置
    void getPos(int *id, int *offset, unsigned short first, int length);
    // 把路径规范化并检查
    int rewrite_dir(char *dir);

    // basics
    // 根据盘块号和偏移量，直接从FAT上读取指定长度的信息
    int fat_read(unsigned short id, unsigned char *text, int offset, int len);
    /   / 读取某个已打开文件的指定长度信息
    int do_read(int fd, unsigned char *text, int len);
    // 根据盘块号和偏移量，直接从FAT上写入指定长度的信息
    int fat_write(unsigned short id, unsigned char *text, int blockoffset, int len);    
    // 向某个已打开文件写入指定长   度信息
    int do_write(int fd, unsigned char *text, int len); 
    // 从一个已打开目录文件找到对应名称的文件夹fcb，用  于一些不断递归打开文件夹的函数中
    int     getFcb(fcb* fcbp, int    *dirno, int *diroff, int fd, const char *dir);
    // 在   一个已打开目录文件下打开   某个文件
    int getOpenlist(int fd, const char *org_dir);
    // 打开文件
    int my_open(char *filename);

    // read
    // 读取一个文件夹下的fcb信息
    int read_ls(int fd, unsigned char *text, int len);
    // 把一个文件夹下的fcb信息打印出来
    void my_ls();
    // 把一个打开文件的内容根据文件指针打印出来
    int my_read(int fd);
    // 重新从磁盘中读取一个打开文件的fcb内容
    void my_reload(int fd);

    // write
    // 把键盘输入的信息写入一个打开文件
    int my_write(int fd);

    // delete
    // 把一个指定目录的fcb的free置为1
    void my_rmdir(char *dirname);
    // 把一个指定文件的fcb的free置为1
    void my_rm(char *filename);

    // creat
    // 格式化
    void my_format();
    // 在指定目录下创建一个文件或文件夹
    int my_touch(char *filename, int attribute, int *rpafd);
    // 调用touch创建出一个文件
    int my_create(char *filename);
    // 调用touch创建出一个文件夹
    void my_mkdir(char *dirname);

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
