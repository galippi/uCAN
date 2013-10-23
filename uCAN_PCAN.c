#include <dlfcn.h>
#include <string.h>
#include <stdio.h> /* only for dprintf */

#include "uCAN.h"

#ifndef TEST_UCAN
//#define TEST_UCAN 1 /* demo application is enabled */
#define TEST_UCAN 0 /* demo application is enabled */
#endif

#define DWORD  unsigned long
#define WORD   unsigned short
#define BYTE   unsigned char
#define LPSTR  char*
#include "PCANBasic.h"

typedef TPCANStatus (__stdcall *fpInitialize)(TPCANHandle, TPCANBaudrate, TPCANType, DWORD, WORD);
typedef TPCANStatus (__stdcall *fpUninitialize)(TPCANHandle);
typedef TPCANStatus (__stdcall *fpOneParam)(TPCANHandle);
typedef TPCANStatus (__stdcall *fpRead)(TPCANHandle, TPCANMsg*, TPCANTimestamp*);
typedef TPCANStatus (__stdcall *fpWrite)(TPCANHandle, TPCANMsg*);
typedef TPCANStatus (__stdcall *fpFilterMessages)(TPCANHandle, DWORD, DWORD, TPCANMode);
typedef TPCANStatus (__stdcall *fpGetSetValue)(TPCANHandle, TPCANParameter, void*, DWORD);
typedef TPCANStatus (__stdcall *fpGetErrorText)(TPCANStatus, WORD, LPSTR);

static const char *CAN_DriverDLL = "PCANBasic.dll";

static int *dlh = NULL; /* pointer to DLL */
static fpInitialize pInitialize = NULL;
static fpUninitialize pUninitialize = NULL;
static fpRead pRead = NULL;
static fpWrite pWrite = NULL;
static fpFilterMessages pFilterMessages = NULL;

#define MAX_CAN_CHANNEL 8

uint32_t uCAN_GetInterfaceVersion(void)
{
  return UCAN_INTERFACE_VERSION_SET(1, 1);
}

int uCAN_socket(int domain, int type, int protocol)
{
  static int socketid = PCAN_USBBUS1 - 1;
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
      pInitialize = (fpInitialize)dlsym(dlh, "CAN_Initialize");
      pRead = (fpRead)dlsym(dlh, "CAN_Read");
      pWrite = (fpWrite)dlsym(dlh, "CAN_Write");
      pFilterMessages = (fpFilterMessages)dlsym(dlh, "CAN_FilterMessages");
      pUninitialize = (fpUninitialize)dlsym(dlh, "CAN_Uninitialize");

      if (
          (pInitialize == NULL) ||
          (pRead == NULL) ||
          (pWrite == NULL) ||
          (pFilterMessages == NULL) ||
          0
         )
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: unable to get addresses of all function!\n");
        dprintf(1, "dlsym(pInitialize):0x%08X\n", (unsigned)pInitialize);
        dprintf(1, "dlsym(pRead):0x%08X\n", (unsigned)pRead);
        dprintf(1, "dlsym(pWrite):0x%08X\n", (unsigned)pWrite);
        dprintf(1, "dlsym(pFilterMessages):0x%08X\n", (unsigned)pFilterMessages);
        dprintf(1, "dlsym(pUninitialize):0x%08X\n", (unsigned)pUninitialize);
        dprintf(1, "dlclose:%d\n", status);
        return -1;
      }
    }
    int i;
    for (i = socketid + 1; i < (PCAN_USBBUS1 + 8); i++)
    {
      int Status = pInitialize(i, PCAN_BAUD_250K, 0, 0, 0);
      if (Status != PCAN_ERROR_OK)
      {
        dprintf(9, "CAN %d Error %04X: %s!\n", i, Status, "");
      }else
      {
        dprintf(9, "Initialize CAN %d: %i - ok\n", i, (int)Status);
        dprintf(9, "pFilterMessages(%d)=%ld\n", i, pFilterMessages(i, 0,      0x7FF, PCAN_MODE_STANDARD));
        dprintf(9, "pFilterMessages(%d)=%ld\n", i, pFilterMessages(i, 0, 0x3FFFFFFF, PCAN_MODE_EXTENDED));
        socketid = i;
        return socketid;
      }
    }
    return -1; /* no available channel is found */
  }else
  {
    return -1;
  }
}

int uCAN_bind(int fd, const struct sockaddr_can *addr, unsigned int len)
{
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
  { /* the driver is not initialized correctly */
    return -1;
  }
  if ((len < sizeof(struct sockaddr_can)) || (addr->can_family != AF_CAN))
  { /* wrong family type */
    return -2;
  }
  return 0;
}

