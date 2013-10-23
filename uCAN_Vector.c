#include <dlfcn.h>
#include <string.h>
#include <stdio.h> /* only for dprintf */

#include "uCAN.h"

#ifndef TEST_UCAN
//#define TEST_UCAN 1 /* demo application is enabled */
#define TEST_UCAN 0 /* demo application is disabled */
#endif

#define POINTER_32
#include "vxlapi.h"

typedef XLstatus (__stdcall *fxlOpenDriver)(void);
typedef XLstatus (__stdcall *fxlClosePort)(XLportHandle);
typedef XLstatus (__stdcall *fxlGetDriverConfig)(XLdriverConfig *);
typedef XLstatus (__stdcall *fxlOpenPort)(XLportHandle *pPortHandle, const char *userName,
                                          XLaccess accessMask, XLaccess *pPermissionMask,
                                          unsigned int rxQueueSize, unsigned int xlInterfaceVersion, unsigned int busType);
typedef XLstatus (__stdcall *fxlCanSetChannelBitrate)(XLportHandle portHandle, XLaccess accessMask, unsigned long bitrate);
typedef XLstatus (__stdcall *fxlActivateChannel)(XLportHandle portHandle, XLaccess accessMask, unsigned int busType, unsigned int flags);
typedef XLstatus (__stdcall *fxlReceive)(XLportHandle portHandle, unsigned int *pEventCount, XLevent *pEvents);
typedef XLstatus (__stdcall *fxlTransmit)(XLportHandle portHandle, XLaccess accessMask, unsigned int *pEventCount, XLevent *pEvents);
typedef XLstringType (__stdcall *fxlGetErrorString)(XLstatus);

static const char *CAN_DriverDLL = "vxlapi.dll";

static int *dlh = NULL; /* pointer to DLL */
static fxlOpenDriver pxlOpenDriver = NULL;
static fxlClosePort pxlClosePort = NULL;
static fxlGetDriverConfig pxlGetDriverConfig = NULL;
static fxlOpenPort pxlOpenPort = NULL;
static fxlCanSetChannelBitrate pxlCanSetChannelBitrate = NULL;
static fxlActivateChannel pxlActivateChannel = NULL;
static fxlReceive pxlReceive = NULL;
static fxlTransmit pxlCanTransmit = NULL;
static fxlGetErrorString pxlGetErrorString = NULL;

#define MAX_CAN_CHANNEL 8
static XLportHandle g_xlPortHandle[MAX_CAN_CHANNEL];
static XLdriverConfig g_xlDrvConfig;

#define RX_QUEUE_SIZE      4096

uint32_t uCAN_GetInterfaceVersion(void)
{
  return UCAN_INTERFACE_VERSION_SET(1, 1);
}

