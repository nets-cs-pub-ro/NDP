#
# Copyright (c) 2015 University of Cambridge
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

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2014.4
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   puts "ERROR: This script was created for Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."

   return 1
}

################################################################
# START
################################################################


# CHANGE DESIGN NAME HERE
set design_name control_sub


# CHECKING IF PROJECT EXISTS
if { [get_projects -quiet] eq "" } {
   puts "ERROR: Please open or create a project!"
   return 1
}


# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "ERROR: Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      puts "INFO: Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   puts "INFO: Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   puts "INFO: Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   puts "INFO: Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

puts "INFO: Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   puts $errMsg
   return $nRet
}

##################################################################
# DESIGN PROCs
##################################################################


# Hierarchical cell: microblaze_0_local_memory
proc create_hier_cell_microblaze_0_local_memory { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_microblaze_0_local_memory() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 DLMB
  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 ILMB

  # Create pins
  create_bd_pin -dir I -type clk LMB_Clk
  create_bd_pin -dir I -from 0 -to 0 -type rst LMB_Rst

  # Create instance: dlmb_bram_if_cntlr, and set properties
  set dlmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 dlmb_bram_if_cntlr ]
  set_property -dict [ list CONFIG.C_ECC {0}  ] $dlmb_bram_if_cntlr

  # Create instance: dlmb_v10, and set properties
  set dlmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 dlmb_v10 ]

  # Create instance: ilmb_bram_if_cntlr, and set properties
  set ilmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 ilmb_bram_if_cntlr ]
  set_property -dict [ list CONFIG.C_ECC {0}  ] $ilmb_bram_if_cntlr

  # Create instance: ilmb_v10, and set properties
  set ilmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 ilmb_v10 ]

  # Create instance: lmb_bram, and set properties
  set lmb_bram [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.2 lmb_bram ]
  set_property -dict [ list CONFIG.Memory_Type {True_Dual_Port_RAM} CONFIG.use_bram_block {BRAM_Controller}  ] $lmb_bram

  # Create interface connections
  connect_bd_intf_net -intf_net microblaze_0_dlmb [get_bd_intf_pins DLMB] [get_bd_intf_pins dlmb_v10/LMB_M]
  connect_bd_intf_net -intf_net microblaze_0_dlmb_bus [get_bd_intf_pins dlmb_bram_if_cntlr/SLMB] [get_bd_intf_pins dlmb_v10/LMB_Sl_0]
  connect_bd_intf_net -intf_net microblaze_0_dlmb_cntlr [get_bd_intf_pins dlmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTA]
  connect_bd_intf_net -intf_net microblaze_0_ilmb [get_bd_intf_pins ILMB] [get_bd_intf_pins ilmb_v10/LMB_M]
  connect_bd_intf_net -intf_net microblaze_0_ilmb_bus [get_bd_intf_pins ilmb_bram_if_cntlr/SLMB] [get_bd_intf_pins ilmb_v10/LMB_Sl_0]
  connect_bd_intf_net -intf_net microblaze_0_ilmb_cntlr [get_bd_intf_pins ilmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTB]

  # Create port connections
  connect_bd_net -net microblaze_0_Clk [get_bd_pins LMB_Clk] [get_bd_pins dlmb_bram_if_cntlr/LMB_Clk] [get_bd_pins dlmb_v10/LMB_Clk] [get_bd_pins ilmb_bram_if_cntlr/LMB_Clk] [get_bd_pins ilmb_v10/LMB_Clk]
  connect_bd_net -net microblaze_0_LMB_Rst [get_bd_pins LMB_Rst] [get_bd_pins dlmb_bram_if_cntlr/LMB_Rst] [get_bd_pins dlmb_v10/SYS_Rst] [get_bd_pins ilmb_bram_if_cntlr/LMB_Rst] [get_bd_pins ilmb_v10/SYS_Rst]
  
  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: mbsys
