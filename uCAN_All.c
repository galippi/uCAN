#include <dlfcn.h>
#include <string.h>
#include <stdio.h> /* only for dprintf */

#include "uCAN.h"

#ifndef TEST_UCAN
//#define TEST_UCAN 1 /* demo application is enabled */
#define TEST_UCAN 0 /* demo application is disabled */
#endif

typedef struct {
  int error;
  t_uCAN_GetInterfaceVersion puCAN_GetInterfaceVersion;
  t_uCAN_socket puCAN_socket;
  t_uCAN_bind puCAN_bind;
  t_uCAN_ioctl puCAN_ioctl;
  t_uCAN_setsockopt puCAN_setsockopt;
  t_uCAN_fcntl puCAN_fcntl;
  t_uCAN_read puCAN_read;
  t_uCAN_write puCAN_write;
  t_uCAN_close puCAN_close;
}t_CANDriver_EntryPoints;

static const char *DLL_Name[] = 
{
  "uCAN_Vector.dll",
  "uCAN_WeCAN.dll",
  "uCAN_PCAN.dll",
};

#define MAX_CAN_DRIVER (sizeof(DLL_Name)/sizeof(DLL_Name[0]))
static t_CANDriver_EntryPoints CANDriver_EntryPoints[MAX_CAN_DRIVER];

typedef struct
{
  int DLL_idx;
  int fd;
}t_CAN_Channel;
#define MAX_CAN_CHANNEL 32
static t_CAN_Channel CAN_Channels[MAX_CAN_CHANNEL];

uint32_t uCAN_GetInterfaceVersion(void)
{
  return UCAN_INTERFACE_VERSION_SET(1, 1);
}

int uCAN_socket(int domain, int type, int protocol)
{
  if (!(domain == PF_CAN && type == SOCK_RAW && protocol == CAN_RAW))
  {
    dprintf(1, "uCAN_socket error: not supported socket type (%d, %d, %d)!\n", domain, type, protocol);
    return -1;
  }
  static int CAN_DLL_LoadedLast = -1;
  while (1)
  {
    static int next_fds_cnt = 0;
    if (next_fds_cnt >= MAX_CAN_CHANNEL)
    { /* no more channel is supported */
      dprintf(1, "Error: no more channel is supported (%d)!\n", next_fds_cnt);
      return -1;
    }
    if ((CAN_DLL_LoadedLast >= 0) && (CAN_DLL_LoadedLast < (int)MAX_CAN_DRIVER) && (CANDriver_EntryPoints[CAN_DLL_LoadedLast].error == 0))
    {
      printf("Requesting driver %d.\n", CAN_DLL_LoadedLast);
      CAN_Channels[next_fds_cnt].fd = CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_socket(domain, type, protocol);
      if (CAN_Channels[next_fds_cnt].fd >= 0)
      {
        dprintf(9, "Info: channel %d is openned.\n", next_fds_cnt);
        CAN_Channels[next_fds_cnt].DLL_idx = CAN_DLL_LoadedLast;
        next_fds_cnt++;
        return next_fds_cnt - 1;
      }
    }
    CAN_DLL_LoadedLast++;
    if (CAN_DLL_LoadedLast >= (int)MAX_CAN_DRIVER)
    { /* no more driver is available */
      dprintf(9, "Info: no more driver is available!\n");
      CAN_DLL_LoadedLast = MAX_CAN_DRIVER;
      return -1;
    }
    int *dlh = dlopen(DLL_Name[CAN_DLL_LoadedLast], 0);
    if (dlh != NULL)
    {
      dprintf(9, "Info: driver %s is loaded.\n", DLL_Name[CAN_DLL_LoadedLast]);
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_GetInterfaceVersion = (t_uCAN_GetInterfaceVersion)dlsym(dlh, "uCAN_GetInterfaceVersion");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_socket              = (t_uCAN_socket)             dlsym(dlh, "uCAN_socket");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_bind                = (t_uCAN_bind)               dlsym(dlh, "uCAN_bind");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_ioctl               = (t_uCAN_ioctl)              dlsym(dlh, "uCAN_ioctl");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_setsockopt          = (t_uCAN_setsockopt)         dlsym(dlh, "uCAN_setsockopt");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_fcntl               = (t_uCAN_fcntl)              dlsym(dlh, "uCAN_fcntl");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_read                = (t_uCAN_read)               dlsym(dlh, "uCAN_read");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_write               = (t_uCAN_write)              dlsym(dlh, "uCAN_write");
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_close               = (t_uCAN_close)              dlsym(dlh, "uCAN_close");
      if (
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_GetInterfaceVersion == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_socket == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_bind == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_ioctl == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_setsockopt == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_fcntl == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_read == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_write == NULL) ||
          (CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_close == NULL) ||
          0
         )
      { /* one of the entry point is not found */
        CANDriver_EntryPoints[CAN_DLL_LoadedLast].error = 2;
        dprintf(1, "Error: one of the entry point is not found!\n");
      }else
      {
        uint32_t version = CANDriver_EntryPoints[CAN_DLL_LoadedLast].puCAN_GetInterfaceVersion();
        if ((UCAN_INTERFACE_VERSION_MAJOR_GET(version) != 1) || (UCAN_INTERFACE_VERSION_MINOR_GET(version) < 1))
        { /* not supported interface version */
          CANDriver_EntryPoints[CAN_DLL_LoadedLast].error = 3;
          dprintf(1, "Error: not supported interface version (%X - %d - %s)!\n", version, CAN_DLL_LoadedLast, DLL_Name[CAN_DLL_LoadedLast]);
        }else
        { /* driver is loaded successfully */
          CANDriver_EntryPoints[CAN_DLL_LoadedLast].error = 0;
          dprintf(9, "Info: driver %s is loaded successfully.\n", DLL_Name[CAN_DLL_LoadedLast]);
        }
      }
    }else
    { /* driver is not found or it's not valid */
      CANDriver_EntryPoints[CAN_DLL_LoadedLast].error = 1;
      dprintf(1, "Error: unable to load driver %s!\n", DLL_Name[CAN_DLL_LoadedLast]);
    }
  }
}

