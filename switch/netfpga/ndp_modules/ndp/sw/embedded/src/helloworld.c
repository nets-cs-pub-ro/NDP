/*
 * Copyright (c) 2015 Digilent Inc.
 * Copyright (c) 2015 Tinghui Wang (Steve)
 * All rights reserved.
 *
 *  File:
 *        sw/embedded/src/helloworld.c
 *
 *  Project:
 *       Reference project
 *
 * Author:
 *        Tinghui Wang (Steve)
 *
 *  Description:
 *        Reference project main function.
 *
 * @NETFPGA_LICENSE_HEADER_START@
 *
 * Licensed to NetFPGA C.I.C. (NetFPGA) under one or more contributor
 * license agreements.  See the NOTICE file distributed with this work for
 * additional information regarding copyright ownership.  NetFPGA licenses this
 * file to you under the NetFPGA Hardware-Software License, Version 1.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at:
 *
 *   http://www.netfpga-cic.org
 *
 * Unless required by applicable law or agreed to in writing, Work distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @NETFPGA_LICENSE_HEADER_END@
 *
*/


#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "string.h"
#include "xiic.h"
#include "xintc.h"
#include "xil_types.h"
#include "platform.h"
#include "mb_interface.h"
#include "xuartlite_l.h"

#define IIC_DEVICE_ID               XPAR_IIC_0_DEVICE_ID
#define INTC_DEVICE_ID              XPAR_INTC_0_DEVICE_ID
#define IIC_INTR_ID                 XPAR_INTC_0_IIC_0_VEC_ID

XIic IicInstance;		/* The instance of the IIC device. */
XIntc InterruptController;	/* The instance of the Interrupt Controller. */

/*
 * printTestResult
 *
 * Print "Passed/Failed" for some test category based on the auto test return value
 */
void printTestResult (char* testName, XStatus result) {
	int dotLen = 40 - strlen(testName);
	int i;

	xil_printf("%s", testName);
	for(i = 0; i < dotLen; i++) {
		xil_printf(".");
	}

	if(result == XST_SUCCESS) {
		xil_printf("Passed\r\n");
	} else {
		xil_printf("Failed\r\n");
	}
	return;
}

/*
 * runAutoTest
 *
 * Test all the interface supported by the project automatically
 */
void runAutoTest(void) {
}

/*
 * runManualTest
 *
 * Provide a menu for user to test each interface manually 
 */
void runManualTest(void) {
	xil_printf("\r\n");
	while(1) {
		xil_printf("---- NetFPGA-SUME Manual Test Menu ----\r\n");
		xil_printf("p: Read Power Info\r\n");
		xil_printf("b: Back to Main Menu\r\n");
		xil_printf("Select: ");
		char cmd = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
		xil_printf("%c\r\n", cmd);
		switch (cmd) {
			case 'p':
				pmReadInfo();
				break;
			case 'b':
				return;
			default:
				break;
		}
		xil_printf("\r\n");
	}
}

int main()
{
    int Status;

    init_platform();

	xil_printf("NetFPGA-SUME SI5324 Configuration\r\n");

	/*
	 * Setup Iic Instance
	 */
	Status = IicInit(&IicInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C Initialization FAILED\n\r");
		return XST_FAILURE;
	}

	/*
	 * Setup the Interrupt System.
	 */
	Status = SetupInterruptSystem(&IicInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("SetupInterruptSystem FAILED\n\r");
		return XST_FAILURE;
	}

	/*
	 * Enable Iic Bus
	 */
	Status = IicInitPost(&IicInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C Initialization FAILED\n\r");
		return XST_FAILURE;
	}

	config_SI5324();

	while(1) {
		xil_printf("============ NetFPGA-SUME ============\n\r");
		xil_printf("m: Manual Test \r\n");
		xil_printf("Select: ");
		char cmd = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
		xil_printf("%c\r\n", cmd);
		switch (cmd) {
			case 'm':
				runManualTest();
				break;
			default:
				break;
		}
		xil_printf("\r\n");
	}

    return 0;
}

/*****************************************************************************/
/**
* This function setups the interrupt system so interrupts can occur for the
* IIC device. The function is application-specific since the actual system may
* or may not have an interrupt controller. The IIC device could be directly
* connected to a processor without an interrupt controller. The user should
* modify this function to fit the application.
*
* @param	IicInstPtr contains a pointer to the instance of the IIC device
*		which is going to be connected to the interrupt controller.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
int SetupInterruptSystem(XIic * IicInstPtr)
{
	int Status;

	if (InterruptController.IsStarted == XIL_COMPONENT_IS_STARTED) {
		return XST_SUCCESS;
	}

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 */
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XIntc_Connect(&InterruptController, IIC_INTR_ID,
			       (XInterruptHandler) XIic_InterruptHandler,
			       IicInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller so interrupts are enabled for all
	 * devices that cause interrupts.
	 */
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupts for the IIC device.
	 */
	XIntc_Enable(&InterruptController, IIC_INTR_ID);

	/*
	 * Enable the Microblaze Interrupts.
	 */
	microblaze_enable_interrupts();

	return XST_SUCCESS;
}