proc create_hier_cell_mbsys { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_mbsys() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M01_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M02_AXI

  # Create pins
  create_bd_pin -dir I -type clk Clk
  create_bd_pin -dir I -from 0 -to 0 In0
  create_bd_pin -dir I -from 0 -to 0 In1
  create_bd_pin -dir I dcm_locked
  create_bd_pin -dir I -type rst ext_reset_in
  create_bd_pin -dir O -from 0 -to 0 -type rst peripheral_aresetn

  # Create instance: mdm_1, and set properties
  set mdm_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:mdm:3.2 mdm_1 ]

  # Create instance: microblaze_0, and set properties
  set microblaze_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze:9.4 microblaze_0 ]
  set_property -dict [ list CONFIG.C_DEBUG_ENABLED {1} CONFIG.C_D_AXI {1} CONFIG.C_D_LMB {1} CONFIG.C_I_LMB {1}  ] $microblaze_0

  # Create instance: microblaze_0_axi_intc, and set properties
  set microblaze_0_axi_intc [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_intc:4.1 microblaze_0_axi_intc ]
  set_property -dict [ list CONFIG.C_HAS_FAST {1}  ] $microblaze_0_axi_intc

  # Create instance: microblaze_0_axi_periph, and set properties
  set microblaze_0_axi_periph [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 microblaze_0_axi_periph ]
  set_property -dict [ list CONFIG.NUM_MI {3}  ] $microblaze_0_axi_periph

  # Create instance: microblaze_0_local_memory
  create_hier_cell_microblaze_0_local_memory $hier_obj microblaze_0_local_memory

  # Create instance: microblaze_0_xlconcat, and set properties
  set microblaze_0_xlconcat [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 microblaze_0_xlconcat ]

  # Create instance: rst_clk_wiz_1_100M, and set properties
  set rst_clk_wiz_1_100M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_clk_wiz_1_100M ]

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins M01_AXI] [get_bd_intf_pins microblaze_0_axi_periph/M01_AXI]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins M02_AXI] [get_bd_intf_pins microblaze_0_axi_periph/M02_AXI]
  connect_bd_intf_net -intf_net microblaze_0_axi_dp [get_bd_intf_pins microblaze_0/M_AXI_DP] [get_bd_intf_pins microblaze_0_axi_periph/S00_AXI]
  connect_bd_intf_net -intf_net microblaze_0_debug [get_bd_intf_pins mdm_1/MBDEBUG_0] [get_bd_intf_pins microblaze_0/DEBUG]
  connect_bd_intf_net -intf_net microblaze_0_dlmb_1 [get_bd_intf_pins microblaze_0/DLMB] [get_bd_intf_pins microblaze_0_local_memory/DLMB]
  connect_bd_intf_net -intf_net microblaze_0_ilmb_1 [get_bd_intf_pins microblaze_0/ILMB] [get_bd_intf_pins microblaze_0_local_memory/ILMB]
  connect_bd_intf_net -intf_net microblaze_0_intc_axi [get_bd_intf_pins microblaze_0_axi_intc/s_axi] [get_bd_intf_pins microblaze_0_axi_periph/M00_AXI]
  connect_bd_intf_net -intf_net microblaze_0_interrupt [get_bd_intf_pins microblaze_0/INTERRUPT] [get_bd_intf_pins microblaze_0_axi_intc/interrupt]

  # Create port connections
  connect_bd_net -net In0_1 [get_bd_pins In0] [get_bd_pins microblaze_0_xlconcat/In0]
  connect_bd_net -net In1_1 [get_bd_pins In1] [get_bd_pins microblaze_0_xlconcat/In1]
  connect_bd_net -net clk_wiz_1_locked [get_bd_pins dcm_locked] [get_bd_pins rst_clk_wiz_1_100M/dcm_locked]
  connect_bd_net -net mdm_1_debug_sys_rst [get_bd_pins mdm_1/Debug_SYS_Rst] [get_bd_pins rst_clk_wiz_1_100M/mb_debug_sys_rst]
  connect_bd_net -net microblaze_0_Clk [get_bd_pins Clk] [get_bd_pins microblaze_0/Clk] [get_bd_pins microblaze_0_axi_intc/processor_clk] [get_bd_pins microblaze_0_axi_intc/s_axi_aclk] [get_bd_pins microblaze_0_axi_periph/ACLK] [get_bd_pins microblaze_0_axi_periph/M00_ACLK] [get_bd_pins microblaze_0_axi_periph/M01_ACLK] [get_bd_pins microblaze_0_axi_periph/M02_ACLK] [get_bd_pins microblaze_0_axi_periph/S00_ACLK] [get_bd_pins microblaze_0_local_memory/LMB_Clk] [get_bd_pins rst_clk_wiz_1_100M/slowest_sync_clk]
  connect_bd_net -net microblaze_0_intr [get_bd_pins microblaze_0_axi_intc/intr] [get_bd_pins microblaze_0_xlconcat/dout]
  connect_bd_net -net reset_1 [get_bd_pins ext_reset_in] [get_bd_pins rst_clk_wiz_1_100M/ext_reset_in]
  connect_bd_net -net rst_clk_wiz_1_100M_bus_struct_reset [get_bd_pins microblaze_0_local_memory/LMB_Rst] [get_bd_pins rst_clk_wiz_1_100M/bus_struct_reset]
  connect_bd_net -net rst_clk_wiz_1_100M_interconnect_aresetn [get_bd_pins microblaze_0_axi_periph/ARESETN] [get_bd_pins rst_clk_wiz_1_100M/interconnect_aresetn]
  connect_bd_net -net rst_clk_wiz_1_100M_mb_reset [get_bd_pins microblaze_0/Reset] [get_bd_pins microblaze_0_axi_intc/processor_rst] [get_bd_pins rst_clk_wiz_1_100M/mb_reset]
  connect_bd_net -net rst_clk_wiz_1_100M_peripheral_aresetn [get_bd_pins peripheral_aresetn] [get_bd_pins microblaze_0_axi_intc/s_axi_aresetn] [get_bd_pins microblaze_0_axi_periph/M00_ARESETN] [get_bd_pins microblaze_0_axi_periph/M01_ARESETN] [get_bd_pins microblaze_0_axi_periph/M02_ARESETN] [get_bd_pins microblaze_0_axi_periph/S00_ARESETN] [get_bd_pins rst_clk_wiz_1_100M/peripheral_aresetn]
  
  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: nf_mbsys
