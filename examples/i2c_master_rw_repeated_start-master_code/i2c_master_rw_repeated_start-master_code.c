/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/******************************************************************************
 *  MSP432 I2C - EUSCI_B0 I2C Master TX  bytes to MSP432 Slave - Repeated Start
 *
 *  Description: This demo connects two MSP432 's via the I2C bus. The master
 *  transmits to the slave. This is the MASTER CODE. It continuously
 *  transmits an array of data and demonstrates how to implement an I2C
 *  master transmitter sending multiple bytes followed by a repeated start,
 *  followed by a read of multiple bytes.  This is a common operation for
 *  reading register values from I2C slave devices such as sensors. The
 *  transaction for the I2C that is written looks as follows:
 *
 *  _________________________________________________________
 *  |  Start   |      |  Start   |                   |       |
 *  | 0x48Addr | 0x04 | 0x48Addr |  <10 Byte Read>   | Stop  |
 *  |    W     |      |    R     |                   |       |
 *  |__________|______|__________|___________________|_______|
 *
 *  ACLK = n/a, MCLK = HSMCLK = SMCLK = BRCLK = default DCO = ~3.0MHz
 *
 *                                /|\  /|\
 *                MSP432P4111     10k  10k      MSP432P4111
 *                   slave         |    |         master
 *             -----------------   |    |   -----------------
 *            |     P1.6/UCB0SDA|<-|----+->|P1.6/UCB0SDA     |
 *            |                 |  |       |                 |
 *            |                 |  |       |                 |
 *            |     P1.7/UCB0SCL|<-+------>|P1.7/UCB0SCL     |
 *            |                 |          |                 |
 *
 *****************************************************************************/
/* DriverLib Defines */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Defines */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Slave Address for I2C Slave */
#define SLAVE_ADDRESS    0x48
#define NUM_OF_REC_BYTES 10

/* Variables */
const uint8_t            TXData[] = {0x04};
static uint8_t           RXData[NUM_OF_REC_BYTES];
static volatile uint32_t xferIndex;
static volatile bool     stopSent;

/* I2C Master Configuration Parameter */
const eUSCI_I2C_MasterConfig i2cConfig = {
    EUSCI_B_I2C_CLOCKSOURCE_SMCLK,     // SMCLK Clock Source
    3000000,                           // SMCLK = 3MHz
    EUSCI_B_I2C_SET_DATA_RATE_100KBPS, // Desired I2C Clock of 100khz
    0,                                 // No byte counter threshold
    EUSCI_B_I2C_NO_AUTO_STOP           // No Autostop
};

int main(void) {
  /* Disabling the Watchdog  */
  MAP_WDT_A_holdTimer();

  /* Select Port 1 for I2C - Set Pin 6, 7 to input Primary Module Function,
   *   (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
   */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1, GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
  stopSent = false;
  memset(RXData, 0x00, NUM_OF_REC_BYTES);

  /* Initializing I2C Master to SMCLK at 100khz with no autostop */
  MAP_I2C_initMaster(EUSCI_B0_BASE, &i2cConfig);

  /* Specify slave address */
  MAP_I2C_setSlaveAddress(EUSCI_B0_BASE, SLAVE_ADDRESS);

  /* Enable I2C Module to start operations */
  MAP_I2C_enableModule(EUSCI_B0_BASE);
  MAP_Interrupt_enableInterrupt(INT_EUSCIB0);

  while (1) {
    /* Making sure the last transaction has been completely sent out */
    while (MAP_I2C_masterIsStopSent(EUSCI_B0_BASE))
      ;

    MAP_Interrupt_enableSleepOnIsrExit();

    /* Send start and the first byte of the transmit buffer. */
    MAP_I2C_masterSendMultiByteStart(EUSCI_B0_BASE, TXData[0]);

    /* Sent the first byte, now we need to initiate the read */
    xferIndex = 0;
    MAP_I2C_masterReceiveStart(EUSCI_B0_BASE);
    MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);

    MAP_PCM_gotoLPM0InterruptSafe();
  }
}

/*******************************************************************************
 * eUSCIB0 ISR. The repeated start and transmit/receive operations happen
 * within this ISR.
 *******************************************************************************/
void EUSCIB0_IRQHandler(void) {
  uint_fast16_t status;

  status = MAP_I2C_getEnabledInterruptStatus(EUSCI_B0_BASE);
  MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, status);

  /* Receives bytes into the receive buffer. If we have received all bytes,
   * send a STOP condition */
  if (status & EUSCI_B_I2C_RECEIVE_INTERRUPT0) {
    if (xferIndex == NUM_OF_REC_BYTES - 2) {
      MAP_I2C_disableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);
      MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);

      /*
       * Switch order so that stop is being set during reception of last
       * byte read byte so that next byte can be read.
       */
      MAP_I2C_masterReceiveMultiByteStop(EUSCI_B0_BASE);
      RXData[xferIndex++] = MAP_I2C_masterReceiveMultiByteNext(EUSCI_B0_BASE);
    } else {
      RXData[xferIndex++] = MAP_I2C_masterReceiveMultiByteNext(EUSCI_B0_BASE);
    }
  } else if (status & EUSCI_B_I2C_STOP_INTERRUPT) {
    MAP_Interrupt_disableSleepOnIsrExit();
    MAP_I2C_disableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);
  }
}
