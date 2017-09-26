#
# Copyright (c) 2015 Noa Zilberman, Jingyun Zhang
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

# The following list include all the items that are mapped to memory segments
# The structure of each item is as follows {<Prefix name> <ID> <has registers> <library name>}

set DEF_LIST {
	{MICROBLAZE_AXI_IIC 0 0 ""} \
	{MICROBLAZE_UARTLITE 0 0 ""} \
	{MICROBLAZE_DLMB_BRAM 0 0 ""} \
	{MICROBLAZE_ILMB_BRAM 0 0 ""} \
	{MICROBLAZE_AXI_INTC 0 0 ""} \
	{INPUT_DRR_ARBITER 0 1 input_drr_arbiter_v1_0_0/data/input_drr_arbiter_regs_defines.txt} \
	{OUTPUT_QUEUES 0 1 nf_ndp_output_queues_v1_0_0/data/output_queues_regs_defines.txt} \
	{OUTPUT_PORT_LOOKUP 0 1 switch_ndp_output_port_lookup_v1_0_0/data/output_port_lookup_regs_defines.txt} \
	{NF_10G_INTERFACE0 0 1 nf_10ge_interface_shared_v1_0_0/data/nf_10g_interface_shared_regs_defines.txt} \
	{NF_10G_INTERFACE1 1 1 nf_10ge_interface_v1_0_0/data/nf_10g_interface_regs_defines.txt} \
	{NF_10G_INTERFACE2 2 1 nf_10ge_interface_v1_0_0/data/nf_10g_interface_regs_defines.txt} \
	{NF_10G_INTERFACE3 3 1 nf_10ge_interface_v1_0_0/data/nf_10g_interface_regs_defines.txt} \
	{NF_RIFFA_DMA 0 1 nf_riffa_dma_v1_0_0/data/nf_riffa_dma_regs_defines.txt} \


}

set target_path $::env(NF_DESIGN_DIR)/sw/embedded/src/
set target_file $target_path/sume_register_defines.h


######################################################
# the following function writes the license header
# into the file
######################################################

proc write_header { target_file } {

# creat a blank header file
# do a fresh rewrite in case the file already exits
file delete -force $target_file
open $target_file "w"
set h_file [open $target_file "w"]


puts $h_file "//-"
puts $h_file "// Copyright (c) 2015 University of Cambridge"
puts $h_file "// All rights reserved."
puts $h_file "//"
puts $h_file "// This software was developed by Stanford University and the University of Cambridge Computer Laboratory "
puts $h_file "// under National Science Foundation under Grant No. CNS-0855268,"
puts $h_file "// the University of Cambridge Computer Laboratory under EPSRC INTERNET Project EP/H040536/1 and"
puts $h_file "// by the University of Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-11-C-0249 (\"MRC2\"), "
puts $h_file "// as part of the DARPA MRC research programme."
puts $h_file "//"
puts $h_file "// @NETFPGA_LICENSE_HEADER_START@"
puts $h_file "//"
puts $h_file "// Licensed to NetFPGA C.I.C. (NetFPGA) under one or more contributor"
puts $h_file "// license agreements.  See the NOTICE file distributed with this work for"
puts $h_file "// additional information regarding copyright ownership.  NetFPGA licenses this"
puts $h_file "// file to you under the NetFPGA Hardware-Software License, Version 1.0 (the"
puts $h_file "// \"License\"); you may not use this file except in compliance with the"
puts $h_file "// License.  You may obtain a copy of the License at:"
puts $h_file "//"
puts $h_file "//   http://www.netfpga-cic.org"
puts $h_file "//"
puts $h_file "// Unless required by applicable law or agreed to in writing, Work distributed"
puts $h_file "// under the License is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR"
puts $h_file "// CONDITIONS OF ANY KIND, either express or implied.  See the License for the"
puts $h_file "// specific language governing permissions and limitations under the License."
puts $h_file "//"
puts $h_file "// @NETFPGA_LICENSE_HEADER_END@"
puts $h_file "/////////////////////////////////////////////////////////////////////////////////"
puts $h_file "// This is an automatically generated header definitions file"
puts $h_file "/////////////////////////////////////////////////////////////////////////////////"
puts $h_file ""

close $h_file 

}; # end of proc write_header


######################################################
# the following function writes all the information
# of a specific core into a file
######################################################

proc write_core {target_file prefix id has_registers lib_name} {


set h_file [open $target_file "a"]

#First, read the memory map information from the reference_project defines file
source $::env(NF_DESIGN_DIR)/hw/tcl/$::env(NF_PROJECT_NAME)_defines.tcl
set public_repo_dir $::env(SUME_FOLDER)/lib/hw/


set baseaddr [set $prefix\_BASEADDR]
set highaddr [set $prefix\_HIGHADDR]
set sizeaddr [set $prefix\_SIZEADDR]

puts $h_file "//######################################################"
puts $h_file "//# Definitions for $prefix"
puts $h_file "//######################################################"

puts $h_file "#define SUME_$prefix\_BASEADDR $baseaddr"
puts $h_file "#define SUME_$prefix\_HIGHADDR $highaddr"
puts $h_file "#define SUME_$prefix\_SIZEADDR $sizeaddr"
puts $h_file ""

#Second, read the registers information from the library defines file
if $has_registers {
	if {$lib_name == "input_drr_arbiter_v1_0_0/data/input_drr_arbiter_regs_defines.txt" || 
	    $lib_name == "switch_ndp_output_port_lookup_v1_0_0/data/output_port_lookup_regs_defines.txt" ||
	    $lib_name == "nf_ndp_output_queues_v1_0_0/data/output_queues_regs_defines.txt"} {
		set lib_path "$public_repo_dir/contrib/cores/$lib_name"
	} else {
		set lib_path "$public_repo_dir/std/cores/$lib_name"
	}
	set regs_h_define_file $lib_path
	set regs_h_define_file_read [open $regs_h_define_file r]
	set regs_h_define_file_data [read $regs_h_define_file_read]
	close $regs_h_define_file_read
	set regs_h_define_file_data_line [split $regs_h_define_file_data "\n"]

       foreach read_line $regs_h_define_file_data_line {
            if {[regexp "#define" $read_line]} {
                puts $h_file "#define SUME_[lindex $read_line 2]\_$id\_[lindex $read_line 3]\_[lindex $read_line 4] [lindex $read_line 5]"
            }
	}
}
puts $h_file ""
close $h_file 
}; # end of proc write_core



######################################################
# the main function
######################################################



write_header  $target_file 

foreach lib_item $DEF_LIST { 
     write_core  $target_file [lindex $lib_item 0] [lindex $lib_item 1] [lindex $lib_item 2] [lindex $lib_item 3] 
}






