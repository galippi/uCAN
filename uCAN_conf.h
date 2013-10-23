#ifndef _UCAN_CONF_H_
#define _UCAN_CONF_H_

#include "stdint.h"

typedef uint32_t canid_t;

struct can_frame {
  canid_t can_id;
  uint32_t can_time_stamp; /* in us */
  uint8_t can_dlc; /** data length */
  uint8_t data[8];
};

#define CAN_ID_MASK         0x7FFFFFFF
#define CAN_ID_STD_EXT_MASK 0x80000000
#define IS_CAN_ID_STD(id) (((id) & CAN_ID_STD_EXT_MASK) ? 0 : 1)
#define IS_CAN_ID_EXT(id) (((id) & CAN_ID_STD_EXT_MASK) ? 1 : 0)

#define CAN_RAW  1

struct sockaddr_can {
  uint16_t   can_family;
  int        can_ifindex;
  union {
    canid_t rx_id;
    canid_t tx_id;
  } can_addr;
};

typedef uint32_t can_baudrate_t;

typedef uint32_t can_ctrlmode_t;
#define CAN_CTRLMODE_NORMAL     0x0
#define CAN_CTRLMODE_LOOPBACK   0x1
#define CAN_CTRLMODE_LISTENONLY 0x2

typedef uint32_t can_mode_t;
#define CAN_MODE_STA 0

typedef struct ifreq
{
  union {
    can_baudrate_t baudrate;
    can_ctrlmode_t ctrlmode;
    can_mode_t     can_mode;
  } ifr_ifru;
}t_ifreq;

#ifndef socklen_t
typedef int socklen_t;
#define socklen_t socklen_t
#endif

/* socket.h defines */
#define SOCK_RAW 3
#define PF_CAN   29
#define AF_CAN   PF_CAN

#define SIOCSCANBAUDRATE 1
#define SIOCSCANCTRLMODE 2
#define SIOCSCANMODE     3

#endif /* _UCAN_CONF_H_ */
