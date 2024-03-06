#ifndef BSP_SDMMC_H
#define BSP_SDMMC_H

#include "esp_log.h"
#include "esp_err.h"

#define MOUNT_POINT "/sdcard"
#define FORMAT_IF_MOUNT_FAILED 0
#define SDMMC_BUS_WIDTH_4 0
#define SDMMC_PIN_CLK 8
#define SDMMC_PIN_CMD 10
#define SDMMC_PIN_DATA0 3

esp_err_t fs_init(void);


#endif // BSP_SDMMC_H