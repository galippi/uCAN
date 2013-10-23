#include <dlfcn.h>
#include <string.h>
#include <stdio.h> /* only for dprintf */

#include "uCAN.h"

#ifndef TEST_UCAN
//#define TEST_UCAN 1 /* demo application is enabled */
#define TEST_UCAN 0 /* demo application is disabled */
#endif

#define EXTERN_C
#define WINAPI
#define HANDLE uint32_t
#include "WeCANIntf.h"
typedef int TPCANStatus;

typedef int         (WECANIMPORT *fpM_WeCAN_get_device_count)(void);
typedef TPCANStatus (WECANIMPORT *fpM_WeCAN_initialize_board)(int CH_No);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_initialize_chip)(int CH_No, int presc, int sjw, int tseg1, int tseg2, int sam);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_set_acceptance)(int CH_No, unsigned int AccCodeStd, unsigned int AccMaskStd, unsigned long AccCodeXtd, unsigned long AccMaskXtd);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_set_output_control)(int CH_No, int OutputControl);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_enable_fifo_transmit_ack)(int CH_No);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_start_receive)(int CH_No);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_stop_receive)(int CH_No);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_read_ac)(int CH_No, ParamStruct_t *write_ac_param);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_send_data)(int CH_No, unsigned long Ident, int Xtd, int DataLength, const byte *pData);
typedef TPCANStatus (WECANIMPORT *fpM_WECAN_close_boards)(void);

static const char *CAN_DriverDLL = "wecanusb.dll";

static int *dlh = NULL; /* pointer to DLL */
static fpM_WeCAN_get_device_count pM_WeCAN_get_device_count = NULL;
static fpM_WeCAN_initialize_board pM_WeCAN_initialize_board = NULL;
static fpM_WECAN_initialize_chip pM_WECAN_initialize_chip = NULL;
static fpM_WECAN_set_acceptance pM_WECAN_set_acceptance = NULL;
static fpM_WECAN_set_output_control pM_WECAN_set_output_control = NULL;
static fpM_WECAN_enable_fifo_transmit_ack pM_WECAN_enable_fifo_transmit_ack = NULL;
static fpM_WECAN_start_receive pM_WECAN_start_receive = NULL;
static fpM_WECAN_stop_receive pM_WECAN_stop_receive = NULL;
static fpM_WECAN_read_ac pM_WECAN_read_ac = NULL;
static fpM_WECAN_send_data pM_WECAN_send_data = NULL;
static fpM_WECAN_close_boards pM_WECAN_close_boards = NULL;

