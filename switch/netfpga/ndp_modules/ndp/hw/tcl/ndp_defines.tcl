#
# Copyright (c) 2015 Noa Zilberman
# All rights reserved.
#
# This software was developed by Stanford University and the University of Cambridge Computer Laboratory 
# under National Science Foundation under Grant No. CNS-0855268,
# the University of Cambridge Computer Laboratory under EPSRC INTERNET Project EP/H040536/1 and
# by the University of Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-11-C-0249 ("MRC2"), 
# as part of the DARPA MRC research programme.
#
# @NETFPGA_LICENSE_HEADER_START@
#
# Licensed to NetFPGA C.I.C. (NetFPGA) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  NetFPGA licenses this
# file to you under the NetFPGA Hardware-Software License, Version 1.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at:
#
#   http://www.netfpga-cic.org
#
# Unless required by applicable law or agreed to in writing, Work distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations under the License.
#
# @NETFPGA_LICENSE_HEADER_END@
#


######################
#MICROBLAZE Section  #
######################
# MICROBLAZE_AXI_IIC base address and size
set MICROBLAZE_AXI_IIC_BASEADDR 0x40800000
set MICROBLAZE_AXI_IIC_HIGHADDR 0x4080FFFF
set MICROBLAZE_AXI_IIC_SIZEADDR 0x10000

# MICROBLAZE_UARTLITE base address and size
set MICROBLAZE_UARTLITE_BASEADDR 0x40600000
set MICROBLAZE_UARTLITE_HIGHADDR 0x4060FFFF
set MICROBLAZE_UARTLITE_SIZEADDR 0x10000

# MICROBLAZE_DLMB_BRAM base address and size
set MICROBLAZE_DLMB_BRAM_BASEADDR 0x00000000
set MICROBLAZE_DLMB_BRAM_HIGHADDR 0x0000FFFF
set MICROBLAZE_DLMB_BRAM_SIZEADDR 0x10000

# MICROBLAZE_UARTLITE base address and size
set MICROBLAZE_ILMB_BRAM_BASEADDR 0x00000000
set MICROBLAZE_ILMB_BRAM_HIGHADDR 0x0000FFFF
set MICROBLAZE_ILMB_BRAM_SIZEADDR 0x10000

# MICROBLAZE_AXI_INTC base address and size
set MICROBLAZE_AXI_INTC_BASEADDR 0x41200000
set MICROBLAZE_AXI_INTC_HIGHADDR 0x4120FFFF
set MICROBLAZE_AXI_INTC_SIZEADDR 0x10000


#######################
# Segments Assignment #
#######################
#M00 
set M00_BASEADDR 0x44000000
set M00_HIGHADDR 0x44000FFF
set M00_SIZEADDR 0x1000

#M01 
set M01_BASEADDR 0x44010000
set M01_HIGHADDR 0x44010FFF
set M01_SIZEADDR 0x1000

#M02 
set M02_BASEADDR 0x44020000
set M02_HIGHADDR 0x44020FFF
set M02_SIZEADDR 0x1000

#M03 
set M03_BASEADDR 0x44030000
set M03_HIGHADDR 0x44030FFF
set M03_SIZEADDR 0x1000

#M04 
set M04_BASEADDR 0x44040000
set M04_HIGHADDR 0x44040FFF
set M04_SIZEADDR 0x1000

#M05 
set M05_BASEADDR 0x44050000
set M05_HIGHADDR 0x44050FFF
set M05_SIZEADDR 0x1000

#M06 
set M06_BASEADDR 0x44060000
set M06_HIGHADDR 0x44060FFF
set M06_SIZEADDR 0x1000

#M07 
set M07_BASEADDR 0x44070000
set M07_HIGHADDR 0x44070FFF
set M07_SIZEADDR 0x1000

#M08
set M08_BASEADDR 0x44080000
set M08_HIGHADDR 0x44080FFF
set M08_SIZEADDR 0x1000



#######################
# IP_ASSIGNMENT       #
#######################
# Note that physical connectivity must match this mapping

#IDENTIFIER base address and size
set IDENTIFIER_BASEADDR $M00_BASEADDR
set IDENTIFIER_HIGHADDR $M00_HIGHADDR
set IDENTIFIER_SIZEADDR $M00_SIZEADDR


#INPUT DRR ARBITER base address and size
set INPUT_DRR_ARBITER_BASEADDR $M01_BASEADDR
set INPUT_DRR_ARBITER_HIGHADDR $M01_HIGHADDR
set INPUT_DRR_ARBITER_SIZEADDR $M01_SIZEADDR

#OUTPUT_QUEUES base address and size
set OUTPUT_QUEUES_BASEADDR $M03_BASEADDR
set OUTPUT_QUEUES_HIGHADDR $M03_HIGHADDR
set OUTPUT_QUEUES_SIZEADDR $M03_SIZEADDR

#OUPUT_PORT_LOOKUP base address and size
set OUTPUT_PORT_LOOKUP_BASEADDR $M02_BASEADDR
set OUTPUT_PORT_LOOKUP_HIGHADDR $M02_HIGHADDR
set OUTPUT_PORT_LOOKUP_SIZEADDR $M02_SIZEADDR

#SFP Port 0 base address and size
set NF_10G_INTERFACE0_BASEADDR $M04_BASEADDR
set NF_10G_INTERFACE0_HIGHADDR $M04_HIGHADDR
set NF_10G_INTERFACE0_SIZEADDR $M04_SIZEADDR

#SFP Port 1 base address and size
set NF_10G_INTERFACE1_BASEADDR $M05_BASEADDR
set NF_10G_INTERFACE1_HIGHADDR $M05_HIGHADDR
set NF_10G_INTERFACE1_SIZEADDR $M05_SIZEADDR

#SFP Port 2 base address and size
set NF_10G_INTERFACE2_BASEADDR $M06_BASEADDR
set NF_10G_INTERFACE2_HIGHADDR $M06_HIGHADDR
set NF_10G_INTERFACE2_SIZEADDR $M06_SIZEADDR

#SFP Port 3 base address and size
set NF_10G_INTERFACE3_BASEADDR $M07_BASEADDR
set NF_10G_INTERFACE3_HIGHADDR $M07_HIGHADDR
set NF_10G_INTERFACE3_SIZEADDR $M07_SIZEADDR

#RIFFA base address and size
set NF_RIFFA_DMA_BASEADDR $M08_BASEADDR
set NF_RIFFA_DMA_HIGHADDR $M08_HIGHADDR
set NF_RIFFA_DMA_SIZEADDR $M08_SIZEADDR



