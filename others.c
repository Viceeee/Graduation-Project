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
// others
void startsys() {
  // 各种变量初始化
  myvhard = (unsigned char*)malloc(SIZE);
  for (int i = 0; i < BLOCKNUM; ++i) blockaddr[i] = i * BLOCKSIZE + myvhard;
  for (int i = 0; i < MAXOPENFILE; ++i) openfilelist[i].topenfile = 0;

  // 准备读入 myfsys 文件信息
  FILE *fp = fopen("myfsys", "rb");
  char need_format = 0;

  // 判断是否需要格式化
  if (fp != NULL) {
    unsigned char *buf = (unsigned char*)malloc(SIZE);
    fread(buf, 1, SIZE, fp);
    memcpy(myvhard, buf, SIZE);
    memcpy(&initblock, blockaddr[0], sizeof(block0));
    if (strcmp(initblock.information, "10101010") != 0) need_format = 1;
    free(buf);
    fclose(fp);
  }
  else {
    need_format = 1;
  }

  // 不需要格式化的话接着读入fat信息
  if (!need_format) {
    memcpy(fat1, blockaddr[1], sizeof(fat1));
    memcpy(fat2, blockaddr[3], sizeof(fat2));
  }
  else {
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

void my_exitsys() {
  // 先关闭所有打开文件项
  for (int i = 0; i < MAXOPENFILE; ++i) my_close(i);
  
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

void my_save(int fd) {
  if (!check_fd(fd)) return;

  useropen *file = &openfilelist[fd];
  if (file->fcbstate) fat_write(file->dirno, (unsigned char *)&file->open_fcb, file->diroff, sizeof(fcb));
  file->fcbstate = 0;
  return;
}

void my_close(int fd) {
  if (!check_fd(fd)) return;
  if (openfilelist[fd].topenfile == 0) return;

  // 若内容有改变，把fcb内容写回父亲的磁盘块中
  if (openfilelist[fd].fcbstate) my_save(fd);

  openfilelist[fd].topenfile = 0;
  return;
}

void my_cd(char *dirname) {
  int fd = my_open(dirname);
  if (!check_fd(fd)) return;
  if (openfilelist[fd].open_fcb.attribute) {
    my_close(fd);
    printf("%s is a file, please use open command\n", openfilelist[fd].dir);
    return;
  }

  // 得到的fd是文件夹的话，就把原来的目录关了,把现在的目录设为当前目录
  my_close(curdirid);
  curdirid = fd;
}