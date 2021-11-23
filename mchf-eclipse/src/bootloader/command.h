/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/**
  ******************************************************************************
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    28-October-2011
  * @brief   Header file for command.c
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMMAND_H
#define _COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "gpio.h" // dummy include for HAL
#include "flash_if.h"
#include "uhsdr_version.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    BL_ERR_NONE = 0,
    BL_ERR_USBPROBLEM = 1,
    BL_ERR_NOIMAGE = 2,
    BL_ERR_FLASHTOOSMALL = 3,
    BL_ERR_WRITEDISK = 4,
    BL_ERR_READDISK = 5,
    BL_ERR_FLASHPROG = 6,
    BL_ERR_FLASHERASE = 7,
    BL_ERR_FLASHPROTECT = 8,
} mchf_bootloader_error_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#define BASE_PREFIX                 "fw-"
#define BASE_FILE                  BASE_PREFIX TRX_ID

#define UPLOAD_FILE                BASE_FILE  ".old"
#define DOWNLOAD_FILE              BASE_FILE  ".bin"
#define UPLOAD_FILENAME            "0:"  UPLOAD_FILE
#define DOWNLOAD_FILENAME          "0:"  DOWNLOAD_FILE
#define VERSION                    "Version: "UHSDR_BOOT_VER
#define AUTHOR                     "Author: DF8OE"
#define UPLOAD_FILE_CONF           "cf-mchf.old"
#define UPLOAD_FILENAME_CONF       "0:"  UPLOAD_FILE_CONF
#define DOWNLOAD_FILE_CONF         "cf-mchf.bin"
#define DOWNLOAD_FILENAME_CONF     "0:"  DOWNLOAD_FILE_CONF

#if defined(STM32F4) || defined(STM32F7)
	#define CONFIG_START_ADDRESS  ((uint32_t)0x08008000)
	#define CONFIG_LEIGHT         ((uint32_t)0x00008000)
#elif defined(STM32H7)
	#define CONFIG_START_ADDRESS  ((uint32_t)0x08020000)
	#define CONFIG_LEIGHT         ((uint32_t)0x00040000)
#endif

/* Exported functions ------------------------------------------------------- */
void COMMAND_UPLOAD(void);
void COMMAND_UPLOAD_CONF(void);
void COMMAND_DOWNLOAD(void);
void COMMAND_DOWNLOAD_CONF(void);
void COMMAND_ResetMCU(uint32_t code);



void BootFail_Handler(uint8_t count);
void FlashFail_Handler(mchf_bootloader_error_t errcode);

#ifdef __cplusplus
}
#endif

#endif  /* _COMMAND_H */