proc create_hier_cell_nf_mbsys { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_nf_mbsys() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:iic_rtl:1.0 iic_fpga
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:uart_rtl:1.0 uart

  # Create pins
  create_bd_pin -dir O -from 1 -to 0 iic_reset
  create_bd_pin -dir I -type rst reset
  create_bd_pin -dir I -type clk sysclk

  # Create instance: axi_iic_0, and set properties
  set axi_iic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_iic:2.0 axi_iic_0 ]
  set_property -dict [ list CONFIG.C_GPO_WIDTH {2} CONFIG.C_SCL_INERTIAL_DELAY {5} CONFIG.C_SDA_INERTIAL_DELAY {5}  ] $axi_iic_0

  # Create instance: axi_uartlite_0, and set properties
  set axi_uartlite_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uartlite:2.0 axi_uartlite_0 ]
  set_property -dict [ list CONFIG.C_BAUDRATE {115200}  ] $axi_uartlite_0

  # Create instance: clk_wiz_1, and set properties
  set clk_wiz_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:5.1 clk_wiz_1 ]
#  set_property -dict [ list CONFIG.PRIM_IN_FREQ {200.000} CONFIG.PRIM_SOURCE {No_buffer}  ] $clk_wiz_1
  
  # config 100MHz input clk
  set_property -dict [list CONFIG.PRIM_IN_FREQ {100.000} CONFIG.PRIM_SOURCE {No_buffer} \
  CONFIG.CLKIN1_JITTER_PS {100.0} CONFIG.MMCM_CLKFBOUT_MULT_F {10.000} \
  CONFIG.MMCM_CLKIN1_PERIOD {10.0} CONFIG.CLKOUT1_JITTER {130.958} \
  CONFIG.CLKOUT1_PHASE_ERROR {98.575}] $clk_wiz_1 
  

  # Create instance: mbsys
  create_hier_cell_mbsys $hier_obj mbsys

  # Create interface connections
  connect_bd_intf_net -intf_net axi_iic_0_IIC [get_bd_intf_pins iic_fpga] [get_bd_intf_pins axi_iic_0/IIC]
  connect_bd_intf_net -intf_net axi_uartlite_0_UART [get_bd_intf_pins uart] [get_bd_intf_pins axi_uartlite_0/UART]
  connect_bd_intf_net -intf_net mbsys_M01_AXI [get_bd_intf_pins axi_iic_0/S_AXI] [get_bd_intf_pins mbsys/M01_AXI]
  connect_bd_intf_net -intf_net mbsys_M02_AXI [get_bd_intf_pins axi_uartlite_0/S_AXI] [get_bd_intf_pins mbsys/M02_AXI]

  # Create port connections
  connect_bd_net -net axi_iic_0_gpo [get_bd_pins iic_reset] [get_bd_pins axi_iic_0/gpo]
  connect_bd_net -net axi_iic_0_iic2intc_irpt [get_bd_pins axi_iic_0/iic2intc_irpt] [get_bd_pins mbsys/In0]
  connect_bd_net -net axi_uartlite_0_interrupt [get_bd_pins axi_uartlite_0/interrupt] [get_bd_pins mbsys/In1]
  connect_bd_net -net clk_wiz_1_locked [get_bd_pins clk_wiz_1/locked] [get_bd_pins mbsys/dcm_locked]
  connect_bd_net -net mbsys_peripheral_aresetn [get_bd_pins axi_iic_0/s_axi_aresetn] [get_bd_pins axi_uartlite_0/s_axi_aresetn] [get_bd_pins mbsys/peripheral_aresetn]
  connect_bd_net -net microblaze_0_Clk [get_bd_pins axi_iic_0/s_axi_aclk] [get_bd_pins axi_uartlite_0/s_axi_aclk] [get_bd_pins clk_wiz_1/clk_out1] [get_bd_pins mbsys/Clk]
  connect_bd_net -net reset_1 [get_bd_pins reset] [get_bd_pins clk_wiz_1/reset] [get_bd_pins mbsys/ext_reset_in]
  connect_bd_net -net sysclk_1 [get_bd_pins sysclk] [get_bd_pins clk_wiz_1/clk_in1]
  
  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: dma_sub
