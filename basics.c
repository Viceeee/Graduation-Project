// basics
// 根据盘块号和偏移量，直接从FAT上读取指定长度的信息
int fat_read(unsigned short id, unsigned char *text, int offset, int len);
// 读取某个已打开文件的指定长度信息
int do_read(int fd, unsigned char *text, int len);
// 根据盘块号和偏移量，直接从FAT上写入指定长度的信息
int fat_write(unsigned short id, unsigned char *text, int blockoffset, int len);
// 向某个已打开文件写入指定长度信息
int do_write(int fd, unsigned char *text, int len);
// 从一个已打开目录文件找到对应名称的文件夹fcb，用于一些不断递归打开文件夹的函数中
int getFcb(fcb *fcbp, int *dirno, int *diroff, int fd, const char *dir);
// 在一个已打开目录文件下打开某个文件
int getOpenlist(int fd, const char *org_dir);
// 打开文件
int my_open(char *filename);
// basics
int fat_read(unsigned short id, unsigned char *text, int offset, int len)
{
    int ret = 0;
    unsigned char *buf = (unsigned char *)malloc(BLOCKSIZE);

    int count = 0;
    while (len)
    {
        memcpy(buf, blockaddr[id], BLOCKSIZE);
        count = min(len, 1024 - offset);
        memcpy(text + ret, buf + offset, count);
        len -= count;
        ret += count;
        offset = 0;
        id = fat1[id].id;
    }

    free(buf);
    return ret;
}

int do_read(int fd, unsigned char *text, int len)
{
    int blockorder = openfilelist[fd].count >> 10;
    int blockoffset = openfilelist[fd].count % 1024;
    unsigned short id = openfilelist[fd].open_fcb.first;
    while (blockorder)
    {
        --blockorder;
        id = fat1[id].id;
    }

    int ret = fat_read(id, text, blockoffset, len);

    return ret;
}

int fat_write(unsigned short id, unsigned char *text, int blockoffset, int len)
{
    int ret = 0;
    char *buf = (char *)malloc(1024);
    if (buf == NULL)
    {
        SAYERROR;
        printf("fat_write: malloc error\n");
        return -1;
    }

    // 写之前先把磁盘长度扩充到所需大小
    int tlen = len;
    int toffset = blockoffset;
    unsigned short tid = id;
    while (tlen)
    {
        if (tlen <= 1024 - toffset)
            break;
        tlen -= (1024 - toffset);
        toffset = 0;
        id = getNextFat(id);
        if (id == END)
        {
            SAYERROR;
            printf("fat_write: no next fat\n");
            return -1;
        }
    }

    int count = 0;
    while (len)
    {
        memcpy(buf, blockaddr[id], BLOCKSIZE);
        count = min(len, 1024 - blockoffset);
        memcpy(buf + blockoffset, text + ret, count);
        memcpy(blockaddr[id], buf, BLOCKSIZE);
        len -= count;
        ret += count;
        blockoffset = 0;
        id = fat1[id].id;
    }

    free(buf);
    return ret;
}

int do_write(int fd, unsigned char *text, int len)
{
    fcb *fcbp = &openfilelist[fd].open_fcb;

    int blockorder = openfilelist[fd].count >> 10;
    int blockoffset = openfilelist[fd].count % 1024;
    unsigned short id = openfilelist[fd].open_fcb.first;
    while (blockorder)
    {
        --blockorder;
        id = fat1[id].id;
    }

    int ret = fat_write(id, text, blockoffset, len);

    fcbp->length += ret;
    openfilelist[fd].fcbstate = 1;
    // 如果文件夹被写了，那么其'.'也要被写进去
    // 其子文件夹的'..'也要被更新
    if (!fcbp->attribute)
    {
        fcb tmp;
        memcpy(&tmp, fcbp, sizeof(fcb));
        strcpy(tmp.filename, ".");
        memcpy(blockaddr[fcbp->first], &tmp, sizeof(fcb));

        // 如果是根目录的话，".."也要被修改
        strcpy(tmp.filename, "..");
        if (fcbp->first == 5)
        {
            memcpy(blockaddr[fcbp->first] + sizeof(fcb), &tmp, sizeof(fcb));
        }

        // 从磁盘中读出当前目录的信息
        unsigned char buf[SIZE];
        int read_size = read_ls(fd, buf, fcbp->length);
        if (read_size == -1)
        {
            SAYERROR;
            printf("do_write: read_ls error\n");
            return 0;
        }
        fcb dirfcb;
        for (int i = 2 * sizeof(fcb); i < read_size; i += sizeof(fcb))
        {
            memcpy(&dirfcb, buf + i, sizeof(fcb));
            if (dirfcb.free || dirfcb.attribute)
                continue;
            memcpy(blockaddr[dirfcb.first] + sizeof(fcb), &tmp, sizeof(fcb));
        }
    }

    return ret;
}

