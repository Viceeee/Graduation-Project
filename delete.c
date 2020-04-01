

// delete
// 把一个指定目录的fcb的free置为1
void my_rmdir(char *dirname);
// 把一个指定文件的fcb的free置为1
void my_rm(char *filename);

void my_rmdir(char *dirname)
{
    int fd = my_open(dirname);
    if (0 <= fd && fd < MAXOPENFILE)
    {
        if (openfilelist[fd].open_fcb.attribute)
        {
            printf("my_rmdir: %s is a file, please use rm command\n", dirname);
            my_close(fd);
            return;
        }
        if (!strcmp(openfilelist[fd].dir, openfilelist[curdirid].dir))
        {
            printf("my_rmdir: can not remove the current directory!\n");
            my_close(fd);
            return;
        }

        // 从磁盘中读出当前目录的信息
        int cnt = 0;
        unsigned char *buf = (unsigned char *)malloc(SIZE);
        int read_size = read_ls(fd, buf, openfilelist[fd].open_fcb.length);
        if (read_size == -1)
        {
            my_close(fd);
            free(buf);
            SAYERROR;
            printf("my_rmdir: read_ls error\n");
            return;
        }
        fcb dirfcb;
        int flag = -1;
        for (int i = 0; i < read_size; i += sizeof(fcb))
        {
            memcpy(&dirfcb, buf + i, sizeof(fcb));
            if (dirfcb.free)
                continue;
            ++cnt;
        }

        if (cnt > 2)
        {
            my_close(fd);
            printf("my_rmdir: %s is not empty\n", dirname);
            return;
        }

        openfilelist[fd].open_fcb.free = 1;
        fatFree(openfilelist[fd].open_fcb.first);
        openfilelist[fd].fcbstate = 1;
        my_close(fd);
    }
}

void my_rm(char *filename)
{
    int fd = my_open(filename);
    if (0 <= fd && fd < MAXOPENFILE)
    {
        if (openfilelist[fd].open_fcb.attribute == 0)
        {
            printf("my_rm: %s is a directory, please use rmdir command\n", filename);
            my_close(fd);
            return;
        }

        openfilelist[fd].open_fcb.free = 1;
        fatFree(openfilelist[fd].open_fcb.first);
        openfilelist[fd].fcbstate = 1;
        my_close(fd);
    }
}