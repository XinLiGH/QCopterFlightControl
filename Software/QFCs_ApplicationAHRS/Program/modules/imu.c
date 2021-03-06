/**
  *      __            ____
  *     / /__ _  __   / __/                      __  
  *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
  *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
  *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
  *                    /_/   github.com/KitSprout    
  * 
  * @file    imu.c
  * @author  KitSprout
  * @date    27-Mar-2017
  * @brief   
  * 
  */

/* Includes --------------------------------------------------------------------------------*/
#include "drivers\stm32f4_system.h"
#include "drivers\stm32f4_spi.h"
#include "modules\imu.h"

#include <stdio.h>
#include <string.h>

/** @addtogroup STM32_Module
  * @{
  */

/* Private typedef -------------------------------------------------------------------------*/
/* Private define --------------------------------------------------------------------------*/
/* Private macro ---------------------------------------------------------------------------*/
/* Private variables -----------------------------------------------------------------------*/
static uint8_t IMU_TX_BUFFER[IMU_MAX_TXBUF] = {0};
static uint8_t IMU_RX_BUFFER[IMU_MAX_RXBUF] = {0};

/* Private function prototypes -------------------------------------------------------------*/
static void IMU_SetSensitivity( IMU_InitTypeDef *imux );
static void IMU_UpdateDataFactor( imu_t *imux );

/* Private functions -----------------------------------------------------------------------*/

/**
  * @brief  IMU_Config
  */
void IMU_Config( void )
{
  hImu.pTxBuf = IMU_TX_BUFFER;
  hImu.pRxBuf = IMU_RX_BUFFER;

#if defined(__MPU92)
  MPU92_Config();
#endif

#if defined(__LPS22)
  LPS22_Config();
#endif

  /* SPI Init ****************************************************************/
  hImu.handle->Instance               = IMU_SPIx;
  hImu.handle->Init.Mode              = SPI_MODE_MASTER;
  hImu.handle->Init.Direction         = SPI_DIRECTION_2LINES;
  hImu.handle->Init.DataSize          = SPI_DATASIZE_8BIT;
  hImu.handle->Init.CLKPolarity       = SPI_POLARITY_HIGH;
  hImu.handle->Init.CLKPhase          = SPI_PHASE_2EDGE;
  hImu.handle->Init.NSS               = SPI_NSS_SOFT;
  hImu.handle->Init.BaudRatePrescaler = IMU_SPIx_SPEED_LOW;
  hImu.handle->Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hImu.handle->Init.TIMode            = SPI_TIMODE_DISABLE;
  hImu.handle->Init.CRCCalculation    = SPI_CRCCALCULATION_ENABLE;
  hImu.handle->Init.CRCPolynomial     = 7;
  HAL_SPI_Init(hImu.handle);

  __HAL_SPI_ENABLE(hImu.handle);
}

/**
  * @brief  IMU_Init
  */
int8_t IMU_Init( IMU_InitTypeDef *imux )
{
  int8_t status;

#if defined(__MPU92)
  status = MPU92_Init(&imux->InitMPU);
  if (status != SUCCESS) {
    return ERROR;
  }
#endif

#if defined(__LPS22)
  status = LPS22_Init(&imux->InitLPS);
  if (status != SUCCESS) {
    return ERROR;
  }
#endif

  SPI_SetSpeed(hImu.handle, IMU_SPIx_SPEED_HIGH);
  delay_ms(10);

  IMU_InitData(imux->Data);
  IMU_SetSensitivity(imux);
  IMU_UpdateDataFactor(imux->Data);

  return SUCCESS;
}

/**
  * @brief  IMU_InitData
  */