#define MAX_CAN_CHANNEL 8
static char ChannelIsInitialized[MAX_CAN_CHANNEL];

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
      pM_WeCAN_get_device_count = (fpM_WeCAN_get_device_count)dlsym(dlh, "_M_WeCAN_get_device_count@0");
      pM_WeCAN_initialize_board = (fpM_WeCAN_initialize_board)dlsym(dlh, "_M_WeCAN_initialize_board@4");
      pM_WECAN_initialize_chip = (fpM_WECAN_initialize_chip)dlsym(dlh, "_M_WECAN_initialize_chip@24");
      pM_WECAN_set_acceptance = (fpM_WECAN_set_acceptance)dlsym(dlh, "_M_WECAN_set_acceptance@20");
      pM_WECAN_set_output_control = (fpM_WECAN_set_output_control)dlsym(dlh, "_M_WECAN_set_output_control@8");
      pM_WECAN_enable_fifo_transmit_ack = (fpM_WECAN_enable_fifo_transmit_ack)dlsym(dlh, "_M_WECAN_enable_fifo_transmit_ack@4");
      pM_WECAN_start_receive = (fpM_WECAN_start_receive)dlsym(dlh, "_M_WECAN_start_receive@4");
      pM_WECAN_stop_receive = (fpM_WECAN_stop_receive)dlsym(dlh, "_M_WECAN_stop_receive@4");
      pM_WECAN_read_ac = (fpM_WECAN_read_ac)dlsym(dlh, "_M_WECAN_read_ac@8");
      pM_WECAN_send_data = (fpM_WECAN_send_data)dlsym(dlh, "_M_WECAN_send_data@20");
      pM_WECAN_close_boards = (fpM_WECAN_close_boards)dlsym(dlh, "_M_WECAN_close_boards@0");
      
      if (
          (pM_WeCAN_get_device_count == NULL) ||
          (pM_WeCAN_initialize_board == NULL) ||
          (pM_WECAN_initialize_chip == NULL) ||
          (pM_WECAN_set_acceptance == NULL) ||
          (pM_WECAN_set_output_control == NULL) ||
          (pM_WECAN_enable_fifo_transmit_ack == NULL) ||
          (pM_WECAN_start_receive == NULL) ||
          (pM_WECAN_stop_receive == NULL) ||
          (pM_WECAN_read_ac == NULL) ||
          (pM_WECAN_send_data == NULL) ||
          (pM_WECAN_close_boards == NULL) ||
          0
         )
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: unable to get addresses of all function!\n");
        dprintf(1, "dlsym(M_WeCAN_get_device_count):0x%08X\n", (unsigned)pM_WeCAN_get_device_count);
        dprintf(1, "dlsym(M_WeCAN_initialize_board):0x%08X\n", (unsigned)pM_WeCAN_initialize_board);
        dprintf(1, "dlsym(M_WECAN_initialize_chip):0x%08X\n", (unsigned)pM_WECAN_initialize_chip);
        dprintf(1, "dlsym(M_WECAN_set_acceptance):0x%08X\n", (unsigned)pM_WECAN_set_acceptance);
        dprintf(1, "dlsym(M_WECAN_set_output_control):0x%08X\n", (unsigned)pM_WECAN_set_output_control);
        dprintf(1, "dlsym(M_WECAN_enable_fifo_transmit_ack):0x%08X\n", (unsigned)pM_WECAN_enable_fifo_transmit_ack);
        dprintf(1, "dlsym(M_WECAN_start_receive):0x%08X\n", (unsigned)pM_WECAN_start_receive);
        dprintf(1, "dlsym(M_WECAN_read_ac):0x%08X\n", (unsigned)pM_WECAN_read_ac);
        dprintf(1, "dlsym(M_WECAN_send_data):0x%08X\n", (unsigned)pM_WECAN_send_data);
        dprintf(1, "dlsym(M_WECAN_close_boards):0x%08X\n", (unsigned)pM_WECAN_close_boards);
        dprintf(1, "dlclose:%d\n", status);
        return -1;
      }
      int device_count = pM_WeCAN_get_device_count(); /* this is necessary to initialize the driver */
      dprintf(9, "M_WeCAN_get_device_count()=%d\n", device_count);
      if (device_count <= 0)
      {
        int status = dlclose(dlh);
        dlh = NULL;
        dprintf(1, "Error: no device is connected (device_count=%d)!\n", device_count);
        dprintf(1, "dlclose:%d\n", status);
        return -1;
      }
    }
    static int socketid = -1;
    int i;
    for (i = socketid + 1; i < 8; i++)
    {
      int Status = pM_WeCAN_initialize_board(i);
      if (Status != NOERROR)
      {
        dprintf(9, "CAN %d Error %04X: %s!\n", i, Status, "");
      }else
      {
        dprintf(9, "Initialize CAN %d: %i - ok\n", i, (int)Status);
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
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  if ((len < sizeof(struct sockaddr_can)) || (addr->can_family != AF_CAN))
  { /* wrong family type */
    return -2;
  }
  if (ChannelIsInitialized[fd] == 0)
  {
    struct ifreq req;
    req.ifr_ifru.baudrate = 250000;
    uCAN_ioctl(fd, SIOCSCANBAUDRATE, &req);
  }
  dprintf(9, "M_WECAN_set_output_control()=%d\n", pM_WECAN_set_output_control(fd, TRANSCIEVER_NORMAL));
  //dprintf(9, "M_WECAN_enable_fifo_transmit_ack()=%d\n", pM_WECAN_enable_fifo_transmit_ack(i));
  dprintf(9, "M_WECAN_start_receive()=%d\n", pM_WECAN_start_receive(fd));
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
    int status = OTHERERROR;
    switch (baudrate)
    {
      case 125000:
        status = pM_WECAN_initialize_chip(fd, INIT_125Kbits, 0);
        break;
      case 250000:
        status = pM_WECAN_initialize_chip(fd, INIT_250Kbits, 0);
        break;
      case 500000:
        status = pM_WECAN_initialize_chip(fd, INIT_500Kbits, 0);
        break;
      case 1000000:
        status = pM_WECAN_initialize_chip(fd, INIT_1Mbits, 0);
        break;
    }
    if (status != NOERROR)
    {
      dprintf(1, "Error: uCAN_ioctl - pM_WECAN_initialize_chip(%d, %u) = %d!\n", fd, baudrate, status);
      return -3;
    }
    dprintf(9, "uCAN_ioctl(%d, SIOCSCANBAUDRATE, %u) - M_WECAN_set_acceptance()=%d\n", fd, baudrate, pM_WECAN_set_acceptance(fd, 0, 0, 0, 0));
    ChannelIsInitialized[fd] = 1;
    return 0;
  }else
  { /* invalid request */
    return -2;
  }
}

