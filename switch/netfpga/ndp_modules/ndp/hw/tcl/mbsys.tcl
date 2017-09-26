#
# Copyright (c) 2015 Digilent Inc.
# Copyright (c) 2015 Tinghui Wang (Steve)
# All rights reserved.
#
#  File:
#        nf_sume_mbsys.tcl
#
#  Project:
#        acceptance_test
#
#  Author:
#        Tinghui Wang (Steve)
#
#  Description:
#        tcl function to create basic microblaze sub-system (mbsys) for
#        embedded design projects. Default bram size is 64KB.
#        useage:
#          create_hier_cell_mbsys <parentCell> <nameHier> <M_AXI_LITE_No>
#
# @NETFPGA_LICENSE_HEADER_START@
#
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


set help_msg "create_bd -name <bd_name> \[-iic <true\/false>\] \[-uart <true\/false>\] \[-add_ro <ro_width>\] \[-add_rw <rw_width>\]"

proc create_bd args {
	if { [llength $args] % 2 == 1 } {
		error "create_bd: Wrong # args."
		puts $help_msg
	}

	array set opts {
		-name "system"
		-iic  false
		-uart false
		-add_ro {}
		-add_rw {} 
	}

	foreach {opt value} [lrange $args 0 end] {
		if {![info exist opts($opt)]} {
			error "create_bd: unreconized option \"$opt\""
		}
		if {$opt != "-add_ro" && $opt != "-add_rw"} {
			set opts($opt) $value
		} else {
			set opts($opt) [lappend opts($opt) $value]
		}
	}

	# Create Block Diagram
	set design_name $opts(-name)
	set current_bd [create_bd_design $design_name]
	current_bd_design $design_name
	set parentCell [get_bd_cells /]
	set parentObj [get_bd_cells $parentCell]
	set parentType [get_property TYPE $parentObj]
	
	# Create Reset Ports
	set reset [ create_bd_port -dir I -type rst reset ]
	set_property -dict [ list CONFIG.POLARITY {ACTIVE_HIGH}  ] $reset

	# Create Sysclk Ports
	set sysclk [ create_bd_port -dir I -type clk sysclk ]
	set_property CONFIG.FREQ_HZ 100000000 $sysclk
  
	# Create instance: mbsys
	create_mbsys [current_bd_instance .] mbsys

	if { $opts(-iic) } {
		create_iic
	}
	
	if { $opts(-uart) } {
		create_uart
	}

	set index 0
	foreach ro_width $opts(-add_ro) {
		create_gpio_ro $index $ro_width 
		set index [expr $index + 1]
	}
	
	set index 0
	foreach rw_width $opts(-add_rw) {
		create_gpio_rw $index $rw_width 
		set index [expr $index + 1]
	}

	regenerate_bd_layout
	save_bd_design
	close_bd_design [current_bd_design]
}

proc create_mbsys { parentCell hierName} {
  # Create microblaze and apply automation
  create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze microblaze_0
  apply_bd_automation -rule xilinx.com:bd_rule:microblaze -config { \
						local_mem "64KB" \
						ecc "None" \
						cache "None" \
						debug_module "Debug Only" \
						axi_periph "Enabled" \
						axi_intc "1" \
						clk "New Clocking Wizard (100 MHz)" }\
					  [get_bd_cells microblaze_0]

  # Configure clock wizard
  set_property -dict [list\
	CONFIG.PRIM_IN_FREQ.VALUE_SRC USER]\
  [get_bd_cells clk_wiz_1]

  set_property -dict [list CONFIG.PRIM_SOURCE {No_buffer} CONFIG.PRIM_IN_FREQ {100.000}] [get_bd_cells clk_wiz_1]


  # Connect sysclk
  connect_bd_net [get_bd_pins sysclk] [get_bd_pins clk_wiz_1/clk_in1]

  # Connect reset
  connect_bd_net [get_bd_ports reset] [get_bd_pins clk_wiz_1/reset] [get_bd_pins rst_clk_wiz_1_100M/ext_reset_in]

  # Create Hierarchy
  group_bd_cells $hierName\
	[get_bd_cells microblaze_0_axi_intc]\
	[get_bd_cells mdm_1]\
	[get_bd_cells microblaze_0_xlconcat]\
	[get_bd_cells microblaze_0]\
	[get_bd_cells rst_clk_wiz_1_100M]\
	[get_bd_cells microblaze_0_axi_periph]\
	[get_bd_cells microblaze_0_local_memory]
}