void IMU_InitData( imu_t *imux )
{
  if (imux->calibState != ENABLE) {
    memset(imux, 0, sizeof(imu_t));

    imux->gyrCalib[0] = 1.0f;
    imux->gyrCalib[1] = 1.0f;
    imux->gyrCalib[2] = 1.0f;
    imux->accStrength = 1.0f;
    imux->accCalib[0] = 1.0f;
    imux->accCalib[4] = 1.0f;
    imux->accCalib[8] = 1.0f;
    imux->magStrength = 1.0f;
    imux->magCalib[0] = 1.0f;
    imux->magCalib[4] = 1.0f;
    imux->magCalib[8] = 1.0f;

#if 0
    /* set gyroscope parameters */
    imux->gyrOffset[0] = -5.266666666666667f;
    imux->gyrOffset[1] = -7.883333333333334f;
    imux->gyrOffset[2] = -8.783333333333333f;

    /* set accelerometer parameters */
    imux->accStrength  =  9.8f;
    imux->accCalib[0]  =  1.000591271010064f;
    imux->accCalib[1]  =  0.000586893913887f;
    imux->accCalib[2]  = -0.003436732180952f;
    imux->accCalib[3]  = -0.001485988936797f;
    imux->accCalib[4]  =  1.000157846160545f;
    imux->accCalib[5]  = -0.011743938010525f;
    imux->accCalib[6]  = -0.007978426891497f;
    imux->accCalib[7]  = -0.000416245697376f;
    imux->accCalib[8]  =  0.995341160302247f;
    imux->accOffset[0] =  42.55588470081017f;
    imux->accOffset[1] = -97.96568104396205f;
    imux->accOffset[2] =  316.0875468994960f;

    /* set magnetometer parameters */
    imux->magStrength  =  212.4818615767788f;
    imux->magCalib[0]  =  1.019823067566470f;
    imux->magCalib[1]  = -0.003356411418474f;
    imux->magCalib[2]  = -0.014430324672665f;
    imux->magCalib[3]  = -0.003356411418474f;
    imux->magCalib[4]  =  1.015454920456150f;
    imux->magCalib[5]  =  0.018734747904013f;
    imux->magCalib[6]  = -0.014430324672665f;
    imux->magCalib[7]  =  0.018734747904013f;
    imux->magCalib[8]  =  0.966196977587016f;
    imux->magOffset[0] =  16.228349462609184f;
    imux->magOffset[1] =  170.7384291607371f;
    imux->magOffset[2] = -58.094694639781515f;
#endif
  }
}

/**
  * @brief  IMU_GetRawData
  */
int8_t IMU_GetRawData( imu_t *imux )
{
  int8_t status;

#if defined(__MPU92)
  int16_t data16[10];
#endif

#if defined(__LPS22)
  int32_t data32[2];
#endif

#if defined(__MPU92)
  status = MPU92_GetRawData(data16);
  imux->ictempRaw = data16[0];    /* ICTemp */
  imux->gyrRaw[0] = data16[1];    /* Gyr.X */
  imux->gyrRaw[1] = data16[2];    /* Gyr.Y */
  imux->gyrRaw[2] = data16[3];    /* Gyr.Z */
  imux->accRaw[0] = data16[4];    /* Acc.X */
  imux->accRaw[1] = data16[5];    /* Acc.Y */
  imux->accRaw[2] = data16[6];    /* Acc.Z */

#if defined(__USE_MAGNETOMETER)
  if (status == 1) {
    imux->magRaw[0] = data16[7];  /* Mag.X */
    imux->magRaw[1] = data16[8];  /* Mag.Y */
    imux->magRaw[2] = data16[9];  /* Mag.Z */
  }
#endif

#endif

#if defined(__LPS22)
  status = LPS22_GetRawData(data32);
  if (status == 1) {
    imux->baroRaw[0] = data32[0];
    imux->baroRaw[1] = data32[1];
  }
#endif

  return status;
}

/**
  * @brief  IMU_GetCalibData
  */
