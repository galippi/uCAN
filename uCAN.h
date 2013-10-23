#ifndef _UCAN_H_
#define _UCAN_H_

#include "uCAN_conf.h"

#if (defined(_MSC_VER)) && (1400 == _MSC_VER)
  /* MSVS 2005 does not have sdtint.h */
  typedef signed char        int8_t;
  typedef signed short       int16_t;
  typedef signed int         int32_t;
  typedef signed long long   int64_t;
  typedef unsigned char      uint8_t;
  typedef unsigned short     uint16_t;
  typedef unsigned int       uint32_t;
  typedef unsigned long long uint64_t;
#else
#include "stdint.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint32_t uCAN_GetInterfaceVersion(void);
typedef uint32_t (*t_uCAN_GetInterfaceVersion)(void);
#define UCAN_INTERFACE_VERSION_MAJOR_MASK 0xFF00
#define UCAN_INTERFACE_VERSION_MINOR_MASK 0x00FF
#define UCAN_INTERFACE_VERSION_MAJOR_GET(ver) (((ver) & UCAN_INTERFACE_VERSION_MAJOR_MASK) >> 8)
#define UCAN_INTERFACE_VERSION_MINOR_GET(ver)  ((ver) & UCAN_INTERFACE_VERSION_MINOR_MASK)
#define UCAN_INTERFACE_VERSION_SET(major, minor)  (((major) << 8) | (minor))

int uCAN_socket(int domain, int type, int protocol);
typedef int (*t_uCAN_socket)(int domain, int type, int protocol);
int uCAN_bind(int fd, const struct sockaddr_can *addr, unsigned int len);
typedef int (*t_uCAN_bind)(int fd, const struct sockaddr_can *addr, unsigned int len);
int uCAN_ioctl(int fd, unsigned long int request, struct ifreq* ifr);
typedef int (*t_uCAN_ioctl)(int fd, unsigned long int request, struct ifreq* ifr);
int uCAN_setsockopt(int fd, int level, int optname,
                      const void *optval, socklen_t optlen);
typedef int (*t_uCAN_setsockopt)(int fd, int level, int optname, const void *optval, socklen_t optlen);
int uCAN_fcntl(int fd, int cmd, int flag, int value);
typedef int (*t_uCAN_fcntl)(int fd, int cmd, int flag, int value);
int uCAN_read(int fd, void *buf, unsigned int n);
typedef int (*t_uCAN_read)(int fd, void *buf, unsigned int n);
int uCAN_write(int fd, const void *buf, unsigned int n);
typedef int (*t_uCAN_write)(int fd, const void *buf, unsigned int n);
int uCAN_close(int fd);
typedef int (*t_uCAN_close)(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _UCAN_H_ */