int uCAN_bind(int fd, const struct sockaddr_can *addr, unsigned int len)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_bind(CAN_Channels[fd].fd, addr, len);
}

int uCAN_ioctl(int fd, unsigned long int request, struct ifreq* ifr)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_ioctl(CAN_Channels[fd].fd, request, ifr);
}

int uCAN_setsockopt(int fd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_setsockopt(CAN_Channels[fd].fd, level, optname, optval, optlen);
}

int uCAN_fcntl(int fd, int cmd, int flag, int value)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_fcntl(CAN_Channels[fd].fd, cmd, flag, value);
}

int uCAN_read(int fd, void *buf, unsigned int n)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_read(CAN_Channels[fd].fd, buf, n);
}

int uCAN_write(int fd, const void *buf, unsigned int n)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_write(CAN_Channels[fd].fd, buf, n);
}

int uCAN_close(int fd)
{
  return CANDriver_EntryPoints[CAN_Channels[fd].DLL_idx].puCAN_close(CAN_Channels[fd].fd);
}

#if defined(TEST_UCAN) && (TEST_UCAN != 0)
int main(int argc, const char **argv)
{
  (void)argc;
  (void)argv;
  {
    uint32_t version = uCAN_GetInterfaceVersion();
    if ((UCAN_INTERFACE_VERSION_MAJOR_GET(version) != 1) || (UCAN_INTERFACE_VERSION_MINOR_GET(version) < 1))
    { /* not supported interface version */
      printf("Error: not supported interface version (%X)!\n", version);
      return 2;
    }
  }
  #define MAX_CAN 8
  int skt[MAX_CAN];
  int CAN_ChNum = 0;
  int i;
  for (i = 0; i < MAX_CAN; i++)
  {
    // Create a CAN socket
    skt[CAN_ChNum] = uCAN_socket( PF_CAN, SOCK_RAW, CAN_RAW );
    if (skt[CAN_ChNum] >= 0)
    { /* interface is available */
      // Select that CAN interface, and bind the socket to it.
      struct sockaddr_can addr;
      addr.can_family = AF_CAN; 
      uCAN_bind( skt[CAN_ChNum], &addr, sizeof(addr) );
      CAN_ChNum++;
    }else
    {
      break;
    }
  }
  if (CAN_ChNum <= 0)
  { /* no interface is available */
    printf("No CAN interface is connected!\n");
    return 1;
  }
  printf("%d channel is found!\n", CAN_ChNum);
  uint32_t TimeOfs[MAX_CAN];
  memset(&TimeOfs[0], 0, sizeof(TimeOfs));
  for (i = 0; i < CAN_ChNum; i++)
  { /* calculating initial time-stamp */
    struct can_frame frame;
    int status;
    while ((status = uCAN_read(skt[i], &frame, sizeof(frame))) != UCAN_READ_EMPTY)
    {
      if (status == UCAN_READ_RX_FRAME)
      {
        TimeOfs[i] = frame.can_time_stamp;
      }
    }
  }
  uint32_t TimeStampLast = 0;
  while(1)
  {
    for (i = 0; i < CAN_ChNum; i++)
    {
      struct can_frame frame;
      if (uCAN_read(skt[i], &frame, sizeof(frame)) >= sizeof(frame))
      {
        char data[1024];
        {
          unsigned char idx;
          char *ptr = &data[0];
          for (idx = 0; idx < frame.can_dlc; idx++)
          {
            if (ptr != &data[0])
            {
              *ptr++ = ' ';
            }
            sprintf(ptr, "%02X", (frame.data[idx]) & 0xFF);
            ptr += 2;
          }
        }
        char id_str[12];
        sprintf(id_str, "%X", frame.can_id & CAN_ID_MASK);
        if (IS_CAN_ID_EXT(frame.can_id))
        { /* extended frame */
          strcat(id_str, "x");
        }
        if (TimeOfs[i] == 0)
        {
          TimeOfs[i] = frame.can_time_stamp - TimeStampLast - 1;
        }
        uint32_t TimeStamp = frame.can_time_stamp - TimeOfs[i];
        #define TIMESTAMP_MONOTONE 0
        if ((TIMESTAMP_MONOTONE) && (TimeStampLast > TimeStamp))
        {
          TimeOfs[i] = TimeOfs[i] - (TimeStampLast - TimeStamp) - 1;
          TimeStamp = frame.can_time_stamp - TimeOfs[i];
        }
        TimeStampLast = TimeStamp;
        printf("%11.6lf %d  %-9s       %2s   d %d %s\n", (double)TimeStamp/1000000.0, skt[i], id_str, "Rx", frame.can_dlc, data);
      }
    }
  }
  return 0;
}

#include <stdarg.h>

int dprintf(int debug, const char *pszFormat, ...)
{
  {
    va_list argptr;
    FILE *debug_out = NULL;
    if (!debug_out)
      debug_out = stdout;
    va_start(argptr, pszFormat);
    vfprintf(debug_out,pszFormat,argptr);
    va_end(argptr);
  }
}

#endif
