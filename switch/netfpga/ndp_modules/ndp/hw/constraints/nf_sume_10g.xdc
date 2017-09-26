
#
# Copyright (c) 2015 Noa Zilberman, Yury Audzevich
# All rights reserved.
#
#  File:
#        nf_sume_10g.xdc
#
#  Author: Noa Zilberman
#
#  Description:
#        Location constraints for 4x 10GbE SFP+ interface used in reference
#        projects.
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
# license agreements. See the NOTICE file distributed with this work for
# additional information regarding copyright ownership. NetFPGA licenses this
# file to you under the NetFPGA Hardware-Software License, Version 1.0 (the
# "License"); you may not use this file except in compliance with the
# License. You may obtain a copy of the License at:
#
# http://www.netfpga-cic.org
#
# Unless required by applicable law or agreed to in writing, Work distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations under the License.
#
# @NETFPGA_LICENSE_HEADER_END@
#


# XGE-SFP0 -- SUME -- THE FIRST INTERFACE FROM THE TOP OF THE BOARD

set_property PACKAGE_PIN M18 [get_ports sfp0_tx_disable]
set_property IOSTANDARD LVCMOS15 [get_ports sfp0_tx_disable]
set_property PACKAGE_PIN M19 [get_ports sfp0_tx_fault]
set_property IOSTANDARD LVCMOS15 [get_ports sfp0_tx_fault]
set_property PACKAGE_PIN N18 [get_ports sfp0_tx_abs]

set_property IOSTANDARD LVCMOS15 [get_ports sfp0_tx_abs]
set_property LOC GTHE2_CHANNEL_X1Y39 [get_cells -hier -filter name=~*interface_0*gthe2_i]

# XGE-SFP1 -- SUME

set_property PACKAGE_PIN B31 [get_ports sfp1_tx_disable]
set_property IOSTANDARD LVCMOS15 [get_ports sfp1_tx_disable]
set_property PACKAGE_PIN C26 [get_ports sfp1_tx_fault]
set_property IOSTANDARD LVCMOS15 [get_ports sfp1_tx_fault]
set_property PACKAGE_PIN L19 [get_ports sfp1_tx_abs]
set_property IOSTANDARD LVCMOS15 [get_ports sfp1_tx_abs]