int uCAN_socket(int domain, int type, int protocol)
{
  if(domain == PF_CAN && type == SOCK_RAW && protocol == CAN_RAW)
  {
    if (dlh == NULL)
    {
      dlh = dlopen(CAN_DriverDLL, 0);
      if (dlh == NULL)
      {
        dprintf(1, "dlopen(%s):0x%08X\n", CAN_DriverDLL, (unsigned)dlh);
        return -1;
      }
      pxlOpenDriver = (fxlOpenDriver)dlsym(dlh, "_xlOpenDriver@0");
      pxlClosePort = (fxlClosePort)dlsym(dlh, "_xlClosePort@4");
      pxlGetDriverConfig = (fxlGetDriverConfig)dlsym(dlh, "_xlGetDriverConfig@4");
      pxlOpenPort = (fxlOpenPort)dlsym(dlh, "_xlOpenPort@32");
      pxlCanSetChannelBitrate = (fxlCanSetChannelBitrate)dlsym(dlh, "_xlCanSetChannelBitrate@16");
      pxlActivateChannel = (fxlActivateChannel)dlsym(dlh, "_xlActivateChannel@20");
      pxlReceive = (fxlReceive)dlsym(dlh, "_xlReceive@12");
      pxlCanTransmit = (fxlTransmit)dlsym(dlh, "_xlCanTransmit@20");
      pxlGetErrorString = (fxlGetErrorString)dlsym(dlh, "_xlGetErrorString@4");

      if (
          (pxlOpenDriver == NULL) ||
          (pxlClosePort == NULL) ||
          (pxlGetDriverConfig == NULL) ||
          (pxlOpenPort == NULL) ||
          (pxlCanSetChannelBitrate == NULL) ||
          (pxlActivateChannel == NULL) ||
          (pxlReceive == NULL) ||
          (pxlCanTransmit == NULL) ||
          (pxlGetErrorString == NULL) ||
          0
         )
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: unable to get addresses of all function!\n");
        dprintf(1, "dlsym(pxlOpenDriver):0x%08X\n", (unsigned)pxlOpenDriver);
        dprintf(1, "dlsym(pxlClosePort):0x%08X\n", (unsigned)pxlClosePort);
        dprintf(1, "dlsym(pxlGetDriverConfig):0x%08X\n", (unsigned)pxlGetDriverConfig);
        dprintf(1, "dlsym(pxlOpenPort):0x%08X\n", (unsigned)pxlOpenPort);
        dprintf(1, "dlsym(pxlCanSetChannelBitrate):0x%08X\n", (unsigned)pxlCanSetChannelBitrate);
        dprintf(1, "dlsym(pxlActivateChannel):0x%08X\n", (unsigned)pxlActivateChannel);
        dprintf(1, "dlsym(pxlReceive):0x%08X\n", (unsigned)pxlReceive);
        dprintf(1, "dlsym(pxlCanTransmit):0x%08X\n", (unsigned)pxlCanTransmit);
        dprintf(1, "dlsym(pxlGetErrorString):0x%08X\n", (unsigned)pxlGetErrorString);
        dprintf(1, "dlclose:%d\n", status);
        return -1;
      }
      XLstatus xlStatus = pxlOpenDriver();
      dprintf(9, "xlOpenDriver()=%d\n", (int)xlStatus);
      if (xlStatus != XL_SUCCESS)
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: unable to open Vector CAN driver (%d - 0x%X)!\n", (int)xlStatus, (unsigned)xlStatus);
        dprintf(1, "dlclose()=%d\n", status);
        return -1;
      }
      xlStatus = pxlGetDriverConfig(&g_xlDrvConfig);
      if (xlStatus != XL_SUCCESS)
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: xlGetDriverConfig error (%d - 0x%X)!\n", (int)xlStatus, (unsigned)xlStatus);
        dprintf(1, "dlclose()=%d\n", status);
        return -1;
      }
    }
    static int last_socketid = - 1;
    int i;
    for (i = last_socketid + 1; (i < (int)g_xlDrvConfig.channelCount) && (i < MAX_CAN_CHANNEL); i++)
    {
      // we take all hardware we found and
      // check that we have only CAN cabs/piggy's
      // at the moment there is no VN8910 XLAPI support!
      if (g_xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_CAN)
      {
        dprintf(9, "Initialize CAN %d\n", i);
        static const char *g_AppName = "uCAN_Vector_driver";
        XLaccess g_xlPermissionMask = g_xlDrvConfig.channel[i].channelMask;
        XLstatus xlStatus = pxlOpenPort(&g_xlPortHandle[i], g_AppName, g_xlDrvConfig.channel[i].channelMask, &g_xlPermissionMask, RX_QUEUE_SIZE, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN);
        if ( (XL_SUCCESS == xlStatus) && (XL_INVALID_PORTHANDLE != g_xlPortHandle[i]) && (g_xlPermissionMask == g_xlDrvConfig.channel[i].channelMask))
        {
          xlStatus = pxlCanSetChannelBitrate(g_xlPortHandle[i], g_xlDrvConfig.channel[i].channelMask, 250000);
          if (xlStatus != XL_SUCCESS)
          {
            dprintf(2, "Warning: xlCanSetChannelBitrate()=%d!\n", xlStatus);
          }else
          {
            last_socketid = i;
            return last_socketid;
          }
        }else
        {
          dprintf(2, "Warning: xlOpenPort failure: xlStatus=%d g_xlPortHandle=%ld mask: %08llx <=> %08llx!\n", xlStatus, g_xlPortHandle[i], (uint64_t)g_xlPermissionMask, (uint64_t)g_xlDrvConfig.channel[i].channelMask);
        }
      }
    }
    dprintf(2, "Warning: no accessible interface is found!\n");
    return -1;
  }else
  {
    return -1;
  }
}

int uCAN_bind(int fd, const struct sockaddr_can *addr, unsigned int len)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  if ((len < sizeof(struct sockaddr_can)) || (addr->can_family != AF_CAN))
  { /* wrong family type */
    return -2;
  }
  XLstatus xlStatus = pxlActivateChannel(g_xlPortHandle[fd], g_xlDrvConfig.channel[fd].channelMask, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
  if (xlStatus != XL_SUCCESS)
  { /* the channel cannot be activated */
    dprintf(2, "Warning: pxlActivateChannel(%d)=%d!\n", fd, xlStatus);
    return -3;
  }
  return 0;
}

int uCAN_ioctl(int fd, unsigned long int request, struct ifreq* ifr)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  if (request == SIOCSCANBAUDRATE)
  {
    can_baudrate_t baudrate = ifr->ifr_ifru.baudrate;
    XLstatus xlStatus = pxlCanSetChannelBitrate(g_xlPortHandle[fd], g_xlDrvConfig.channel[fd].channelMask, baudrate);
    if (xlStatus != XL_SUCCESS)
    { /* the baudrate cannot be activated to channel */
      dprintf(2, "Warning: pxlCanSetChannelBitrate(%d,0x%llX,%u)=%d!\n", fd, g_xlDrvConfig.channel[fd].channelMask, baudrate, xlStatus);
      return -3;
    }
    return 0;
  }else
  { /* invalid request */
    return -2;
  }
}