proc create_iic {} {
  ## Create instance: axi_iic_0, and set properties
  set axi_iic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_iic axi_iic_0 ]
  set_property -dict [ list CONFIG.C_GPO_WIDTH {2} CONFIG.C_SCL_INERTIAL_DELAY {5} CONFIG.C_SDA_INERTIAL_DELAY {5}  ] $axi_iic_0
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/mbsys/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_iic_0/S_AXI]
  ## Create and connect to external ports
  set iic_fpga [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:iic_rtl:1.0 iic_fpga ]
  set iic_reset [ create_bd_port -dir O -from 1 -to 0 iic_reset ]
  connect_bd_intf_net [get_bd_intf_ports iic_fpga] [get_bd_intf_pins axi_iic_0/IIC]
  connect_bd_net [get_bd_ports iic_reset] [get_bd_pins axi_iic_0/gpo]
  ## Connect interrupts
  connect_bd_net [get_bd_pins axi_iic_0/iic2intc_irpt] [get_bd_pins mbsys/microblaze_0_xlconcat/In0]
}

proc create_uart {} {
  ## Create instance: axi_uartlite_0, and set properties
  set axi_uartlite_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uartlite axi_uartlite_0 ]
  set_property -dict [ list CONFIG.C_BAUDRATE {115200}  ] $axi_uartlite_0
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/mbsys/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_uartlite_0/S_AXI]
  ## Create and connect to external ports
  set uart [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:uart_rtl:1.0 uart ]
  connect_bd_intf_net [get_bd_intf_ports uart] [get_bd_intf_pins axi_uartlite_0/UART]
  ## Connect interrupts
  connect_bd_net [get_bd_pins axi_uartlite_0/interrupt] [get_bd_pins mbsys/microblaze_0_xlconcat/In1]
}

proc create_gpio_ro { index width } {
  set gpio_name axi_gpio_ro_$index
  ## Create instance: axi_gpio_ro, and set properties
  create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio $gpio_name
  set_property -dict [ list\
	CONFIG.C_IS_DUAL 0\
	CONFIG.C_ALL_INPUTS 1\
	CONFIG.C_ALL_OUTPUTS 0\
	CONFIG.C_GPIO_WIDTH $width ]\
  [get_bd_cells $gpio_name]

  apply_bd_automation -rule xilinx.com:bd_rule:axi4\
	-config {Master "/mbsys/microblaze_0 (Periph)" Clk "Auto" }\
	[get_bd_intf_pins $gpio_name/S_AXI]
  
  set gpio_in_$index [create_bd_port -dir I -from [expr $width - 1] -to 0 gpio_in_$index]
  connect_bd_net [get_bd_pins $gpio_name/gpio_io_i] [get_bd_ports gpio_in_$index]
}

proc create_gpio_rw { index width } {
  set gpio_name axi_gpio_rw_$index
  create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio $gpio_name 
  set_property -dict [ list\
	CONFIG.C_IS_DUAL 0\
	CONFIG.C_ALL_INPUTS 0\
	CONFIG.C_ALL_OUTPUTS 1\
	CONFIG.C_GPIO_WIDTH $width ]\
  [get_bd_cells $gpio_name]

  apply_bd_automation -rule xilinx.com:bd_rule:axi4\
	-config {Master "/mbsys/microblaze_0 (Periph)" Clk "Auto" }\
	[get_bd_intf_pins $gpio_name/S_AXI]
  
  set gpio_out_$index [create_bd_port -dir O -from [expr $width - 1] -to 0 gpio_out_$index]
  connect_bd_net [get_bd_pins $gpio_name/gpio_io_o] [get_bd_ports gpio_out_$index]
}