void IMU_GetCalibData( imu_t *imux )
{
  float32_t tmp[3] = {0};

  IMU_GetRawData(imux);

#if defined(__USE_GYROSCOPE)
//  tmp[0] = imux->gyrRaw[0] - imux->gyrOffset[0];  /* Gyr.X */
//  tmp[1] = imux->gyrRaw[1] - imux->gyrOffset[1];  /* Gyr.Y */
//  tmp[2] = imux->gyrRaw[2] - imux->gyrOffset[2];  /* Gyr.Z */
//  imux->gyrData[0] = imux->gyrCalib[0] * tmp[0];
//  imux->gyrData[1] = imux->gyrCalib[1] * tmp[1];
//  imux->gyrData[2] = imux->gyrCalib[2] * tmp[2];
  imux->gyrData[0] = imux->gyrRaw[0] - imux->gyrOffset[0];  /* Gyr.X */
  imux->gyrData[1] = imux->gyrRaw[1] - imux->gyrOffset[1];  /* Gyr.Y */
  imux->gyrData[2] = imux->gyrRaw[2] - imux->gyrOffset[2];  /* Gyr.Z */
  imux->gyrInt[0]  = imux->gyrData[0];
  imux->gyrInt[1]  = imux->gyrData[1];
  imux->gyrInt[2]  = imux->gyrData[2];
#endif

#if defined(__USE_ACCELEROMETER)
  tmp[0] = imux->accRaw[0] - imux->accOffset[0];  /* Acc.X */
  tmp[1] = imux->accRaw[1] - imux->accOffset[1];  /* Acc.Y */
  tmp[2] = imux->accRaw[2] - imux->accOffset[2];  /* Acc.Z */
  imux->accData[0] = imux->accCalib[0] * tmp[0] + imux->accCalib[1] * tmp[1] + imux->accCalib[2] * tmp[2];
  imux->accData[1] = imux->accCalib[3] * tmp[0] + imux->accCalib[4] * tmp[1] + imux->accCalib[5] * tmp[2];
  imux->accData[2] = imux->accCalib[6] * tmp[0] + imux->accCalib[7] * tmp[1] + imux->accCalib[8] * tmp[2];
  imux->accInt[0]  = imux->accData[0];
  imux->accInt[1]  = imux->accData[1];
  imux->accInt[2]  = imux->accData[2];
#endif

#if defined(__USE_MAGNETOMETER)
  tmp[0] = imux->magRaw[0] - imux->magOffset[0];  /* Mag.X */
  tmp[1] = imux->magRaw[1] - imux->magOffset[1];  /* Mag.Y */
  tmp[2] = imux->magRaw[2] - imux->magOffset[2];  /* Mag.Z */
  imux->magData[0] = imux->magCalib[0] * tmp[0] + imux->magCalib[1] * tmp[1] + imux->magCalib[2] * tmp[2];
  imux->magData[1] = imux->magCalib[3] * tmp[0] + imux->magCalib[4] * tmp[1] + imux->magCalib[5] * tmp[2];
  imux->magData[2] = imux->magCalib[6] * tmp[0] + imux->magCalib[7] * tmp[1] + imux->magCalib[8] * tmp[2];
  imux->magInt[0]  = imux->magData[0];
  imux->magInt[1]  = imux->magData[1];
  imux->magInt[2]  = imux->magData[2];
#endif

#if defined(__USE_ICTEMPERATURE)
  imux->ictempData = imux->ictempRaw * imux->ictempScale + imux->ictempOffset;
  imux->ictempInt  = imux->ictempData;
#endif

#if defined(__USE_BAROMETER)
  imux->baroData[0] = imux->baroRaw[0];
  imux->baroData[1] = imux->baroRaw[1];
  imux->baroInt[0]  = imux->baroData[0];
  imux->baroInt[1]  = imux->baroData[1];
#endif
}

/**
  * @brief  IMU_GetRealData
  */