set_property LOC GTHE2_CHANNEL_X1Y38 [get_cells -hier -filter name=~*interface_1*gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y38 [get_cells nf_10g_interface_1/inst/nf_10g_interface_block_i/axi_10g_ethernet_i/inst/ten_gig_eth_pcs_pma/inst/gt0_gtwizard_10gbaser_multi_gt_i/gt0_gtwizard_gth_10gbaser_i/gthe2_i]

# XGE-SFP2 -- SUME

set_property PACKAGE_PIN J38 [get_ports sfp2_tx_disable]
set_property IOSTANDARD LVCMOS15 [get_ports sfp2_tx_disable]
set_property PACKAGE_PIN E39 [get_ports sfp2_tx_fault]
set_property IOSTANDARD LVCMOS15 [get_ports sfp2_tx_fault]
set_property PACKAGE_PIN J37 [get_ports sfp2_tx_abs]
set_property IOSTANDARD LVCMOS15 [get_ports sfp2_tx_abs]

set_property LOC GTHE2_CHANNEL_X1Y37 [get_cells -hier -filter name=~*interface_2*gthe2_i]

# XGE-SFP3 -- SUME -- FIRST FROM THE BOTTOM (to PCIe)

set_property PACKAGE_PIN L21 [get_ports sfp3_tx_disable]
set_property IOSTANDARD LVCMOS15 [get_ports sfp3_tx_disable]
set_property PACKAGE_PIN J26 [get_ports sfp3_tx_fault]
set_property IOSTANDARD LVCMOS15 [get_ports sfp3_tx_fault]
set_property PACKAGE_PIN H36 [get_ports sfp3_tx_abs]
set_property IOSTANDARD LVCMOS15 [get_ports sfp3_tx_abs]

set_property LOC GTHE2_CHANNEL_X1Y36 [get_cells -hier -filter name=~*interface_3*gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y36 [get_cells nf_10g_interface_3/inst/nf_10g_interface_block_i/axi_10g_ethernet_i/inst/ten_gig_eth_pcs_pma/inst/gt0_gtwizard_10gbaser_multi_gt_i/gt0_gtwizard_gth_10gbaser_i/gthe2_i]

## -- SFP clocks
## -- The clock is supplied by Si5324 chip;
## -- the clock is configured through microblaze.
set_property PACKAGE_PIN E10 [get_ports xphy_refclk_p]
set_property PACKAGE_PIN E9 [get_ports xphy_refclk_n]
create_clock -period 6.400 [get_ports xphy_refclk_p]

#create_clock -period 6.400 -name xgemac_clk_156 [get_ports xphy_refclk_p]

# XGE TX/RX LEDs
#   GRN - TX
#   YLW - RX
set_property PACKAGE_PIN G13 [get_ports sfp0_tx_led]
set_property PACKAGE_PIN AL22 [get_ports sfp1_tx_led]
set_property PACKAGE_PIN AY18 [get_ports sfp2_tx_led]
set_property PACKAGE_PIN P31 [get_ports sfp3_tx_led]

set_property PACKAGE_PIN L15 [get_ports sfp0_rx_led]
set_property PACKAGE_PIN BA20 [get_ports sfp1_rx_led]
set_property PACKAGE_PIN AY17 [get_ports sfp2_rx_led]
set_property PACKAGE_PIN K32 [get_ports sfp3_rx_led]

set_property IOSTANDARD LVCMOS15 [get_ports sfp?_?x_led]

#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets nf_10g_shared_i/inst/refclk]

## Timing Constraints
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_0*gthe2_i/RXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_0*gthe2_i/TXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_1*gthe2_i/RXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_1*gthe2_i/TXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_2*gthe2_i/RXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_2*gthe2_i/TXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_3*gthe2_i/RXOUTCLK]
create_clock -period 3.103  [get_pins -hier -filter name=~*interface_3*gthe2_i/TXOUTCLK]





## Timing Constraints

###################
## Other constraints
set_false_path -from [get_clocks xphy_refclk_p] -to [get_clocks clk_200]
set_false_path -from [get_clocks clk_200] -to [get_clocks xphy_refclk_p]

set_false_path -from [get_clocks clk_250mhz_mux_x0y1] -to [get_clocks clk_125mhz_x0y1]
set_false_path -from [get_clocks clk_125mhz_x0y1] -to [get_clocks clk_250mhz_mux_x0y1]

set_false_path -from [get_clocks userclk1] -to [get_clocks clk_200]
set_false_path -from [get_clocks clk_200] -to [get_clocks userclk1]

set_false_path -from [get_clocks userclk1] -to [get_clocks sys_clk]
set_false_path -from [get_clocks sys_clk] -to [get_clocks userclk1]

set_false_path -from [get_clocks userclk1] -to [get_clocks axi_clk]
set_false_path -from [get_clocks axi_clk] -to [get_clocks userclk1]

set_false_path -from [get_clocks userclk1] -to [get_clocks xphy_refclk_p]
set_false_path -from [get_clocks xphy_refclk_p] -to [get_clocks userclk1]

set_false_path -from [get_clocks -filter name=~*interface_*gthe2_i/RXOUTCLK] -to [get_clocks xphy_refclk_p]
set_false_path -from [get_clocks xphy_refclk_p] -to [get_clocks -filter name=~*interface_*gthe2_i/RXOUTCLK]

set_false_path -from [get_clocks  -filter name=~*interface_*gthe2_i/TXOUTCLK] -to [get_clocks xphy_refclk_p]
set_false_path -from [get_clocks xphy_refclk_p] -to [get_clocks -filter name=~*interface_*gthe2_i/TXOUTCLK]


