#!/bin/bash

echo ""
echo "-------------------------"
echo "Cloning now the latest NetFPGA-SUME repo..."
echo "WARNING: you need to have github access to NetFPGA-SUME repo"
echo "If you do not have access, request permission here: http://netfpga.org/site/#/SUME_reg_form/" 
echo "More information here: https://github.com/NetFPGA/NetFPGA-SUME-public/wiki/Getting-Started-Guide"
echo "-------------------------"
echo ""
sleep 2
git clone https://github.com/NetFPGA/NetFPGA-SUME-live.git
sleep 2
echo ""
echo "-------------------------"
echo "Checking out the NetFPGA-SUME release 1.5"
echo "-------------------------"
echo ""
sleep 2
cd NetFPGA-SUME-live && git checkout 1b0b1d3
sleep 2
echo ""
echo "-------------------------"
echo "Adding NDP modules to the repo"
echo "-------------------------"
sleep 2
cd ..
cp -rf ndp_modules/input_drr_arbiter_v1_0_0/ NetFPGA-SUME-live/lib/hw/contrib/cores/
cp -rf ndp_modules/switch_ndp_output_port_lookup_v1_0_0/ NetFPGA-SUME-live/lib/hw/contrib/cores/
cp -rf ndp_modules/nf_ndp_v1_0_0/ NetFPGA-SUME-live/lib/hw/contrib/cores/
cp -rf ndp_modules/nf_ndp_output_queues_v1_0_0/ NetFPGA-SUME-live/lib/hw/contrib/cores/
cp -rf ndp_modules/ndp/ NetFPGA-SUME-live/contrib-projects/
echo "OK"
sleep 2
echo ""
echo "-------------------------"
echo "Patching NetFPGA-SUME repo for NDP"
echo "-------------------------"
sleep 2
patch -p 0 < ndp_patch.diff
sleep 2
echo ""
echo "-------------------------"
echo "Creating NetFPGA-SUME and NDP cores"
echo "-------------------------"
sleep 2
export LOCAL_DIR=${PWD}
cd NetFPGA-SUME-live && source tools/settings.sh
make
sleep 2
echo ""
echo "-------------------------"
echo "Generating NDP bitfile"
echo "-------------------------"
sleep 2
cd contrib-projects/ndp/ && make
sleep 2
echo ""
echo "-------------------------"
echo "Process finished!"
echo "-------------------------"
echo "You can find the NDP bitfile here:"
echo "$PROJECTS/bitfiles"


















