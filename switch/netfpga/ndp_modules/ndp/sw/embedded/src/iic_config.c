/*
 * Copyright (c) 2015 Digilent Inc.
 * Copyright (c) 2015 Tinghui Wang (Steve)
 * All rights reserved.
 *
 *  File:
 *        sw/embedded/src/iic_config.c
 *
 *  Project:
 *       Reference project
 *
 * Author:
 *        Tinghui Wang (Steve)
 *
 *  Description:
 *        Read/Write functions with timeout ability for IIC communication used by
 *        acceptance_test project.
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
#include "xiic.h"
#include "xintc.h"
#include "xstatus.h"
#include "xil_types.h"
#include "xparameters.h"
#include "math.h"

/*
 * Flags for IIC Transmit/Receive
 */
int TransmitComplete = 0;
int ReceiveComplete = 0;

extern XIic IicInstance;

/*
 * Initialize Iic Structure
 */
int IicInit(XIic *IicInstPtr) {

	XIic_Config *IicConfigPtr;
	int Status;

	/*
	 * Initialize the IIC device Instance.
	 */
	IicConfigPtr = XIic_LookupConfig(XPAR_IIC_0_DEVICE_ID);
	if (IicConfigPtr == NULL) {
		return XST_FAILURE;
	}

	/*
	 * Initialize Iic Instance with Config Ptr
	 */
	Status = XIic_CfgInitialize(IicInstPtr, IicConfigPtr, IicConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Error: XIic_Initialize FAILED\n\r");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*
 * Iic Send Handler
 * Reset Transmit Flag to 0 after Transmit is done
 */
static void IicSendHandler(XIic *IicInstPtr) {
	TransmitComplete = 0;
}

/*
 * Iic Receive Handler
 * Reset Receive Flag to 0 after Transmit is done
 */
static void IicRecvHandler(XIic *IicInstPtr) {
	ReceiveComplete = 0;
}

/*
 * Iic Status Handler
 * Do nothing
 */
static void IicStatusHandler(XIic *IicInstPtr) {
	return;
}

/*
 * Iic Post Initialization Procedure
 * Need to be called after Interrupt system is configured.
 */
int IicInitPost(XIic *IicInstPtr) {
	// Setup Handlers for transmit and reception
	XIic_SetSendHandler(IicInstPtr, IicInstPtr, (XIic_Handler) IicSendHandler);
	XIic_SetRecvHandler(IicInstPtr, IicInstPtr, (XIic_Handler) IicRecvHandler);
	XIic_SetStatusHandler(IicInstPtr, IicInstPtr, (XIic_StatusHandler) IicStatusHandler);

	// Release reset on the PCA9548 IIC Switch
	XIic_SetGpOutput(IicInstPtr, 0xFF);
	XIic_SetGpOutput(IicInstPtr, 0x00);

	return XST_SUCCESS;
}

/*
 * *********************************************************
 * IicReadData with address added as an input parameter - Repeated Start
 * *********************************************************
 */
int IicReadData(u8 IicAddr, u8 addr, u8 *BufferPtr, u16 ByteCount)
{
	int Status;
	u8 IicOptions;
	u32 IicTimeoutCounter = 0;
	/*
	 * Set Receive Flag
	 */
	ReceiveComplete = 1;

	IicOptions = XIic_GetOptions(&IicInstance);
	XIic_SetOptions(&IicInstance, IicOptions | XII_REPEATED_START_OPTION);

	/*
	 * Start Iic Device
	 */
	Status = XIic_Start(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Start failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Set Iic Address
	 */
	Status = XIic_SetAddress(&IicInstance, XII_ADDR_TO_SEND_TYPE, IicAddr);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Set Address failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Write addr to the device
	 */
	// Mark the Transmit Flag
	TransmitComplete = 1;
	IicInstance.Stats.TxErrors = 0;

	/*
	 * Send the Data
	 */
	Status = XIic_MasterSend(&IicInstance, &addr, 1);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Master Send failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Wait till the transmission is completed
	 */
	while((TransmitComplete) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Clear Repeated Start option
	 */
	XIic_SetOptions(&IicInstance, IicOptions);

	/*
	 * Handle Tx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Write Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Receive Data
	 */
	Status = XIic_MasterRecv(&IicInstance, BufferPtr, ByteCount);
	if(Status != XST_SUCCESS) {
		if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
			xil_printf("IicReadData: IIC Master Recv Failed with status %x\r\n", Status);
#endif
			return XST_FAILURE;
		}
	}

	/*
	 * Wait until all the data is received
	 */
	IicTimeoutCounter = 0;

	while(((ReceiveComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Handle Rx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Recv Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Stop Iic
	 */
	Status = XIic_Stop(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData: IIC Stop Failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*
 * *********************************************************
 * IicReadData with address added as an input parameter - Stop
 * *********************************************************
 */
int IicReadData2(u8 IicAddr, u8 addr, u8 *BufferPtr, u16 ByteCount)
{
	int Status;
	u32 IicTimeoutCounter = 0;

	/*
	 * Set Receive Flag
	 */
	ReceiveComplete = 1;

	/*
	 * Start Iic Device
	 */
	Status = XIic_Start(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Start failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Set Iic Address
	 */
	Status = XIic_SetAddress(&IicInstance, XII_ADDR_TO_SEND_TYPE, IicAddr);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Set Address failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Write addr to the device
	 */
	// Mark the Transmit Flag
	TransmitComplete = 1;
	IicInstance.Stats.TxErrors = 0;

	/*
	 * Send the Data
	 */
	Status = XIic_MasterSend(&IicInstance, &addr, 1);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Master Send failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Wait till the transmission is completed
	 */
	while(((TransmitComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Handle Tx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Write Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData2: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Receive Data
	 */
	Status = XIic_MasterRecv(&IicInstance, BufferPtr, ByteCount);
	if(Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
			xil_printf("IicReadData2: IIC Master Recv Failed with status %x\r\n", Status);
#endif
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}

	/*
	 * Wait until all the data is received
	 */
	IicTimeoutCounter = 0;
	while(((ReceiveComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Handle Rx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Recv Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData2: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Stop Iic
	 */
	Status = XIic_Stop(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData2: IIC Stop Failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*
 * *********************************************************
 * IicReadData3 with address added as an input parameter - two bytes address
 * *********************************************************
 */
int IicReadData3(u8 IicAddr, u16 addr, u8 *BufferPtr, u16 ByteCount)
{
	int Status;
	u8 IicOptions;
	u32 IicTimeoutCounter = 0;

	/*
	 * Set Receive Flag
	 */
	ReceiveComplete = 1;

	/*
	 * Start Iic Device
	 */
	Status = XIic_Start(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Start failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Set Iic Address
	 */
	Status = XIic_SetAddress(&IicInstance, XII_ADDR_TO_SEND_TYPE, IicAddr);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Set Address failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Write addr to the device
	 */
	// Mark the Transmit Flag
	TransmitComplete = 1;
	IicInstance.Stats.TxErrors = 0;

	/*
	 * Send the Data
	 */
	u8 addrReorder[2];
	u8 *addrPtr;
	addrPtr = &addr;
	addrReorder[0] = addrPtr[1];
	addrReorder[1] = addrPtr[0];
	Status = XIic_MasterSend(&IicInstance, addrReorder, 2);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Master Send failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Wait till the transmission is completed
	 */
	while(((TransmitComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Handle Tx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Write Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData3: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Receive Data
	 */
	Status = XIic_MasterRecv(&IicInstance, BufferPtr, ByteCount);
	if(Status != XST_SUCCESS) {
		if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
			xil_printf("IicReadData3: IIC Master Recv Failed with status %x\r\n", Status);
#endif
			return XST_FAILURE;
		}
	}

	/*
	 * Wait until all the data is received
	 */
	IicTimeoutCounter = 0;
	while(((ReceiveComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter ++;
	}

	/*
	 * Handle Rx Timeout
	 */
	if (IicTimeoutCounter > IIC_TIMEOUT) {
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Recv Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicReadData3: IIC Stop Failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Stop Iic
	 */
	Status = XIic_Stop(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicReadData3: IIC Stop Failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


/*
 * *********************************************************
 * IicWriteData with address added as an input parameter
 * *********************************************************
 */
int IicWriteData(u8 IicAddr, u8 *BufferPtr, u16 ByteCount)
{
	int Status;
	u32 IicTimeoutCounter = 0;

	// Mark the Transmit Flag
	TransmitComplete = 1;
	IicInstance.Stats.TxErrors = 0;

	/*
	 * Start Iic Device
	 */
	Status = XIic_Start(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicWriteData: IIC Start Device Failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Set Iic Address
	 */
	Status = XIic_SetAddress(&IicInstance, XII_ADDR_TO_SEND_TYPE, IicAddr);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicWriteData: IIC Set Address Failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Send the Data
	 */
	Status = XIic_MasterSend(&IicInstance, BufferPtr, ByteCount);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicWriteData: IIC Master Send failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	/*
	 * Wait till the transmission is completed
	 */
	while(((TransmitComplete) || (XIic_IsIicBusy(&IicInstance)==TRUE)) && IicTimeoutCounter <= IIC_TIMEOUT) {
		IicTimeoutCounter++;
	}

	if (IicTimeoutCounter > IIC_TIMEOUT) {
		TransmitComplete = 0;
		XIic_Reset(&IicInstance);
		Status = XIic_Stop(&IicInstance);
#ifdef IIC_DEBUG
		xil_printf("IicWriteData: IIC Write Timeout!\r\n");
		if (Status != XST_SUCCESS) {
			xil_printf("IicWriteData: IIC stop failed with status %x\r\n", Status);
		}
#endif
		return XST_FAILURE;
	}

	/*
	 * Stop Iic Device
	 */
	Status = XIic_Stop(&IicInstance);
	if (Status != XST_SUCCESS) {
#ifdef IIC_DEBUG
		xil_printf("IicWriteData: IIC Stop failed with status %x\r\n", Status);
#endif
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

