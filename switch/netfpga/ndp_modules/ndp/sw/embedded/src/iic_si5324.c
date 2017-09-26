/*
 * Copyright (c) 2015 Digilent Inc.
 * Copyright (c) 2015 Tinghui Wang (Steve)
 * All rights reserved.
 *
 *  File:
 *        sw/embedded/src/iic_si5324.c
 *
 *  Project:
 *       Reference project
 *
 * Author:
 *        Tinghui Wang (Steve)
 *
 *  Description:
 *        IIC configuration to generate 156.25MHz clocks from SI5324
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


#include "iic_config.h"
#include "xstatus.h"
#include <stdio.h>

/*
 * Read register data from SI5324
 */
int read5324()
{
	u32 Index;
	int Status;
	u8 reg_addr;
	u8 ReadBuffer[20];

    /*
 * Read from Si5324
 * Addr, Bit Field Description
 * 25, N1_HS
 * 31, NC1_LS
 * 40, N2_HS
 * 40, N2_LS
 * 43, N31
 */
//    for( delay = 0; delay < MAX_DELAY_COUNT; delay++);

	reg_addr = 25; //N1_HS
	Status = IicReadData2(IIC_SI5324_ADDRESS, reg_addr, ReadBuffer, 1);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Read Failed.\r\n");
		return XST_FAILURE;
	}
	xil_printf("\r\n");
	for (Index = 0; Index < 1; Index++) {
		xil_printf("Reg %d: N1_HS = 0x%02X\r\n", reg_addr, ReadBuffer[0]);
	}

	reg_addr = 31; //NC1_LS
	Status = IicReadData2(IIC_SI5324_ADDRESS, reg_addr, ReadBuffer, 3);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Read Failed.\r\n");
		return XST_FAILURE;
	}
	xil_printf("\r\n");
	for (Index = 0; Index < 3; Index++) {
		xil_printf("Reg %d: NC1_LS = 0x%02X\r\n", reg_addr++, ReadBuffer[Index]);
	}

	reg_addr = 40; //N2_HS, N2_LS
	Status = IicReadData2(IIC_SI5324_ADDRESS, reg_addr, ReadBuffer, 3);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Read Failed.\r\n");
		return XST_FAILURE;
	}
	xil_printf("\r\n");
	for (Index = 0; Index < 3; Index++) {
		xil_printf("Reg %d: N2_HS_LS = 0x%02X\r\n",reg_addr++, ReadBuffer[Index]);
	}

	reg_addr = 43; //N31
	Status = IicReadData2(IIC_SI5324_ADDRESS, reg_addr, ReadBuffer, 3);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Read Failed.\r\n");
		return XST_FAILURE;
	}

	xil_printf("\r\n");
	for (Index = 0; Index < 3; Index++) {
		xil_printf("Reg %d: N31 = 0x%02X\r\n", reg_addr++, ReadBuffer[Index]);
	}
    return XST_SUCCESS;
}

/*
 * Configure SI5324 to generate 156.25MHz
 */
int config_SI5324() {
	int Status;
	u8 WriteBuffer[10];

	/*
	 * Write to the IIC Switch.
	 */
	WriteBuffer[0] = IIC_BUS_DDR3; //Select Bus7 - Si5326
	Status = IicWriteData(IIC_SWITCH_ADDRESS, WriteBuffer, 1);
	if (Status != XST_SUCCESS) {
		xil_printf("PCA9548 FAILED to select Si5324 IIC Bus\r\n");
		return XST_FAILURE;
	}

	// Set Reg 0, 1, 2, 3, 4
	WriteBuffer[0] = 0;
	WriteBuffer[1] = 0x54;	// Reg 0: Free run, Clock always on, No Bypass (Normal Op)
	WriteBuffer[2] = 0xE4;	// Reg 1: CLKIN2 is second priority
	WriteBuffer[3] = 0x12;	// Reg 2: BWSEL set to 1
	WriteBuffer[4] = 0x15;	// Reg 3: CKIN1 selected,  No Digital Hold, Output clocks disabled during ICAL
	WriteBuffer[5] = 0x92;	// Reg 4: Automatic Revertive, HIST_DEL = 0x12
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 6);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 0-4 FAILED\r\n");
		return XST_FAILURE;
	}

	// Set Reg 10, 11
	WriteBuffer[0] = 10;
	WriteBuffer[1] = 0x08;	// Reg 10: CKOUT2 disabled, CKOUT1 enabled
	WriteBuffer[2] = 0x40;	// Reg 11: CKIN1, CKIN2 enabled
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 3);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 10/11 FAILED\r\n");
		return XST_FAILURE;
	}

	// Write Reg 25 to set N1_HS = 9
	WriteBuffer[0] = 25;
	WriteBuffer[1] = 0xA0;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 2);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 25 FAILED\r\n");
		return XST_FAILURE;
	}

	// Write Regs 31,32,33 to set NC1_LS = 4
	WriteBuffer[0] = 31;
	WriteBuffer[1] = 0x00;
	WriteBuffer[2] = 0x00;
	WriteBuffer[3] = 0x03;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 4);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 31-33 FAILED\r\n");
		return XST_FAILURE;
	}

	// Write Regs 40,41,42 to set N2_HS = 10, N2_LS = 150000
	WriteBuffer[0] = 40;
	WriteBuffer[1] = 0xC2;
	WriteBuffer[2] = 0x49;
	WriteBuffer[3] = 0xEF;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 4);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 40-42 FAILED\r\n");
		return XST_FAILURE;
	}

	// Write Regs 43,44,45 to set N31 = 30475
	WriteBuffer[0] = 43;
	WriteBuffer[1] = 0x00;
	WriteBuffer[2] = 0x77;
	WriteBuffer[3] = 0x0B;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 4);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 43-45 FAILED\r\n");
		return XST_FAILURE;
	}

	// Write Regs 46,47,48 to set N32 = 30475
	WriteBuffer[0] = 46;
	WriteBuffer[1] = 0x00;
	WriteBuffer[2] = 0x77;
	WriteBuffer[3] = 0x0B;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 4);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 46-48 FAILED\r\n");
		return XST_FAILURE;
	}

	// Read Si5324 regs after update
#ifdef SI5324_DEBUG
	read5324();
#endif

    // Start Si5324 Internal Calibration process
	// Write Reg 136 to set ICAL = 1
	WriteBuffer[0] = 136;
	WriteBuffer[1] = 0x40;
	Status = IicWriteData(IIC_SI5324_ADDRESS, WriteBuffer, 2);
	if (Status != XST_SUCCESS) {
		xil_printf("SI5324 IIC Write to Reg 136 FAILED\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}