int getFcb(fcb *fcbp, int *dirno, int *diroff, int fd, const char *dir)
{
    if (fd == -1)
    {
        memcpy(fcbp, blockaddr[5], sizeof(fcb));
        *dirno = 5;
        *diroff = 0;
        return 1;
    }

    useropen *file = &openfilelist[fd];

    // 从磁盘中读出当前目录的信息
    unsigned char *buf = (unsigned char *)malloc(SIZE);
    int read_size = read_ls(fd, buf, file->open_fcb.length);
    if (read_size == -1)
    {
        SAYERROR;
        printf("getFcb: read_ls error\n");
        return -1;
    }
    fcb dirfcb;
    int flag = -1;
    for (int i = 0; i < read_size; i += sizeof(fcb))
    {
        memcpy(&dirfcb, buf + i, sizeof(fcb));
        if (dirfcb.free)
            continue;
        if (!strcmp(dirfcb.filename, dir))
        {
            flag = i;
            break;
        }
    }

    free(buf);

    // 没有找到需要的文件
    if (flag == -1)
        return -1;

    // 找到的话就开始计算相关信息，改变对应打开文件项的值
    getPos(dirno, diroff, file->open_fcb.first, flag);
    memcpy(fcbp, &dirfcb, sizeof(fcb));

    return 1;
}

int getOpenlist(int fd, const char *org_dir)
{
    // 把路径名处理成绝对路径
    char dir[DIRLEN];
    if (fd == -1)
    {
        strcpy(dir, "~/");
    }
    else
    {
        strcpy(dir, openfilelist[fd].dir);
        strcat(dir, org_dir);
    }

    // 如果有打开的目录和想打开的目录重名，必须把原目录的内容写回磁盘
    for (int i = 0; i < MAXOPENFILE; ++i)
        if (i != fd)
        {
            if (openfilelist[i].topenfile && !strcmp(openfilelist[i].dir, dir))
            {
                my_save(i);
            }
        }

    int fileid = getFreeOpenlist();
    if (fileid == -1)
    {
        SAYERROR;
        printf("getOpenlist: openlist is full\n");
        return -1;
    }

    fcb dirfcb;
    useropen *file = &openfilelist[fileid];
    int ret;
    if (fd == -1)
    {
        ret = getFcb(&file->open_fcb, &file->dirno, &file->diroff, -1, ".");
    }
    else
    {
        ret = getFcb(&file->open_fcb, &file->dirno, &file->diroff, fd, org_dir);
    }
    strcpy(file->dir, dir);
    file->fcbstate = 0;
    file->topenfile = 1;

    //如果打开的是一个文件夹，就在路径后面加上'/'
    if (!file->open_fcb.attribute)
    {
        int len = strlen(file->dir);
        if (file->dir[len - 1] != '/')
            strcat(file->dir, "/");
    }

    if (ret == -1)
    {
        file->topenfile = 0;
        return -1;
    }
    return fileid;
}

int my_open(char *filename)
{
    char dirs[DIRLEN][DIRLEN];
    int count = spiltDir(dirs, filename);

    char realdirs[DIRLEN][DIRLEN];
    int tot = 0;
    for (int i = 1; i < count; ++i)
    {
        if (!strcmp(dirs[i], "."))
            continue;
        if (!strcmp(dirs[i], ".."))
        {
            if (tot)
                --tot;
            continue;
        }
        strcpy(realdirs[tot++], dirs[i]);
    }

    // 生成根目录的副本
    int fd = getOpenlist(-1, "");

    // 利用当前目录的副本不断找到下一个目录
    int flag = 0;
    for (int i = 0; i < tot; ++i)
    {
        int newfd = getOpenlist(fd, realdirs[i]);
        if (newfd == -1)
        {
            flag = 1;
            break;
        }
        my_close(fd);
        fd = newfd;
    }
    if (flag)
    {
        printf("my_open: %s no such file or directory\n", filename);
        openfilelist[fd].topenfile = 0;
        return -1;
    }

    if (openfilelist[fd].open_fcb.attribute)
        openfilelist[fd].count = 0;
    else
        openfilelist[fd].count = openfilelist[fd].open_fcb.length;
    return fd;
}