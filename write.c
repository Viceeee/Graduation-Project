

// write
// 把键盘输入的信息写入一个打开文件
int my_write(int fd);

// write
int my_write(int fd)
{
    if (!(0 <= fd && fd < MAXOPENFILE) || !openfilelist[fd].topenfile ||
        !openfilelist[fd].open_fcb.attribute)
    {
        printf("my_write: fd invaild\n");
        return -1;
    }

    useropen *file = &openfilelist[fd];
    printf("please tell me which write style do you prefer?\n");
    printf("  a : append write\n");
    printf("  w : truncate write\n");
    printf("  o : overwrite write\n");
    char op[5];
    scanf("%s", op);
    if (op[0] == 'a')
    {
        file->count = file->open_fcb.length;
    }
    else if (op[0] == 'w')
    {
        file->count = 0;
        file->open_fcb.length = 0;
        fatFree(fat1[file->open_fcb.first].id);
    }
    else if (op[0] != 'o')
    {
        printf("my_write: invaild write style!\n");
        return -1;
    }

    int ret = 0;
    int tmp;
    while (gets(str))
    {
        int len = strlen(str);
        str[len] = '\n';
        tmp = do_write(fd, (unsigned char *)str, len + 1);
        if (tmp == -1)
        {
            SAYERROR;
            printf("my_write: do_write error\n");
            return -1;
        }
        file->count += tmp;
        ret += tmp;
    }
    return ret;
}
