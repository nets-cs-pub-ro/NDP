#
# Copyright (c) 2015 Noa Zilberman
# All rights reserved.
#
#  File:
#        Makefile
#
#  Project:
#        ndp
#
#
#  Description:
#        This makefile is used to generate and compile SDK project for NetFPGA reference projects.
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
# Vivado Launch Script
################################################################################


HW = hw
SW = sw/embedded/
TEST = test

all:    clean
	make -C ${HW} project
	make -C ${HW} export_to_sdk
	make -C ${SW} project
	make -C ${SW} compile
	make -C ${HW} load_elf


clean:
	make -C ${HW} distclean
	make -C ${SW} distclean
	rm -rfv vivado*;\