int uCAN_setsockopt(int fd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  return -1;
}

int uCAN_fcntl(int fd, int cmd, int flag, int value)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  (void)cmd;
  (void)flag;
  (void)value;
  return -1;
}

int uCAN_read(int fd, void *buf, unsigned int n)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  #define RECEIVE_EVENT_SIZE 1 /* DO NOT EDIT! Currently 1 is supported only */
  unsigned int    msgsrx = RECEIVE_EVENT_SIZE;
  XLevent         xlEvent; 
  XLstatus        xlStatus;
  switch (xlStatus = pxlReceive(g_xlPortHandle[fd], &msgsrx, &xlEvent))
  {
    case XL_ERR_QUEUE_IS_EMPTY:
      return 0;
    case XL_SUCCESS:
    { /* standard or extended message is received */
      if ((n < sizeof(struct can_frame)) || (xlEvent.tagData.msg.dlc > 8))
      { /* too small buffer */
        return -1;
      }
      struct can_frame *frame = (struct can_frame *)buf;
      frame->can_id = xlEvent.tagData.msg.id; /* STD / EXT compatible marking */
      frame->can_dlc = xlEvent.tagData.msg.dlc;
      memcpy(&(frame->data[0]), &(xlEvent.tagData.msg.data[0]), frame->can_dlc);
      frame->can_time_stamp = (uint32_t)(xlEvent.timeStamp/1000);
      return sizeof(struct can_frame);
    }
    //case ACCRES_ERROR_RECEIVED:
    //  return UCAN_READ_BUS_ERROR;
    default:
      dprintf(2, "not handled case: pxlReceive(%d)=%d - %ld\n", fd, xlStatus, g_xlPortHandle[fd]);
      break;
  }
  return -1;
}

int uCAN_write(int fd, const void *buf, unsigned int n)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL) || (n > 8))
  { /* the driver is not initialized correctly */
    return -1;
  }
  XLevent       xlEvent;
  const struct can_frame *frame = (const struct can_frame *)buf;
  memset(&xlEvent, 0, sizeof(xlEvent));
  xlEvent.tag                 = XL_TRANSMIT_MSG;
  xlEvent.tagData.msg.id      = frame->can_id;
  xlEvent.tagData.msg.dlc     = n;
  xlEvent.tagData.msg.flags   = 0;
  memcpy(&(xlEvent.tagData.msg.data[0]), &(frame->data[0]), xlEvent.tagData.msg.dlc);
  unsigned int EventCount = 1;
  XLstatus xlStatus = pxlCanTransmit(g_xlPortHandle[fd], g_xlDrvConfig.channel[fd].channelMask, &EventCount, &xlEvent);
  if(XL_SUCCESS != xlStatus)
  {
    dprintf(1, "Error: xlCanTransmit: %d - %s\n", xlStatus, pxlGetErrorString(xlStatus));
    return -2;
  }
  return n;
}

int uCAN_close(int fd)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  pxlClosePort(g_xlPortHandle[fd]);
  return 0;
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
      struct ifreq req;
      req.ifr_ifru.baudrate = 250000;
      uCAN_ioctl(skt[CAN_ChNum], SIOCSCANBAUDRATE, &req);
      struct sockaddr_can addr;
      addr.can_family = AF_CAN; 
      uCAN_bind( skt[CAN_ChNum], &addr, sizeof(addr) );
      CAN_ChNum++;
    }
  }
  if (CAN_ChNum <= 0)
  { /* no interface is available */
    printf("No CAN interface is connected!\n");
    return 1;
  }
  uint32_t TimeOfs[MAX_CAN];
  memset(&TimeOfs[0], 0, sizeof(TimeOfs));
  for (i = 0; i < CAN_ChNum; i++)
  { /* calculating initial time-stamp */
    struct can_frame frame;
    int status;
    while ((status = uCAN_read(skt[i], &frame, sizeof(frame))) >= (int)sizeof(frame))
    {
      TimeOfs[i] = frame.can_time_stamp;
    }
  }
  time_t t_last = time(NULL);
  uint32_t TimeStampLast = 0;
  while(1)
  {
    if ((time(NULL) - t_last) != 0)
    {
      struct can_frame frame;
      frame.data[0] = (uint8_t)t_last;
      int i;
      for(i = 1; i < 8; i++)
      {
        frame.data[i] = i;
      }
      frame.can_id = 123; /* standard frame */
      printf("uCAN_write(0) = %d\n", uCAN_write(skt[0], &frame, 8));
      frame.can_id = 4096 | CAN_ID_STD_EXT_MASK; /* extended frame */
      printf("uCAN_write(1) = %d\n", uCAN_write(skt[1], &frame, 8));
      t_last = time(NULL);
    }
    for (i = 0; i < CAN_ChNum; i++)
    {
      struct can_frame frame;
      if (uCAN_read(skt[i], &frame, sizeof(frame)) >= (int)sizeof(frame))
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