void IMU_GetRealData( imu_t *imux )
{
  IMU_GetCalibData(imux);

#if defined(__USE_GYROSCOPE)
  imux->gyrData[0] = imux->gyrData[0] * imux->gyrFactor[0]; /* Gyr.X */
  imux->gyrData[1] = imux->gyrData[1] * imux->gyrFactor[1]; /* Gyr.Y */
  imux->gyrData[2] = imux->gyrData[2] * imux->gyrFactor[2]; /* Gyr.Z */
#endif

#if defined(__USE_ACCELEROMETER)
  imux->accData[0] = imux->accData[0] * imux->accFactor[0]; /* Acc.X */
  imux->accData[1] = imux->accData[1] * imux->accFactor[1]; /* Acc.Y */
  imux->accData[2] = imux->accData[2] * imux->accFactor[2]; /* Acc.Z */
#endif

#if defined(__USE_MAGNETOMETER)
  imux->magData[0] = imux->magData[0] * imux->magFactor[0]; /* Mag.X */
  imux->magData[1] = imux->magData[1] * imux->magFactor[1]; /* Mag.Y */
  imux->magData[2] = imux->magData[2] * imux->magFactor[2]; /* Mag.Z */
#endif

#if defined(__USE_BAROMETER)
  imux->baroData[0] = imux->baroData[0] * imux->baroFactor[0];  /* Pressure */
  imux->baroData[1] = imux->baroData[1] * imux->baroFactor[1];  /* Temperature */
#endif
}

/**
  * @brief  IMU_SetSensitivity
  */
static void IMU_SetSensitivity( IMU_InitTypeDef *IMUx )
{
  float32_t scale[5];

#if defined(__MPU92)
  MPU92_GetSensitivity(&IMUx->InitMPU, scale);

  /* Set gyroscope sensitivity (dps/LSB) */
  IMUx->Data->gyrScale[0] = scale[0];
  IMUx->Data->gyrScale[1] = scale[0];
  IMUx->Data->gyrScale[2] = scale[0];

  /* Set accelerometer sensitivity (g/LSB) */
  IMUx->Data->accScale[0] = scale[1];
  IMUx->Data->accScale[1] = scale[1];
  IMUx->Data->accScale[2] = scale[1];

  /* Set magnetometer sensitivity (uT/LSB) */
  IMUx->Data->magScale[0] = scale[2];
  IMUx->Data->magScale[1] = scale[2];
  IMUx->Data->magScale[2] = scale[2];

  /* Set ictemperature sensitivity (degC/LSB) */
  IMUx->Data->ictempScale  = scale[3];
  IMUx->Data->ictempOffset = scale[4];
#endif

#if defined(__LPS22)
  LPS22_GetSensitivity(scale);

  /* Set barometer pressure sensitivity (hPa/LSB) */
  IMUx->Data->baroScale[0] = scale[0];

  /* Set barometer temperature sensitivity (degC/LSB) */
  IMUx->Data->baroScale[1] = scale[1];
#endif
}

/**
  * @brief  IMU_UpdateDataFactor
  */
static void IMU_UpdateDataFactor( imu_t *imux )
{
#if defined(__USE_GYROSCOPE)
  /* Combine gyroscope scale and sensitivity (dps/LSB) */
  imux->gyrFactor[0] = imux->gyrScale[0];
  imux->gyrFactor[1] = imux->gyrScale[1];
  imux->gyrFactor[2] = imux->gyrScale[2];
#endif

#if defined(__USE_ACCELEROMETER)
  /* Combine accelerometer scale and sensitivity (g/LSB) */
  imux->accFactor[0] = imux->accScale[0] * imux->accStrength;
  imux->accFactor[1] = imux->accScale[1] * imux->accStrength;
  imux->accFactor[2] = imux->accScale[2] * imux->accStrength;
#endif

#if defined(__USE_MAGNETOMETER)
  /* Combine magnetometer scale and sensitivity (uT/LSB) */
  imux->magFactor[0] = imux->magScale[0] * imux->accStrength;
  imux->magFactor[1] = imux->magScale[1] * imux->accStrength;
  imux->magFactor[2] = imux->magScale[2] * imux->accStrength;
#endif

#if defined(__USE_BAROMETER)
  /* Combine barometer scale and sensitivity (hPa/LSB) (degC/LSB) */
  imux->baroFactor[0] = imux->baroScale[0];
  imux->baroFactor[1] = imux->baroScale[1];
#endif
}

/**
  * @brief  IMU_PrintData
  */
