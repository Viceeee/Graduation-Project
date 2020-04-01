



// creat
// 格式化
void my_format();
// 在指定目录下创建一个文件或文件夹
int my_touch(char *filename, int attribute, int *rpafd);
// 调用touch创建出一个文件
int my_create(char *filename);
// 调用touch创建出一个文件夹
void my_mkdir(char *dirname);

void my_format() {
  strcpy(initblock.information, "10101010");
  initblock.root = 5;
  initblock.startblock = blockaddr[5];

  for (int i = 0; i < 5; ++i) fat1[i].id = END;
  for (int i = 5; i < BLOCKNUM; ++i) fat1[i].id = FREE;
  for (int i = 0; i < BLOCKNUM; ++i) fat2[i].id = fat1[i].id;

  fat1[5].id = END;
  fcb root;
  fcb_init(&root, ".", 5, 0);
  memcpy(blockaddr[5], &root, sizeof(fcb));

#ifdef DEBUG_INFO
  printf("my_format %s\n", root.filename);
#endif // DEBUG_INFO

  strcpy(root.filename, "..");
  memcpy(blockaddr[5] + sizeof(fcb), &root, sizeof(fcb));

#ifdef DEBUG_INFO
  printf("my_format %s\n", root.filename);
#endif // DEBUG_INFO

  printf("初始化完成\n");
}

int my_touch(char *filename, int attribute, int *rpafd) {
  // 先打开file的上级目录，如果上级目录不存在就报错（至少自己电脑上的Ubuntu是这个逻辑）
  char split_dir[2][DIRLEN];
  splitLastDir(filename, split_dir);

  int pafd = my_open(split_dir[0]);
  if (!(0 <= pafd && pafd < MAXOPENFILE)) {
    SAYERROR;
    printf("my_creat: my_open error\n");
    return -1;
  }

  // 从磁盘中读出当前目录的信息，进行检查
  unsigned char *buf = (unsigned char*)malloc(SIZE);
  int read_size = read_ls(pafd, buf, openfilelist[pafd].open_fcb.length);
  if (read_size == -1) {
    SAYERROR;
    printf("my_touch: read_ls error\n");
    return -1;
  }
  fcb dirfcb;
  for (int i = 0; i < read_size; i += sizeof(fcb)) {
    memcpy(&dirfcb, buf + i, sizeof(fcb));
    if (dirfcb.free) continue;
    if (!strcmp(dirfcb.filename, split_dir[1])) {
      printf("%s is already exit\n", split_dir[1]);
      return -1;
    }
  }

  // 利用空闲磁盘块创建文件
  int fatid = getFreeFatid();
  if (fatid == -1) {
    SAYERROR;
    printf("my_touch: no free fat\n");
    return -1;
  }
  fat1[fatid].id = END;
  fcb_init(&dirfcb, split_dir[1], fatid, attribute);

  // 写入父亲目录内存
  memcpy(buf, &dirfcb, sizeof(fcb));
  int write_size = do_write(pafd, buf, sizeof(fcb));
  if (write_size == -1) {
    SAYERROR;
    printf("my_touch: do_write error\n");
    return -1;
  }
  openfilelist[pafd].count += write_size;

  // 创建自己的打开文件项
  int fd = getFreeOpenlist();
  if (!(0 <= fd && fd < MAXOPENFILE)) {
    SAYERROR;
    printf("my_touch: no free fat\n");
    return -1;
  }
  getPos(&openfilelist[fd].dirno, &openfilelist[fd].diroff, openfilelist[pafd].open_fcb.first, openfilelist[pafd].count - write_size);
  memcpy(&openfilelist[fd].open_fcb, &dirfcb, sizeof(fcb));
  if (attribute) openfilelist[fd].count = 0;
  else openfilelist[fd].count = openfilelist[fd].open_fcb.length;
  openfilelist[fd].fcbstate = 1;
  openfilelist[fd].topenfile = 1;
  strcpy(openfilelist[fd].dir, openfilelist[pafd].dir);
  strcat(openfilelist[fd].dir, split_dir[1]);

  free(buf);
  *rpafd = pafd;
  return fd;
}

int my_create(char *filename) {
  int pafd;
  int fd = my_touch(filename, 1, &pafd);
  if (!check_fd(fd)) return -1;
  my_close(pafd);
  return fd;
}

void my_mkdir(char *dirname) {
  int pafd;
  int fd = my_touch(dirname, 0, &pafd);
  if (!check_fd(fd)) return;
  unsigned char *buf = (unsigned char*)malloc(SIZE);

  // 把"."和".."装入自己的磁盘
  fcb dirfcb;
  memcpy(&dirfcb, &openfilelist[fd].open_fcb, sizeof(fcb));
  int fatid = dirfcb.first;
  strcpy(dirfcb.filename, ".");
  memcpy(blockaddr[fatid], &dirfcb, sizeof(fcb));
  memcpy(&dirfcb, &openfilelist[pafd].open_fcb, sizeof(fcb));
  strcpy(dirfcb.filename, "..");
  memcpy(blockaddr[fatid] + sizeof(fcb), &dirfcb, sizeof(fcb));

  my_close(pafd);
  my_close(fd);
  free(buf);
}