proc create_hier_cell_dma_sub { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_dma_sub() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M00_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M01_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M02_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M03_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M04_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M05_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M06_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M07_AXI
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_dma_tx
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pcie_7x_mgt
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_dma_rx

  # Create pins
  create_bd_pin -dir I -type clk axi_lite_aclk
  create_bd_pin -dir I -type rst axi_lite_aresetn
  create_bd_pin -dir I -type clk axis_datapath_aclk
  create_bd_pin -dir I -type rst axis_datapath_aresetn
  create_bd_pin -dir I -type clk sys_clk
  create_bd_pin -dir I -type rst sys_reset
  
  create_bd_pin -dir I -type clk M00_ACLK
  create_bd_pin -dir I -type rst M00_ARESETN
  create_bd_pin -dir I -type clk M01_ACLK
  create_bd_pin -dir I -type rst M01_ARESETN
  create_bd_pin -dir I -type clk M02_ACLK
  create_bd_pin -dir I -type rst M02_ARESETN
  create_bd_pin -dir I -type clk M03_ACLK
  create_bd_pin -dir I -type rst M03_ARESETN
  create_bd_pin -dir I -type clk M04_ACLK
  create_bd_pin -dir I -type rst M04_ARESETN
  create_bd_pin -dir I -type clk M05_ACLK
  create_bd_pin -dir I -type rst M05_ARESETN
  create_bd_pin -dir I -type clk M06_ACLK
  create_bd_pin -dir I -type rst M06_ARESETN
  create_bd_pin -dir I -type clk M07_ACLK
  create_bd_pin -dir I -type rst M07_ARESETN
  
  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list CONFIG.NUM_MI {9}  ] $axi_interconnect_0
  set_property -dict [list CONFIG.M00_HAS_REGSLICE {3} CONFIG.M00_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M01_HAS_REGSLICE {3} CONFIG.M01_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M02_HAS_REGSLICE {3} CONFIG.M02_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M03_HAS_REGSLICE {3} CONFIG.M03_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M04_HAS_REGSLICE {3} CONFIG.M04_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M05_HAS_REGSLICE {3} CONFIG.M05_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M06_HAS_REGSLICE {3} CONFIG.M06_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.M07_HAS_REGSLICE {3} CONFIG.M07_HAS_DATA_FIFO {1} ] $axi_interconnect_0              
  set_property -dict [list CONFIG.M08_HAS_REGSLICE {3} CONFIG.M08_HAS_DATA_FIFO {1} ] $axi_interconnect_0
  set_property -dict [list CONFIG.S00_HAS_REGSLICE {3} CONFIG.S00_HAS_DATA_FIFO {1} ] $axi_interconnect_0

  # AXIS: clock domain crossing FIFO, TX (PCIe->FPGA) user_fifo_reset (user_clk)
  set pcie_reset_inv [create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic pcie_reset_inv]
  set_property -dict [list CONFIG.C_SIZE {1} CONFIG.C_OPERATION {not}] [get_bd_cells pcie_reset_inv]

  # Create instance: axis_dwidth_converter
  set axis_dwidth_dma_tx [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 axis_dwidth_dma_tx]
  set_property -dict [list CONFIG.HAS_TKEEP.VALUE_SRC USER CONFIG.HAS_TLAST.VALUE_SRC USER \
           CONFIG.HAS_TSTRB.VALUE_SRC USER CONFIG.S_TDATA_NUM_BYTES.VALUE_SRC USER \
	   CONFIG.TUSER_BITS_PER_BYTE.VALUE_SRC USER] $axis_dwidth_dma_tx

  set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {16} CONFIG.M_TDATA_NUM_BYTES {32} \
	   CONFIG.TUSER_BITS_PER_BYTE {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TSTRB {0} \
	   CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1}] $axis_dwidth_dma_tx



  set axis_dwidth_dma_rx [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 axis_dwidth_dma_rx]	    
 
  set_property -dict [list CONFIG.HAS_TKEEP.VALUE_SRC USER CONFIG.HAS_TLAST.VALUE_SRC USER \
             CONFIG.HAS_TSTRB.VALUE_SRC USER CONFIG.S_TDATA_NUM_BYTES.VALUE_SRC USER \
             CONFIG.TUSER_BITS_PER_BYTE.VALUE_SRC USER] $axis_dwidth_dma_rx

  set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {32} CONFIG.M_TDATA_NUM_BYTES {16} \
             CONFIG.TUSER_BITS_PER_BYTE {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TSTRB {0} \
	     CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1}] $axis_dwidth_dma_rx

 # Create instance: axis_fifo_10g_rx, and set properties
 set axis_fifo_10g_rx [create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 axis_fifo_10g_rx]
 set_property -dict [list CONFIG.TDATA_NUM_BYTES {16} CONFIG.TUSER_WIDTH {128} CONFIG.IS_ACLK_ASYNC {1} CONFIG.FIFO_DEPTH {32}] $axis_fifo_10g_rx

  # Create instance: axis_fifo_10g_tx, and set properties
 set axis_fifo_10g_tx [create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 axis_fifo_10g_tx]
 set_property -dict [list CONFIG.TDATA_NUM_BYTES {16} CONFIG.TUSER_WIDTH {128} CONFIG.IS_ACLK_ASYNC {1} CONFIG.FIFO_DEPTH {32}] $axis_fifo_10g_tx	 
  
  # Create instance: nf_riffa_dma_1, and set properties
  set nf_riffa_dma_1 [ create_bd_cell -type ip -vlnv NetFPGA:NetFPGA:nf_riffa_dma:1.0 nf_riffa_dma_1 ]
  
  # Create instance: axi_clock_converter_0, and set properties
  set axi_clock_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_0 ] 

  # Create instance: pcie3_7x_1, and set properties
  set pcie3_7x_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:pcie3_7x:3.0 pcie3_7x_1 ]
  set_property -dict [ list CONFIG.PF0_DEVICE_ID {7028} \
CONFIG.PF0_INTERRUPT_PIN {NONE} CONFIG.PF1_DEVICE_ID {7011} \
CONFIG.PL_LINK_CAP_MAX_LINK_SPEED {5.0_GT/s} CONFIG.PL_LINK_CAP_MAX_LINK_WIDTH {X8} \
CONFIG.axisten_freq {250} CONFIG.axisten_if_enable_client_tag {false} \
CONFIG.axisten_if_width {128_bit} CONFIG.cfg_ctl_if {false} \
CONFIG.cfg_ext_if {false} CONFIG.cfg_mgmt_if {false} \
CONFIG.cfg_tx_msg_if {false} CONFIG.en_ext_clk {false} \
CONFIG.extended_tag_field {true} CONFIG.gen_x0y0 {false} \
CONFIG.mode_selection {Advanced} CONFIG.pcie_blk_locn {X0Y1} \
CONFIG.per_func_status_if {false} CONFIG.pf0_bar0_size {1} \
CONFIG.pf0_dev_cap_max_payload {128_bytes} CONFIG.rcv_msg_if {false} \
CONFIG.tx_fc_if {false} CONFIG.xlnx_ref_board {None} \
 ] $pcie3_7x_1

  # Create interface connections
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins M00_AXI] [get_bd_intf_pins axi_interconnect_0/M00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M01_AXI [get_bd_intf_pins M01_AXI] [get_bd_intf_pins axi_interconnect_0/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M02_AXI [get_bd_intf_pins M02_AXI] [get_bd_intf_pins axi_interconnect_0/M02_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M03_AXI [get_bd_intf_pins M03_AXI] [get_bd_intf_pins axi_interconnect_0/M03_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M04_AXI [get_bd_intf_pins M04_AXI] [get_bd_intf_pins axi_interconnect_0/M04_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M05_AXI [get_bd_intf_pins M05_AXI] [get_bd_intf_pins axi_interconnect_0/M05_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M06_AXI [get_bd_intf_pins M06_AXI] [get_bd_intf_pins axi_interconnect_0/M06_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M07_AXI [get_bd_intf_pins M07_AXI] [get_bd_intf_pins axi_interconnect_0/M07_AXI]

  connect_bd_intf_net -intf_net nf_riffa_dma_1_s_axis_dma_rx [get_bd_intf_pins s_axis_dma_rx] [get_bd_intf_pins axis_dwidth_dma_rx/S_AXIS]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_fifo_dwidth_rx [get_bd_intf_pins axis_fifo_10g_rx/S_AXIS] [get_bd_intf_pins axis_dwidth_dma_rx/M_AXIS]
  connect_bd_intf_net -intf_net axis_fifo_10g_rx_M_AXIS [get_bd_intf_pins axis_fifo_10g_rx/M_AXIS] [get_bd_intf_pins nf_riffa_dma_1/s_axis_xge_rx]


  connect_bd_intf_net -intf_net nf_riffa_dma_1_m_axis_dma_tx [get_bd_intf_pins m_axis_dma_tx] [get_bd_intf_pins axis_dwidth_dma_tx/M_AXIS]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_fifo_dwidth_tx [get_bd_intf_pins axis_fifo_10g_tx/M_AXIS] [get_bd_intf_pins axis_dwidth_dma_tx/S_AXIS]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_dwidth_conv_tx [get_bd_intf_pins axis_fifo_10g_tx/S_AXIS] [get_bd_intf_pins nf_riffa_dma_1/m_axis_xge_tx]
	
 

 connect_bd_intf_net -intf_net nf_riffa_dma_1_pcie3_cfg_interrupt [get_bd_intf_pins nf_riffa_dma_1/cfg_interrupt] [get_bd_intf_pins pcie3_7x_1/pcie3_cfg_interrupt]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_pcie3_cfg_msi [get_bd_intf_pins nf_riffa_dma_1/cfg_interrupt_msi] [get_bd_intf_pins pcie3_7x_1/pcie3_cfg_msi]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_pcie3_cfg_status [get_bd_intf_pins nf_riffa_dma_1/cfg] [get_bd_intf_pins pcie3_7x_1/pcie3_cfg_status]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_pcie_cfg_fc [get_bd_intf_pins nf_riffa_dma_1/cfg_fc] [get_bd_intf_pins pcie3_7x_1/pcie_cfg_fc]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_s_axis_cc [get_bd_intf_pins nf_riffa_dma_1/s_axis_cc] [get_bd_intf_pins pcie3_7x_1/s_axis_cc]
  connect_bd_intf_net -intf_net nf_riffa_dma_1_s_axis_rq [get_bd_intf_pins nf_riffa_dma_1/s_axis_rq] [get_bd_intf_pins pcie3_7x_1/s_axis_rq]
  connect_bd_intf_net -intf_net pcie3_7x_1_m_axis_cq [get_bd_intf_pins nf_riffa_dma_1/m_axis_cq] [get_bd_intf_pins pcie3_7x_1/m_axis_cq]
  connect_bd_intf_net -intf_net pcie3_7x_1_m_axis_rc [get_bd_intf_pins nf_riffa_dma_1/m_axis_rc] [get_bd_intf_pins pcie3_7x_1/m_axis_rc]
  connect_bd_intf_net -intf_net pcie3_7x_1_pcie_7x_mgt [get_bd_intf_pins pcie_7x_mgt] [get_bd_intf_pins pcie3_7x_1/pcie_7x_mgt]
  connect_bd_intf_net -intf_net s00_axi_1 [get_bd_intf_pins axi_interconnect_0/S00_AXI] [get_bd_intf_pins nf_riffa_dma_1/m_axi_lite]
  
  #Clock converter connections
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/M_AXI] [get_bd_intf_pins nf_riffa_dma_1/s_axi_lite]
  connect_bd_intf_net -intf_net axi_interconnect_0_M08_AXI [get_bd_intf_pins axi_clock_converter_0/S_AXI] [get_bd_intf_pins axi_interconnect_0/M08_AXI]
set_property -dict [ list CONFIG.FREQ_HZ {250000000}  ] [get_bd_intf_pins nf_riffa_dma_1/s_axi_lite]

  

  # Create port connections
  connect_bd_net -net axi_lite_clk_1 [get_bd_pins axi_lite_aclk]  [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins nf_riffa_dma_1/m_axi_lite_aclk]


connect_bd_net -net M00_ACLK_i [get_bd_pins M00_ACLK] [get_bd_pins axi_interconnect_0/M00_ACLK] 
connect_bd_net -net M01_ACLK_i [get_bd_pins M01_ACLK] [get_bd_pins axi_interconnect_0/M01_ACLK] 
connect_bd_net -net M02_ACLK_i [get_bd_pins M02_ACLK] [get_bd_pins axi_interconnect_0/M02_ACLK]
connect_bd_net -net M03_ACLK_i [get_bd_pins M03_ACLK] [get_bd_pins axi_interconnect_0/M03_ACLK]
connect_bd_net -net M04_ACLK_i [get_bd_pins M04_ACLK] [get_bd_pins axi_interconnect_0/M04_ACLK] 
connect_bd_net -net M05_ACLK_i [get_bd_pins M05_ACLK] [get_bd_pins axi_interconnect_0/M05_ACLK] 
connect_bd_net -net M06_ACLK_i [get_bd_pins M06_ACLK] [get_bd_pins axi_interconnect_0/M06_ACLK] 
connect_bd_net -net M07_ACLK_i [get_bd_pins M07_ACLK] [get_bd_pins axi_interconnect_0/M07_ACLK] 

  connect_bd_net -net axi_lite_rstn_1  [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins axi_lite_aresetn] [get_bd_pins nf_riffa_dma_1/m_axi_lite_aresetn]


connect_bd_net -net M00_ARESETN_i [get_bd_pins M00_ARESETN] [get_bd_pins axi_interconnect_0/M00_ARESETN] 
connect_bd_net -net M01_ARESETN_i [get_bd_pins M01_ARESETN]  [get_bd_pins axi_interconnect_0/M01_ARESETN]
connect_bd_net -net M02_ARESETN_i [get_bd_pins M02_ARESETN]  [get_bd_pins axi_interconnect_0/M02_ARESETN]
connect_bd_net -net M03_ARESETN_i [get_bd_pins M03_ARESETN]  [get_bd_pins axi_interconnect_0/M03_ARESETN]
connect_bd_net -net M04_ARESETN_i [get_bd_pins M04_ARESETN]  [get_bd_pins axi_interconnect_0/M04_ARESETN]
connect_bd_net -net M05_ARESETN_i [get_bd_pins M05_ARESETN]  [get_bd_pins axi_interconnect_0/M05_ARESETN]
connect_bd_net -net M06_ARESETN_i [get_bd_pins M06_ARESETN]  [get_bd_pins axi_interconnect_0/M06_ARESETN] 
connect_bd_net -net M07_ARESETN_i [get_bd_pins M07_ARESETN]  [get_bd_pins axi_interconnect_0/M07_ARESETN]

  connect_bd_net -net axis_10g_clk_1 [get_bd_pins axis_datapath_aclk] [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/M08_ACLK] [get_bd_pins axis_dwidth_dma_rx/aclk] [get_bd_pins axis_dwidth_dma_tx/aclk] [get_bd_pins axis_fifo_10g_rx/s_axis_aclk] [get_bd_pins axis_fifo_10g_tx/m_axis_aclk]
  
  connect_bd_net -net axis_rx_sys_reset_0_peripheral_aresetn [get_bd_pins axis_datapath_aresetn] [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/M08_ARESETN] [get_bd_pins axis_dwidth_dma_rx/aresetn] [get_bd_pins axis_dwidth_dma_tx/aresetn] [get_bd_pins axis_fifo_10g_rx/s_axis_aresetn] [get_bd_pins axis_fifo_10g_tx/m_axis_aresetn]
  
  connect_bd_net -net axis_tx_sys_reset_0_peripheral_aresetn [get_bd_pins axi_clock_converter_0/m_axi_aresetn] [get_bd_pins axis_fifo_10g_rx/m_axis_aresetn] [get_bd_pins axis_fifo_10g_tx/s_axis_aresetn] [get_bd_pins pcie_reset_inv/Res]

  connect_bd_net -net pcie3_7x_1_user_clk [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axis_fifo_10g_rx/m_axis_aclk] [get_bd_pins axis_fifo_10g_tx/s_axis_aclk] [get_bd_pins nf_riffa_dma_1/user_clk] [get_bd_pins pcie3_7x_1/user_clk]
  
  connect_bd_net -net pcie3_7x_1_user_lnk_up [get_bd_pins nf_riffa_dma_1/user_lnk_up] [get_bd_pins pcie3_7x_1/user_lnk_up]
  connect_bd_net -net pcie3_7x_1_user_reset [get_bd_pins pcie_reset_inv/Op1] [get_bd_pins nf_riffa_dma_1/user_reset] [get_bd_pins pcie3_7x_1/user_reset]
  connect_bd_net -net sys_clk_1 [get_bd_pins sys_clk] [get_bd_pins pcie3_7x_1/sys_clk]
  connect_bd_net -net sys_reset_1 [get_bd_pins sys_reset] [get_bd_pins pcie3_7x_1/sys_reset]
  
  # Restore current instance
  current_bd_instance $oldCurInst
}


# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set M00_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M00_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M00_AXI
  set M01_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M01_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M01_AXI
  set M02_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M02_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M02_AXI
  set M03_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M03_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M03_AXI
  set M04_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M04_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M04_AXI
  set M05_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M05_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M05_AXI
  set M06_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M06_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M06_AXI
  set M07_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M07_AXI ]
  set_property -dict [ list CONFIG.ADDR_WIDTH {12} CONFIG.DATA_WIDTH {32} CONFIG.PROTOCOL {AXI4LITE}  ] $M07_AXI
  set iic_fpga [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:iic_rtl:1.0 iic_fpga ]
  set m_axis_dma_tx [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_dma_tx ]
  set pcie_7x_mgt [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pcie_7x_mgt ]
  set s_axis_dma_rx [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_dma_rx ]
  set_property -dict [ list CONFIG.FREQ_HZ {100000000} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_TREADY {1} CONFIG.HAS_TSTRB {0} CONFIG.LAYERED_METADATA {undef} CONFIG.PHASE {0.000} CONFIG.TDATA_NUM_BYTES {32} CONFIG.TDEST_WIDTH {0} CONFIG.TID_WIDTH {0} CONFIG.TUSER_WIDTH {128}  ] $s_axis_dma_rx
  set uart [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:uart_rtl:1.0 uart ]

  # Create ports
  set axi_lite_aclk [ create_bd_port -dir I -type clk axi_lite_aclk ]
  set axi_lite_aresetn [ create_bd_port -dir I -type rst axi_lite_aresetn ]
  set_property -dict [ list CONFIG.POLARITY {ACTIVE_LOW}] $axi_lite_aresetn 
  set axis_datapath_aclk [ create_bd_port -dir I -type clk axis_datapath_aclk ]
  set axis_datapath_aresetn [ create_bd_port -dir I -type rst axis_datapath_aresetn ]
  set_property -dict [ list CONFIG.POLARITY {ACTIVE_LOW}  ] $axis_datapath_aresetn
  set iic_reset [ create_bd_port -dir O -from 1 -to 0 iic_reset ]
  set sys_clk [ create_bd_port -dir I -type clk sys_clk ]
  set_property -dict [ list CONFIG.FREQ_HZ {100000000}  ] $sys_clk
  set sys_reset [ create_bd_port -dir I -type rst sys_reset ]
  set_property -dict [ list CONFIG.POLARITY {ACTIVE_HIGH}  ] $sys_reset



  # Create instance: dma_sub
  create_hier_cell_dma_sub [current_bd_instance .] dma_sub

  # Create instance: nf_mbsys
  create_hier_cell_nf_mbsys [current_bd_instance .] nf_mbsys

  # Create interface connections
  connect_bd_intf_net -intf_net dma_sub_M00_AXI [get_bd_intf_ports M00_AXI] [get_bd_intf_pins dma_sub/M00_AXI]
  connect_bd_intf_net -intf_net dma_sub_M01_AXI [get_bd_intf_ports M01_AXI] [get_bd_intf_pins dma_sub/M01_AXI]
  connect_bd_intf_net -intf_net dma_sub_M02_AXI [get_bd_intf_ports M02_AXI] [get_bd_intf_pins dma_sub/M02_AXI]
  connect_bd_intf_net -intf_net dma_sub_M03_AXI [get_bd_intf_ports M03_AXI] [get_bd_intf_pins dma_sub/M03_AXI]
  connect_bd_intf_net -intf_net dma_sub_M04_AXI [get_bd_intf_ports M04_AXI] [get_bd_intf_pins dma_sub/M04_AXI]
  connect_bd_intf_net -intf_net dma_sub_M05_AXI [get_bd_intf_ports M05_AXI] [get_bd_intf_pins dma_sub/M05_AXI]
  connect_bd_intf_net -intf_net dma_sub_M06_AXI [get_bd_intf_ports M06_AXI] [get_bd_intf_pins dma_sub/M06_AXI]
  connect_bd_intf_net -intf_net dma_sub_M07_AXI [get_bd_intf_ports M07_AXI] [get_bd_intf_pins dma_sub/M07_AXI]
  connect_bd_intf_net -intf_net dma_sub_m_axis_dma_tx [get_bd_intf_ports m_axis_dma_tx] [get_bd_intf_pins dma_sub/m_axis_dma_tx]
  connect_bd_intf_net -intf_net dma_sub_pcie_7x_mgt [get_bd_intf_ports pcie_7x_mgt] [get_bd_intf_pins dma_sub/pcie_7x_mgt]
  connect_bd_intf_net -intf_net nf_mbsys_iic_fpga [get_bd_intf_ports iic_fpga] [get_bd_intf_pins nf_mbsys/iic_fpga]
  connect_bd_intf_net -intf_net nf_mbsys_uart [get_bd_intf_ports uart] [get_bd_intf_pins nf_mbsys/uart]
  connect_bd_intf_net -intf_net s_axis_dma_rx_1 [get_bd_intf_ports s_axis_dma_rx] [get_bd_intf_pins dma_sub/s_axis_dma_rx]

  # Create port connections
  connect_bd_net -net axi_lite_aclk_1 [get_bd_ports axi_lite_aclk] [get_bd_pins dma_sub/axi_lite_aclk]
  connect_bd_net -net axi_lite_aresetn_1 [get_bd_ports axi_lite_aresetn] [get_bd_pins dma_sub/axi_lite_aresetn]
  connect_bd_net -net axis_datapath_aclk_1 [get_bd_ports axis_datapath_aclk] [get_bd_pins dma_sub/axis_datapath_aclk] [get_bd_pins dma_sub/M00_ACLK] [get_bd_pins dma_sub/M01_ACLK] [get_bd_pins dma_sub/M02_ACLK] [get_bd_pins dma_sub/M03_ACLK] [get_bd_pins dma_sub/M04_ACLK] [get_bd_pins dma_sub/M05_ACLK] [get_bd_pins dma_sub/M06_ACLK] [get_bd_pins dma_sub/M07_ACLK] 
  connect_bd_net -net axis_datapath_aresetn_1 [get_bd_ports axis_datapath_aresetn] [get_bd_pins dma_sub/axis_datapath_aresetn]  [get_bd_pins dma_sub/M00_ARESETN] [get_bd_pins dma_sub/M01_ARESETN] [get_bd_pins dma_sub/M02_ARESETN] [get_bd_pins dma_sub/M03_ARESETN] [get_bd_pins dma_sub/M04_ARESETN] [get_bd_pins dma_sub/M05_ARESETN] [get_bd_pins dma_sub/M06_ARESETN] [get_bd_pins dma_sub/M07_ARESETN]
  connect_bd_net -net nf_mbsys_iic_reset [get_bd_ports iic_reset] [get_bd_pins nf_mbsys/iic_reset]
  connect_bd_net -net sys_clk_1 [get_bd_ports sys_clk] [get_bd_pins dma_sub/sys_clk] [get_bd_pins nf_mbsys/sysclk]
  connect_bd_net -net sys_reset_1 [get_bd_ports sys_reset] [get_bd_pins dma_sub/sys_reset] [get_bd_pins nf_mbsys/reset]


  # Create address segments
  source ./tcl/$::env(NF_PROJECT_NAME)_defines.tcl
  create_bd_addr_seg -range $M00_SIZEADDR -offset $M00_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M00_AXI/Reg] SEG_M00_AXI_Reg
  create_bd_addr_seg -range $M01_SIZEADDR -offset $M01_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M01_AXI/Reg] SEG_M01_AXI_Reg
  create_bd_addr_seg -range $M02_SIZEADDR -offset $M02_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M02_AXI/Reg] SEG_M02_AXI_Reg
  create_bd_addr_seg -range $M03_SIZEADDR -offset $M03_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M03_AXI/Reg] SEG_M03_AXI_Reg
  create_bd_addr_seg -range $M04_SIZEADDR -offset $M04_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M04_AXI/Reg] SEG_M04_AXI_Reg
  create_bd_addr_seg -range $M05_SIZEADDR -offset $M05_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M05_AXI/Reg] SEG_M05_AXI_Reg
  create_bd_addr_seg -range $M06_SIZEADDR -offset $M06_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M06_AXI/Reg] SEG_M06_AXI_Reg
  create_bd_addr_seg -range $M07_SIZEADDR -offset $M07_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs M07_AXI/Reg] SEG_M07_AXI_Reg
  create_bd_addr_seg -range $M08_SIZEADDR -offset $M08_BASEADDR [get_bd_addr_spaces dma_sub/nf_riffa_dma_1/m_axi_lite] [get_bd_addr_segs  dma_sub/nf_riffa_dma_1/s_axi_lite/reg0] SEG_nf_riffa_dma_1_reg0
  
  create_bd_addr_seg -range $MICROBLAZE_AXI_IIC_SIZEADDR -offset $MICROBLAZE_AXI_IIC_BASEADDR [get_bd_addr_spaces nf_mbsys/mbsys/microblaze_0/Data] [get_bd_addr_segs nf_mbsys/axi_iic_0/S_AXI/Reg] SEG_axi_iic_0_Reg
  create_bd_addr_seg -range $MICROBLAZE_UARTLITE_SIZEADDR -offset $MICROBLAZE_UARTLITE_BASEADDR [get_bd_addr_spaces nf_mbsys/mbsys/microblaze_0/Data] [get_bd_addr_segs nf_mbsys/axi_uartlite_0/S_AXI/Reg] SEG_axi_uartlite_0_Reg
  create_bd_addr_seg -range $MICROBLAZE_DLMB_BRAM_SIZEADDR -offset $MICROBLAZE_DLMB_BRAM_BASEADDR [get_bd_addr_spaces nf_mbsys/mbsys/microblaze_0/Data] [get_bd_addr_segs nf_mbsys/mbsys/microblaze_0_local_memory/dlmb_bram_if_cntlr/SLMB/Mem] SEG_dlmb_bram_if_cntlr_Mem
  create_bd_addr_seg -range $MICROBLAZE_ILMB_BRAM_SIZEADDR -offset $MICROBLAZE_ILMB_BRAM_BASEADDR [get_bd_addr_spaces nf_mbsys/mbsys/microblaze_0/Instruction] [get_bd_addr_segs nf_mbsys/mbsys/microblaze_0_local_memory/ilmb_bram_if_cntlr/SLMB/Mem] SEG_ilmb_bram_if_cntlr_Mem
  create_bd_addr_seg -range $MICROBLAZE_AXI_INTC_SIZEADDR -offset $MICROBLAZE_AXI_INTC_BASEADDR [get_bd_addr_spaces nf_mbsys/mbsys/microblaze_0/Data] [get_bd_addr_segs nf_mbsys/mbsys/microblaze_0_axi_intc/s_axi/Reg] SEG_microblaze_0_axi_intc_Reg
  

  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


