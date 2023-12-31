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
 * MSP432 SYSCTL_A_A - Get Flash/SRAM Size
 *
 * Description: In this very simple code example, the SysCtl module APIs are
 * used to retrieve the total user flash and SRAM available to the user. This
 * feature might be useful if the user's program is intended to be used
 * across devices with multiple footprints and intelligently flash/reconfigure
 * certain areas of memory. After the flash/SRAM sizes are retrieved, a
 * no operation function is called where the user is expected to place a
 * breakpoint.
 *
 *                MSP432P4111
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST               |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 *
 ******************************************************************************/
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>

/* Variables we will store the sizes in. Declared volatile so the compiler
 * does not optimize out
 */
volatile uint32_t sramSize, flashSize, infoFlash;

//![Simple SysCtl Example]
int main(void) { /* Halting the Watchdog */
  MAP_WDT_A_holdTimer();

  sramSize  = MAP_SysCtl_A_getSRAMSize();
  flashSize = MAP_SysCtl_A_getFlashSize();
  infoFlash = MAP_SysCtl_A_getInfoFlashSize();

  /* No operation. Set Breakpoint here */
  __no_operation();
}
//![Simple SysCtl Example]
