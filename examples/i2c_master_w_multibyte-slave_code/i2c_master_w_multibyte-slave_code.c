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
 *  MSP432 I2C - EUSCI_B0_BASE I2C Slave RX multiple bytes from MSP432 Master
 *
 *  Description: This demo connects two MSP432 's via the I2C bus. The master
 *  transmits to the slave. This is the slave code. The interrupt driven
 *  data reception is demonstrated using the USCI_B0 RX interrupt. Data is
 *  stored in the RXData array.
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

/* Application Defines */
#define SLAVE_ADDRESS   0x48
#define NUM_OF_RX_BYTES 4

/* Statics */
static volatile uint8_t  RXData[NUM_OF_RX_BYTES];
static volatile uint32_t xferIndex;

int main(void) {
  /* Disabling the Watchdog */
  MAP_WDT_A_holdTimer();
  xferIndex = 0;

  /* Select Port 1 for I2C - Set Pin 6, 7 to input Primary Module Function,
   *   (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
   */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1, GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

  /* eUSCI I2C Slave Configuration */
  MAP_I2C_initSlave(EUSCI_B0_BASE, SLAVE_ADDRESS,
                    EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
                    EUSCI_B_I2C_OWN_ADDRESS_ENABLE);

  /* Set in receive mode */
  MAP_I2C_setMode(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_MODE);

  /* Enable the module and enable interrupts */
  MAP_I2C_enableModule(EUSCI_B0_BASE);
  MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);
  MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);
  MAP_Interrupt_enableSleepOnIsrExit();
  MAP_Interrupt_enableInterrupt(INT_EUSCIB0);
  MAP_Interrupt_enableMaster();

  /* Sleeping while not in use */
  while (1) { MAP_PCM_gotoLPM0(); }
}

/******************************************************************************
 * The USCI_B0 data ISR RX vector is used to move received data from the I2C
 * master to the MSP432 memory.
 ******************************************************************************/
void EUSCIB0_IRQHandler(void) {
  uint_fast16_t status;

  status = MAP_I2C_getEnabledInterruptStatus(EUSCI_B0_BASE);
  MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, status);

  /* RXIFG */
  if (status & EUSCI_B_I2C_RECEIVE_INTERRUPT0) {
    RXData[xferIndex++] = MAP_I2C_slaveGetData(EUSCI_B0_BASE);

    /* Resetting the index if we are at the end */
    if (xferIndex == NUM_OF_RX_BYTES)
      xferIndex = 0;
  }
}
