

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <linux/input.h>

int main(void)
{
    int buttons_fd;
    int key_value,i=0,count;
    struct input_event ev_key;
    
    buttons_fd = open("/dev/input/event0", O_RDWR);
    if (buttons_fd < 0) 
    {
        perror("open device buttons");
        exit(1);
    }
    for (;;) 
    {
        count = read(buttons_fd,&ev_key,sizeof(struct input_event));
        for(i=0; i<(int)count/sizeof(struct input_event); i++)
            if(EV_KEY==ev_key.type)
                printf("type:%d,code:%d,value:%d\n", ev_key.type,ev_key.code-1,ev_key.value);
        if(EV_SYN==ev_key.type)
        printf("syn event\n\n");
    }
 
    close(buttons_fd);
    return 0;
}