int uCAN_setsockopt(int fd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  return -1;
}

int uCAN_fcntl(int fd, int cmd, int flag, int value)
{
  (void)cmd;
  (void)flag;
  (void)value;
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  return -1;
}

int uCAN_read(int fd, void *buf, unsigned int n)
{
  if ((dlh == NULL) || (fd < 0) || (fd >= MAX_CAN_CHANNEL))
  { /* the driver is not initialized correctly */
    return -1;
  }
  ParamStruct_t ParamStruct;
  TPCANStatus status;
  switch (status = pM_WECAN_read_ac(fd, &ParamStruct))
  {
    case ACCRES_NONEW_MSG:
      return 0;
    case ACCRES_STD_FRAME_RECEIVED:
    case ACCRES_EXT_FRAME_RECEIVED:
    { /* standard or extended message is received */
      if ((n < sizeof(struct can_frame)) || (ParamStruct.DataLength > 8))
      { /* too small buffer */
        return -1;
      }
      struct can_frame *frame = (struct can_frame *)buf;
      frame->can_id = ParamStruct.Ident;
      if (status == ACCRES_EXT_FRAME_RECEIVED)
      { /* marking extended frame */
        frame->can_id |= 0x80000000;
      }
      frame->can_dlc = ParamStruct.DataLength;
      memcpy(&(frame->data[0]), &(ParamStruct.RCV_data[0]), ParamStruct.DataLength);
      frame->can_time_stamp = ParamStruct.Time;
      return sizeof(struct can_frame);
    }
    case ACCRES_ERROR_RECEIVED:
      return -2;
    default:
      dprintf(2, "not handled case: M_WECAN_read_ac(%d)=%d\n", fd, status);
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
  const struct can_frame *frame = (const struct can_frame *)buf;
  TPCANStatus  status = pM_WECAN_send_data(fd, frame->can_id & CAN_ID_MASK, IS_CAN_ID_EXT(frame->can_id), n, frame->data);
  if (status != NOERROR)
  {
    dprintf(1, "Error: pM_WECAN_send_data: %d - %s\n", status, "");
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
  pM_WECAN_stop_receive(fd);
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
      struct sockaddr_can addr;
      addr.can_family = AF_CAN; 
      uCAN_bind( skt[CAN_ChNum], &addr, sizeof(addr) );
      CAN_ChNum++;
    }else
    { /* no more channel is found */
      break;
    }
  }
  if (CAN_ChNum <= 0)
  { /* no interface is available */
    printf("No WeCAN interface is connected!\n");
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
