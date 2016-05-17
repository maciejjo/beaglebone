#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int fd;
	unsigned char val = 0x01;
	unsigned char out;
	char buf[4];

	fd = open("/dev/pcf8574_sample", O_RDWR);
	if(fd < 0)
	{
		printf("open error\n");
		return -1;
	}

	while(val)
	{
		out = ~val;
		sprintf(buf, "%X", out);
		printf("0x%s\n", buf);
		write(fd, buf, 8);
		sleep(2);
		val = val << 1;
	}
	printf("0xFF\n");
	write(fd, "FF", 8);
	close(fd);
	return 0;
}
