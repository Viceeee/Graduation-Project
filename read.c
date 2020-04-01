





// read
// 读取一个文件夹下的fcb信息
int read_ls(int fd, unsigned char *text, int len);
// 把一个文件夹下的fcb信息打印出来
void my_ls();
// 把一个打开文件的内容根据文件指针打印出来
int my_read(int fd);
// 重新从磁盘中读取一个打开文件的fcb内容
void my_reload(int fd);

// read
int read_ls(int fd, unsigned char *text, int len) {
  int tcount = openfilelist[fd].count;
  openfilelist[fd].count = 0;
  int ret = do_read(fd, text, len);
  openfilelist[fd].count = tcount;
  return ret;
}

void my_ls() {
  // 从磁盘中读出当前目录的信息
  unsigned char *buf = (unsigned char*)malloc(SIZE);
  int read_size = read_ls(curdirid, buf, openfilelist[curdirid].open_fcb.length);
  if (read_size == -1) {
    free(buf);
    SAYERROR;
    printf("my_ls: read_ls error\n");
    return;
  }
  fcb dirfcb;
  for (int i = 0; i < read_size; i += sizeof(fcb)) {
    memcpy(&dirfcb, buf + i, sizeof(fcb));
    if (dirfcb.free) continue;
    if (dirfcb.attribute) printf(" %s", dirfcb.filename);
    else printf(" " GRN "%s" RESET, dirfcb.filename);
  }
  printf("\n");
  free(buf);
}

int my_read(int fd) {
  if (!(0 <= fd && fd < MAXOPENFILE) || !openfilelist[fd].topenfile ||
    !openfilelist[fd].open_fcb.attribute) {
    printf("my_read: fd invaild\n");
    return -1;
  }

  unsigned char *buf = (unsigned char *)malloc(SIZE);
  int len = openfilelist[fd].open_fcb.length - openfilelist[fd].count;
  int ret = do_read(fd, buf, len);
  if (ret == -1) {
    free(buf);
    printf("my_read: do_read error\n");
    return -1;
  }
  buf[ret] = '\0';
  printf("%s\n", buf);
  return ret;
}

void my_reload(int fd) {
  if (!check_fd(fd)) return;
  fat_read(openfilelist[fd].dirno, (unsigned char*)&openfilelist[fd].open_fcb, openfilelist[fd].diroff, sizeof(fcb));
  return;
}
