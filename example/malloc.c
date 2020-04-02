//this function returns a pointer to the allocated memory, or NULL if the request fails.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char *str;

    /* Initial memory allocation */
    str = (char *)malloc(15); //malloc(size)size是字符串的长度
    strcpy(str, "tutorialspoint");
    printf("String = %s,  Address = %u\n", str, str);

    /* Reallocating memory */
    str = (char *)realloc(str, 25);
    printf("String = %s,  Address = %u\n", str, str);

    free(str); //清理内存

    return (0);
}