int uCAN_ioctl(int fd, unsigned long int request, struct ifreq* ifr)
{
  (void)request;
  (void)ifr;
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
  { /* the driver is not initialized correctly */
    return -1;
  }
  if (request == SIOCSCANBAUDRATE)
  {
    int status;
    if ((status = pUninitialize(fd)) != PCAN_ERROR_OK)
    { /* Unable to uninitialize channel */
      dprintf(1, "Error: uCAN_ioctl - pUninitialize(%d) = %d!\n", fd, status);
      return -4;
    }
    can_baudrate_t baudrate = ifr->ifr_ifru.baudrate;
    status = PCAN_ERROR_ILLPARAMVAL;
    switch (baudrate)
    {
      case 125000:
        status = pInitialize(fd, PCAN_BAUD_125K, 0, 0, 0);
        break;
      case 250000:
        status = pInitialize(fd, PCAN_BAUD_250K, 0, 0, 0);
        break;
      case 500000:
        status = pInitialize(fd, PCAN_BAUD_500K, 0, 0, 0);
        break;
      case 1000000:
        status = pInitialize(fd, PCAN_BAUD_1M, 0, 0, 0);
        break;
    }
    if (status != PCAN_ERROR_OK)
    {
      dprintf(1, "Error: uCAN_ioctl - pInitialize(%d, %u) = %d!\n", fd, baudrate, status);
      return -3;
    }
    return 0;
  }else
  { /* invalid request - do nothing*/
  }
  return -2; /* invalid request */
}

int uCAN_setsockopt(int fd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
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
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
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
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
  { /* the driver is not initialized correctly */
    return -1;
  }
  TPCANMsg MessageBuffer;
  TPCANTimestamp TimestampBuffer;
  TPCANStatus status;
  switch (status = pRead(fd, &MessageBuffer, &TimestampBuffer))
  {
    case PCAN_ERROR_QRCVEMPTY:
      return 0;
    case PCAN_ERROR_OK:
    { /* standard or extended message is received */
      if ((n < sizeof(struct can_frame)) || (MessageBuffer.LEN > 8))
      { /* too small buffer */
        return -1;
      }
      struct can_frame *frame = (struct can_frame *)buf;
      frame->can_id = MessageBuffer.ID;
      if (MessageBuffer.MSGTYPE != PCAN_MESSAGE_STANDARD)
      { /* marking extended frame */
        frame->can_id |= 0x80000000;
      }
      frame->can_dlc = MessageBuffer.LEN;
      memcpy(&(frame->data[0]), &(MessageBuffer.DATA[0]), frame->can_dlc);
      frame->can_time_stamp = TimestampBuffer.millis * 1000 + TimestampBuffer.micros;
      return sizeof(struct can_frame);
    }
    //case ACCRES_ERROR_RECEIVED:
    //  return UCAN_READ_BUS_ERROR;
    default:
      dprintf(2, "not handled case: pRead(%d)=%ld\n", fd, status);
      break;
  }
  return -1;
}

int uCAN_write(int fd, const void *buf, unsigned int n)
{
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 + MAX_CAN_CHANNEL)) ||(n > 8))
  { /* the driver is not initialized correctly */
    return -1;
  }
  const struct can_frame *frame = (const struct can_frame *)buf;
  TPCANMsg MessageBuffer;
  MessageBuffer.ID = frame->can_id & CAN_ID_MASK;
  if (IS_CAN_ID_EXT(frame->can_id))
  {
    MessageBuffer.MSGTYPE = PCAN_MESSAGE_EXTENDED;
  }else
  {
    MessageBuffer.MSGTYPE = PCAN_MESSAGE_STANDARD;
  }
  MessageBuffer.LEN = n;
  memcpy(&(MessageBuffer.DATA[0]), &(frame->data[0]), n);
  TPCANStatus status = pWrite(fd, &MessageBuffer);
  dprintf(9, "pWrite(%d, ...)=%ld\n", fd, status);
  return n;
}

int uCAN_close(int fd)
{
  if ((dlh == NULL) || (fd < PCAN_USBBUS1) || (fd >= (PCAN_USBBUS1 +MAX_CAN_CHANNEL)))
  { /* the driver is not initialized correctly */
    return -1;
  }
  pUninitialize(fd);
  return 0;
}

#if defined(TEST_UCAN) && (TEST_UCAN != 0)

#include <time.h>

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
      // Select that CAN interface, and bind the socket to it.
      struct sockaddr_can addr;
      addr.can_family = AF_CAN; 
      uCAN_bind( skt[CAN_ChNum], &addr, sizeof(addr) );
      CAN_ChNum++;
    }
  }
  if (CAN_ChNum <= 0)
  { /* no interface is available */
    printf("No PCAN interface is connected!\n");
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
