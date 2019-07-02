#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>


#define CMD_POWER_ON	'1'
#define CMD_POWER_OFF	'0'

typedef struct termios termios_t;

termios_t g_tio;
termios_t g_stdio;
termios_t g_old_stdio;

extern int sys_nerr;
extern int errno;

int open_ttydev(char* dev)
{
	int tty_fd;

	tty_fd = open(dev, O_RDWR | O_NONBLOCK);      

	if (tty_fd < 0) {
			perror("TTY open error: ");

			return -1;
	}

	cfsetospeed(&g_tio,B115200);            // 115200 baud
	cfsetispeed(&g_tio,B115200);            // 115200 baud

	tcsetattr(tty_fd,TCSANOW,&g_tio);

	printf("Opened %s\n", dev);

	return tty_fd;

}

int send_cmd(int tty_fd, char cmd)
{		
	int send_len; 

	send_len = write(tty_fd, &cmd, 1);

	if (send_len < 0) {
			perror("Command write error: ");

			return -1;
	}

	printf("Sent CMD %c\n", cmd);

	return send_len;
}

int recv_rsp(int tty_fd)
{
	char cmd;
	int resp_len = 0;

	while (read(tty_fd, &cmd, 1) > 0) {
		// drop received messages
//		write(STDOUT_FILENO, &cmd, 1);
		resp_len++;
	}

	return resp_len;
}

int main(int argc,char** argv)
{
	int tty_fd;

//	unsigned char c='D';
	char cmd;
//	tcgetattr(STDOUT_FILENO,&g_old_stdio);

	if (argc != 3) {
		printf("Usage: %s devfile 1|0\n",argv[0]);
		printf("          devfile: /dev/ttyUSBn, 1: on, 0: off\n");

		return -1;
	}

	cmd = argv[2][0];
	if (cmd != '1' && cmd != '0') {
			printf("Invalid parameter (only 1 or 0 is valid)\n");

		return -1;
	}

	memset(&g_stdio,0,sizeof(g_stdio));
	g_stdio.c_iflag=0;
	g_stdio.c_oflag=0;
	g_stdio.c_cflag=0;
	g_stdio.c_lflag=0;
	g_stdio.c_cc[VMIN]=1;
	g_stdio.c_cc[VTIME]=0;
//	tcsetattr(STDOUT_FILENO,TCSANOW,&g_stdio);
//	tcsetattr(STDOUT_FILENO,TCSAFLUSH,&g_stdio);
//	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);       // make the reads non-blocking

	memset(&g_tio,0,sizeof(g_tio));
	g_tio.c_iflag=0;
	g_tio.c_oflag=0;
	g_tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more informag_tion
	g_tio.c_lflag=0;
	g_tio.c_cc[VMIN]=1;
	g_tio.c_cc[VTIME]=5;

	tty_fd = open_ttydev(argv[1]);

	if (tty_fd < 0) {
			return -1;
	}

	if (cmd == CMD_POWER_ON) {
		send_cmd(tty_fd, CMD_POWER_ON);
		recv_rsp(tty_fd);
	} else if (cmd == CMD_POWER_OFF) {
		send_cmd(tty_fd, CMD_POWER_OFF);
		recv_rsp(tty_fd);
	}

	close(tty_fd);
//	tcsetattr(STDOUT_FILENO,TCSANOW,&g_old_stdio);

	return EXIT_SUCCESS;
}
