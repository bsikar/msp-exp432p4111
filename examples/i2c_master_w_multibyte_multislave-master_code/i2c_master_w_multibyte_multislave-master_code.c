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
/*******************************************************************************
 *  MSP432 I2C - EUSCI_B0_BASE I2C Master TX multiple bytes to multiple MSP432
 *Slaves
 *
 *  Description: In this example, the MSP432 is used in master mode to transmit
 *  a byte array to multiple I2C slaves using the same EUSCI module. This is
 *  the MASTER code. In this code, the master sends bytes to different slave
 *  addresses by changing the slave address using the I2C_setSlaveAddress
 *  function to change the slave address between transfers. A flip-flop boolean
 *  variable is used for state management.
 *
 *  ACLK = n/a, MCLK = HSMCLK = SMCLK = BRCLK = default DCO = ~3.0MHz
 *
 *                                /|\  /|\
 *                MSP432P4111      10k  10k      MSP432P4111
 *                   slave         |    |         master
 *             -----------------   |    |   -----------------
 *            |     P1.6/UCB0SDA|<-|----+->|P1.6/UCB0SDA     |
 *            |                 |  |       |                 |
 *            |                 |  |       |                 |
 *            |     P1.7/UCB0SCL|<-+------>|P1.7/UCB0SCL     |
 *            |                 |          |                 |
 *
 ******************************************************************************/
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>

/* Slave Address for I2C Slave */
#define SLAVE_ADDRESS_1 0x48
#define SLAVE_ADDRESS_2 0x49

/* Statics */
static uint8_t       TXData = 0;
static uint8_t       TXByteCtr;
static volatile bool sendStopCondition;
static volatile bool sendToSlaveAddress2;

/* I2C Master Configuration Parameter */
const eUSCI_I2C_MasterConfig i2cConfig = {
    EUSCI_B_I2C_CLOCKSOURCE_SMCLK,     // SMCLK Clock Source
    3000000,                           // SMCLK = 3MHz
    EUSCI_B_I2C_SET_DATA_RATE_100KBPS, // Desired I2C Clock of 100khz
    0,                                 // No byte counter threshold
    EUSCI_B_I2C_NO_AUTO_STOP           // No Autostop
};

int main(void) {
  /* Disabling the Watchdog */
  MAP_WDT_A_holdTimer();

  /* Select Port 1 for I2C - Set Pin 6, 7 to input Primary Module Function,
   *   (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
   */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1, GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
  sendStopCondition   = false;
  sendToSlaveAddress2 = false;

  /* Initializing I2C Master to SMCLK at 100khz with no autostop */
  MAP_I2C_initMaster(EUSCI_B0_BASE, &i2cConfig);

  /* Specify slave address. For start we will do our first slave address */
  MAP_I2C_setSlaveAddress(EUSCI_B0_BASE, SLAVE_ADDRESS_1);

  /* Set Master in transmit mode */
  MAP_I2C_setMode(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_MODE);

  /* Enable I2C Module to start operations */
  MAP_I2C_enableModule(EUSCI_B0_BASE);

  /* Enable and clear the interrupt flag */
  MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
                                                EUSCI_B_I2C_NAK_INTERRUPT);

  /* Enable master transmit interrupt */
  MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
                                             EUSCI_B_I2C_NAK_INTERRUPT);
  MAP_Interrupt_enableInterrupt(INT_EUSCIB0);

  while (1) {

    /* Load Byte Counter */
    TXByteCtr = 4;

    /* Making sure the last transaction has been completely sent out */
    while (MAP_I2C_masterIsStopSent(EUSCI_B0_BASE) == EUSCI_B_I2C_SENDING_STOP)
      ;

    /* Sending the initial start condition for appropriate
     *   slave addresses */
    if (sendToSlaveAddress2) {
      MAP_I2C_setSlaveAddress(EUSCI_B0_BASE, SLAVE_ADDRESS_2);
    } else {
      MAP_I2C_setSlaveAddress(EUSCI_B0_BASE, SLAVE_ADDRESS_1);
    }

    MAP_I2C_masterSendMultiByteStart(EUSCI_B0_BASE, TXData++);
    TXByteCtr--;

    while (sendStopCondition == false) {
      /* Go to sleep while data is being transmitted */
      MAP_Interrupt_enableSleepOnIsrExit();
      MAP_PCM_gotoLPM0InterruptSafe();
    }

    sendStopCondition = false;
  }
}

/*******************************************************************************
 * The USCIAB0TX_ISR is structured such that it can be used to transmit any
 * number of bytes by pre-loading TXByteCtr with the byte count. Also, TXData
 * points to the next byte to transmit.
 *******************************************************************************/
void EUSCIB0_IRQHandler(void) {
  uint_fast16_t status;

  status = MAP_I2C_getEnabledInterruptStatus(EUSCI_B0_BASE);
  MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, status);

  if (status & EUSCI_B_I2C_NAK_INTERRUPT) {
    MAP_I2C_masterSendStart(EUSCI_B0_BASE);
    return;
  }

  /* Check the byte counter */
  if (TXByteCtr) {
    /* Send the next data and decrement the byte counter */
    MAP_I2C_masterSendMultiByteNext(EUSCI_B0_BASE, TXData++);
    TXByteCtr--;
  } else {
    MAP_I2C_masterSendMultiByteStop(EUSCI_B0_BASE);
    sendStopCondition   = true;
    sendToSlaveAddress2 = !sendToSlaveAddress2;
    MAP_Interrupt_disableSleepOnIsrExit();
  }
}
