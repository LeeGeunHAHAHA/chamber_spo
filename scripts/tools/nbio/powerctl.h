/**
 * @file powerctl.h
 * @date 2018. 09. 27.
 * @author paul
 *
 */
 
#ifndef __NBIO_POWERCTL_H__
#define __NBIO_POWERCTL_H__



extern int g_powerctl_fd;

int powerctl_init(char* dev);
//int __powerctl_send_cmd(int tty_fd, char *cmd, int len);
//int __powerctl_recv_rsp(int tty_fd);
int powerctl_exit(int tty_fd);

int powerctl_power_on(int tty_fd);
int powerctl_power_off(int tty_fd);


#endif /* __NBIO_POWERCTL_H__ */

