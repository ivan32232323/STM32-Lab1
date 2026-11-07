/**
 * syscalls.c — мінімальні системні виклики newlib.
 * _write() перенаправляє stdout (printf) у USART2 = ST-Link VCP.
 */
#include "main.h"
#include <errno.h>
#include <sys/stat.h>

int _write(int fd, char *p, int n)
{
  (void)fd;
  HAL_UART_Transmit(&huart2, (uint8_t *)p, (uint16_t)n, HAL_MAX_DELAY);
  return n;
}

int _read(int fd, char *p, int n)
{
  (void)fd;
  if (HAL_UART_Receive(&huart2, (uint8_t *)p, 1, HAL_MAX_DELAY) == HAL_OK) return 1;
  (void)n;
  return 0;
}

int _close(int fd)                  { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)                 { (void)fd; return 1; }
int _lseek(int fd, int p, int d)    { (void)fd; (void)p; (void)d; return 0; }
int _getpid(void)                   { return 1; }
int _kill(int pid, int sig)         { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _exit(int status)              { (void)status; while (1) { } }
