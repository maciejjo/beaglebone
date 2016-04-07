#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
	int fd;
	char buf[16];
	fd = open("/dev/pcf8574_sample", O_RDWR);
	if(fd < 0)
	{
		printf("open error\n");
		return -1;
	}
	read(fd, buf, 16);
	printf("%s\n", buf);
	close(fd);
	return 0;
}
