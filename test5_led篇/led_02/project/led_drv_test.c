
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * ./led_drv_test01 /dev/db_led0 on/off
 */
int main(int argc, char **argv)
{
	int fd;
	char send_buf;
	int read_buf = -1;
	if(argc != 3)
	{
		printf("%s <dev> <on/off>\n",argv[0]);
		printf("	eg:%s /dev/db_led0 on\n",argv[0]);
		printf("	eg:%s /dev/db_led0 off\n",argv[0]);
		return -1;
	}
	fd = open(argv[1],O_RDWR);
	if(fd < 0)
	{
		printf("%s open error\n",argv[1]);
		return -1;
	}
	if(strcmp(argv[2],"on") == 0)
	{
		send_buf = '1';
		write(fd,&send_buf,1);
		read(fd,&read_buf,sizeof(read_buf));
		printf("read_buf = %d\n",read_buf);
	}
	else   
	{
		send_buf = '0';
		write(fd,&send_buf,1);
		read(fd,&read_buf,sizeof(read_buf));
		printf("read_buf = %d\n",read_buf);
	}
	close(fd);
	return 0;
}


