/*
 * Copyright (c) 2015 Digilent Inc.
 * Copyright (c) 2015 Tinghui Wang (Steve)
 * All rights reserved.
 *
 *  File:
 *        sw/embedded/src/iic_config.h
 *
 *  Project:
 *       Reference project
 *
 * Author:
 *        Tinghui Wang (Steve)
 *
 *  Description:
 *        Iic related definition used by Iic communication with NetFPGA-SUME board
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

#ifndef IIC_CONFIG_H
#define IIC_CONFIG_H

#include "xil_types.h"

// PCA9548 8-port IIC Switch
#define IIC_SWITCH_ADDRESS 0x74
// Connected to IIC Buses
// Bus 0
#define IIC_BUS_SFP1		0x01
#define IIC_SFP1_ADDRESS  	0x50
// Bus 1
#define IIC_BUS_SFP2		0x02
#define IIC_SFP2_ADDRESS  	0x50
// Bus 2
#define IIC_BUS_SFP3		0x04
#define IIC_SFP3_ADDRESS  	0x50
// Bus 3
#define IIC_BUS_SFP4		0x08
#define IIC_SFP4_ADDRESS  	0x50
// Bus 4
#define IIC_BUS_DDR3		0x10
#define IIC_SI5324_ADDRESS	0x68
#define IIC_DDR3A_ADDRESS	0x01
#define IIC_DDR3B_ADDRESS	0x02
// Bus 5
#define IIC_BUS_FMC			0x20
#define IIC_FMC_CPLD		0x58
#define IIC_FMC_CDCM		0x54
#define IIC_FMC_EEPROM		0x50
// Bus 6
#define IIC_BUS_PCON	 	0x40
// Bus 7
#define IIC_BUS_PMOD		0x80

#define IIC_TIMEOUT			1000000UL

int IicReadData(u8 IicAddr, u8 addr, u8 *BufferPtr, u16 ByteCount);
int IicReadData2(u8 IicAddr, u8 addr, u8 *BufferPtr, u16 ByteCount);
int IicReadData3(u8 IicAddr, u16 addr, u8 *BufferPtr, u16 ByteCount);
int IicWriteData(u8 IicAddr, u8 *BufferPtr, u16 ByteCount);

#endif
