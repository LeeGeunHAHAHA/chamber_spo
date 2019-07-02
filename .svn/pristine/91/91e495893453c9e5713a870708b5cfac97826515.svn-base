#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "powerctl.h"

#define CMD_POWER_ON	'1'
#define CMD_POWER_OFF	'0'


typedef struct termios termios_t;

termios_t g_tio;
termios_t g_stdio;
termios_t g_old_stdio;

int g_powerctl_fd;

extern int errno;

int powerctl_init(char *dev)
{
	int tty_fd;
    termios_t   *pst_tio;    

    pst_tio = &g_tio;
    
	tty_fd = open(dev, O_RDWR | O_NONBLOCK);      

	if (tty_fd < 0) {
			perror("TTY open error: ");

			return -1;
	}

	memset(pst_tio, 0, sizeof(termios_t));
	pst_tio->c_iflag = 0;
	pst_tio->c_oflag = 0;
	pst_tio->c_cflag = CS8 | CREAD | CLOCAL;           // 8n1, see termios.h for more informag_tion
	pst_tio->c_lflag = 0;
	pst_tio->c_cc[VMIN] = 1;
	pst_tio->c_cc[VTIME] = 5;    

	cfsetospeed(pst_tio, B115200);            // 115200 baud
	cfsetispeed(pst_tio, B115200);            // 115200 baud

	tcsetattr(tty_fd, TCSANOW, pst_tio);

	printf("Opened %s\n", dev);

	return tty_fd;
}

int __powerctl_send_cmd(int tty_fd, char *cmd, int len)
{		
	int i;
    int ret;

    printf("%s: len: %d\n", __func__, len);
    
    for (i = 0; i < len ; i++) {
        ret = write(tty_fd, cmd + i, 1);

        
    	if (ret < 0) {
    			perror("Command write error: ");

    			return -1;
    	}
    }

    if (len == 1) {
        printf("Sent CMD %c\n", *cmd);
    } else {
        printf("Sent CMD %s\n", cmd);
    }

	return i;
}

int __powerctl_recv_rsp(int tty_fd)
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

int powerctl_power_on(int tty_fd)
{
    char cmd = CMD_POWER_ON;
    
    __powerctl_send_cmd(tty_fd, &cmd, sizeof(cmd));

    __powerctl_recv_rsp(tty_fd);

    return 0;
}


int powerctl_power_off(int tty_fd)
{
    char cmd = CMD_POWER_OFF;
    
    __powerctl_send_cmd(tty_fd, &cmd, sizeof(cmd));

    __powerctl_recv_rsp(tty_fd);

    return 0;
}


int powerctl_exit(int tty_fd)
{
    int ret;

    ret = close(tty_fd);

    if (ret < 0) {
        perror("%s close error: ");

        return -1;
    }

    return 0;
}

#if 0
int main(int argc,char** argv)
{
	int tty_fd;

	unsigned char c='D';
	tcgetattr(STDOUT_FILENO,&g_old_stdio);

	if (argc != 2) {
		printf("Usage: %s /dev/ttyUSB0 (for example)\n",argv[0]);

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

	send_cmd(tty_fd, CMD_POWER_ON);
	recv_rsp(tty_fd);
	sleep(5);

	send_cmd(tty_fd, CMD_POWER_OFF);
	recv_rsp(tty_fd);
	sleep(5);

	send_cmd(tty_fd, CMD_POWER_ON);
	recv_rsp(tty_fd);

out:
	close(tty_fd);
//	tcsetattr(STDOUT_FILENO,TCSANOW,&g_old_stdio);

	return EXIT_SUCCESS;
}
#endif
