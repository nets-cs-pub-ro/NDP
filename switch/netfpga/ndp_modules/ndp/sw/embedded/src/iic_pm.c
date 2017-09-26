/*
 * Copyright (c) 2015 Digilent Inc.
 * Copyright (c) 2015 Tinghui Wang (Steve)
 * All rights reserved.
 *
 *  File:
 *        sw/embedded/src/iic_pm.c
 *
 *  Project:
 *       Reference project
 *
 * Author:
 *        Tinghui Wang (Steve)
 *
 *  Description:
 *        Iic codes to read power information about NetFPGA-SUME boards through
 *        PMBus.
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

#define PM_CMD_LOAD_PAGE 0x00
#define PM_CMD_POUT	0x96
#define PM_CMD_VOUT 0x8B
#define PM_CMD_IOUT 0x8C

struct pm_info {
	char* railName;
	u8 i2cAddress;
	u8 pageIndex;
	u16 voltage;
	u16 current;
	u16 power;
};

struct pm_info pm[8] = {
	{
		.railName = "VCC1V0",
		.i2cAddress = 0x5C,
		.pageIndex = 0,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "VCC1V5",
		.i2cAddress = 0x5D,
		.pageIndex = 2,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "VCC1V8",
		.i2cAddress = 0x5C,
		.pageIndex = 1,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "VCC2V0",
		.i2cAddress = 0x5C,
		.pageIndex = 2,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "VCC3V3",
		.i2cAddress = 0x5D,
		.pageIndex = 1,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "MGTAVCC",
		.i2cAddress = 0x5C,
		.pageIndex = 3,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "MGTAVTT",
		.i2cAddress = 0x5D,
		.pageIndex = 0,
		.voltage = 0,
		.current = 0,
		.power = 0
	},{
		.railName = "MGTVAUX",
		.i2cAddress = 0x5D,
		.pageIndex = 3,
		.voltage = 0,
		.current = 0,
		.power = 0
	}
};

/*
 * This function reads the information of Power Management
 *
 * @return	XST_SUCCESS if successful else XST_FAILURE.
 *
 */

int pmReadInfo(void) {
	int Status;
	int i;
	u8 WriteBuffer[10];
	u8 ReadBuffer[30];

	/*
	 * Write to the IIC Switch.
	 */
	WriteBuffer[0] = IIC_BUS_PCON; //Select Bus7 - DDR3
	Status = IicWriteData(IIC_SWITCH_ADDRESS, WriteBuffer, 1);
	if (Status != XST_SUCCESS) {
		xil_printf("pmReadInfo: PCA9548 FAILED to select PM IIC Bus\r\n");
		return XST_FAILURE;
	}

	for(i = 0; i < 8; i++) {
		int decimal;
		int small;

		/*
		 * Load Corresponding Page Info
		 */
		WriteBuffer[0] = PM_CMD_LOAD_PAGE;
		WriteBuffer[1] = pm[i].pageIndex; //Select Bus7 - Si5326
		Status = IicWriteData(pm[i].i2cAddress, WriteBuffer, 2);
		if (Status != XST_SUCCESS) {
			xil_printf("PMBus[%x]: Load Page %d Failed\r\n", pm[i].i2cAddress, pm[i].pageIndex);
			return XST_FAILURE;
		}
		xil_printf("Power Rail %s:\r\n", pm[i].railName);

		Status = IicReadData(pm[i].i2cAddress, PM_CMD_LOAD_PAGE, ReadBuffer, 1);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
//		xil_printf("Page: %d\r\n", ReadBuffer[0]);

		/*
		 * Read Voltage
		 */
		Status = IicReadData(pm[i].i2cAddress, PM_CMD_VOUT, ReadBuffer, 2);
		if (Status != XST_SUCCESS) {
			xil_printf("PMBus[%x]: IIC Read Failed\r\n", pm[i].i2cAddress);
			return XST_FAILURE;
		}
		pm[i].voltage = (ReadBuffer[1] << 8) + ReadBuffer[0];
//		xil_printf("Voltage: 0x%x 0x%x V\r\n", ReadBuffer[0], ReadBuffer[1]);
//		xil_printf("Voltage: 0x%x, %d V\r\n", pm[i].voltage, pm[i].voltage);
		decimal = pm[i].voltage >> 13;
		small = (pm[i].voltage - (decimal <<13)) * 1000 / (1<<13);
		xil_printf("\tVoltage: %d.%d V\r\n", decimal, small);

		/*
		 * Read Current
		 */
		Status = IicReadData(pm[i].i2cAddress, PM_CMD_IOUT, ReadBuffer, 2);
		if (Status != XST_SUCCESS) {
			xil_printf("PMBus[%x]: IIC Read Failed\r\n", pm[i].i2cAddress);
			return XST_FAILURE;
		}
		pm[i].current = (ReadBuffer[1] << 8) + ReadBuffer[0];
//		xil_printf("Current: 0x%x 0x%x V\r\n", ReadBuffer[0], ReadBuffer[1]);
//		xil_printf("Current: 0x%x, %d V\r\n", pm[i].current, pm[i].current);
		decimal = pm[i].current >> 13;
		small = (pm[i].current - (decimal <<13)) * 1000 / (1<<13);
		xil_printf("\tCurrent: %d.%d A\r\n", decimal, small);

		/*
		 * Read Voltage
		 */
		Status = IicReadData(pm[i].i2cAddress, PM_CMD_POUT, ReadBuffer, 2);
		if (Status != XST_SUCCESS) {
			xil_printf("PMBus[%x]: IIC Read Failed\r\n", pm[i].i2cAddress);
			return XST_FAILURE;
		}
		pm[i].power = (ReadBuffer[1] << 8) + ReadBuffer[0];
//		xil_printf("Power: 0x%x 0x%x V\r\n", ReadBuffer[0], ReadBuffer[1]);
//		xil_printf("Power: 0x%x, %d V\r\n", pm[i].power, pm[i].power);
		decimal = pm[i].power >> 13;
		small = (pm[i].power - (decimal <<13)) * 1000 / (1<<13);
		xil_printf("\tPower: %d.%d W\r\n", decimal, small);
	}
	return XST_SUCCESS;
}

