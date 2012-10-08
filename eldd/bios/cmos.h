#ifndef CMOS_H
#define CMOS_H

#include <linux/module.h>
#define  MSG(str, args...)  printk(DEVICE_NAME": "str, ##args)

#define  NUM_CMOS_BANKS           2
#define  CMOS_BANK_SIZE           (0xFF*2)
#define  DEVICE_NAME              "cmos"
#define  CMOS_BANK0_INDEX_PORT    0x70
#define  CMOS_BANK0_DATA_PORT     0x71
#define  CMOS_BANK1_INDEX_PORT    0x72
#define  CMOS_BANK1_DATA_PORT     0x73

#endif