void IMU_PrintData( imu_t *imux )
{
  printf("\r\n");
  printf("- IMU Data -----------------------\r\n");
  printf("G_raw : %5i, %5i, %5i\r\n", imux->gyrRaw[0], imux->gyrRaw[1], imux->gyrRaw[2]);
  printf("A_raw : %5i, %5i, %5i\r\n", imux->accRaw[0], imux->accRaw[1], imux->accRaw[2]);
#if defined(__USE_MAGNETOMETER)
  printf("M_raw : %5i, %5i, %5i\r\n", imux->magRaw[0], imux->magRaw[1], imux->magRaw[2]);
#endif
  printf("T_raw : %5i\r\n", imux->ictempRaw);
#if defined(__USE_BAROMETER)
  printf("B_raw : %5i, %5i\r\n", imux->baroRaw[0], imux->baroRaw[1]);
#endif

  printf("\r\n");
  printf("G_offset : %f, %f, %f\r\n", imux->gyrOffset[0], imux->gyrOffset[1], imux->gyrOffset[2]);
  printf("A_offset : %f, %f, %f\r\n", imux->accOffset[0], imux->accOffset[1], imux->accOffset[2]);
#if defined(__USE_MAGNETOMETER)
  printf("M_offset : %f, %f, %f\r\n", imux->magOffset[0], imux->magOffset[1], imux->magOffset[2]);
#endif
  printf("T_offset : %f\r\n", imux->ictempOffset);

  printf("\r\n");
  printf("G_data : %f, %f, %f\r\n", imux->gyrData[0], imux->gyrData[1], imux->gyrData[2]);
  printf("A_data : %f, %f, %f\r\n", imux->accData[0], imux->accData[1], imux->accData[2]);
#if defined(__USE_MAGNETOMETER)
  printf("M_data : %f, %f, %f\r\n", imux->magData[0], imux->magData[1], imux->magData[2]);
#endif
  printf("T_data : %f\r\n", imux->ictempData);
#if defined(__USE_BAROMETER)
  printf("B_data : %f, %f\r\n", imux->baroData[0], imux->baroData[1]);
#endif

  printf("\r\n");
  printf("G_calib : %f, %f, %f\r\n", imux->gyrCalib[0], imux->gyrCalib[1], imux->gyrCalib[2]);
  printf("A_calib : %f, %f, %f\r\n", imux->accCalib[0], imux->accCalib[1], imux->accCalib[2]);
  printf("          %f, %f, %f\r\n", imux->accCalib[3], imux->accCalib[4], imux->accCalib[5]);
  printf("          %f, %f, %f\r\n", imux->accCalib[6], imux->accCalib[7], imux->accCalib[8]);
#if defined(__USE_MAGNETOMETER)
  printf("M_calib : %f, %f, %f\r\n", imux->magCalib[0], imux->magCalib[1], imux->magCalib[2]);
  printf("          %f, %f, %f\r\n", imux->magCalib[3], imux->magCalib[4], imux->magCalib[5]);
  printf("          %f, %f, %f\r\n", imux->magCalib[6], imux->magCalib[7], imux->magCalib[8]);
#endif

  printf("\r\n");
  printf("G_scale : %f, %f, %f\r\n", imux->gyrScale[0], imux->gyrScale[1], imux->gyrScale[2]);
  printf("A_scale : %f, %f, %f\r\n", imux->accScale[0], imux->accScale[1], imux->accScale[2]);
#if defined(__USE_MAGNETOMETER)
  printf("M_scale : %f, %f, %f\r\n", imux->magScale[0], imux->magScale[1], imux->magScale[2]);
#endif
  printf("T_scale : %f\r\n", imux->ictempScale);
#if defined(__USE_BAROMETER)
  printf("B_scale : %f, %f\r\n", imux->baroScale[0], imux->baroScale[1]);
#endif

  printf("\r\n");
#if defined(__USE_MAGNETOMETER)
  printf("A_strength : %f\r\n", imux->accStrength);
  printf("M_strength : %f\r\n", imux->magStrength);
#endif

  printf("----------------------------------\r\n");
  printf("\r\n\r\n");
}

/*************************************** END OF FILE ****************************************/
