#include <string.h>
#include <stdio.h> /* only for dprintf */
#include <time.h>

#include "uCAN.h"


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
      /* set the baudrate to 250kBaud */
      struct ifreq req;
      req.ifr_ifru.baudrate = 250000;
      printf("uCAN_ioctl(SIOCSCANBAUDRATE)=%d\n", uCAN_ioctl(skt[CAN_ChNum], SIOCSCANBAUDRATE, &req));
      // Select that CAN interface, and bind the socket to it.
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
  printf("%d channel is found!\n", CAN_ChNum);
  uint32_t TimeOfs[MAX_CAN];
  memset(&TimeOfs[0], 0, sizeof(TimeOfs));
  for (i = 0; i < CAN_ChNum; i++)
  { /* calculating initial time-stamp */
    struct can_frame frame;
    while (uCAN_read(skt[i], &frame, sizeof(frame)) >= (int)sizeof(frame))
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
