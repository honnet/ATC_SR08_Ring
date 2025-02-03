#!/bin/bash
#*******************************************************************************
# Copyright(C) [2022-2023] Renesas Electronics Corporation and/or its affiliates.
# All rights reserved. Confidential Information.
#
# This software (“Software”) is supplied by Renesas Electronics Corporation and/or its 
# affiliates (“Renesas”). Renesas grants you a personal, non-exclusive, non-transferable, 
# revocable, non-sub-licensable right and license to use the Software, solely if 
# used in or together with Renesas products. 
# You may make copies of this Software, provided this copyright notice and 
# disclaimer (“Notice”) is included in all such copies. Renesas reserves the right to 
# change or discontinue the Software at any time without notice.
#
# THE SOFTWARE IS PROVIDED “AS IS”. RENESAS DISCLAIMS ALL WARRANTIES OF 
# ANY KIND, WHETHER EXPRESS, IMPLIED, OR STATUORY, INCLUDING BUT NOT 
# LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE AND NONINFRINGEMENT. TO THE MAXIMUM EXTENT PERMITTED 
# UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT, INDIRECT, 
# SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR 
# IN CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF 
# THE POSSIBILITY OF SUCH DAMAGES.
#
# USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS 
# CONTAINED IN AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. 
# IN CASE OF CONFLICT BETWEEN THE TERMS OF THIS NOTICE AND ANY SUCH 
# ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT SHALL TAKE 
# PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE 
# TERMS OF THIS NOTICE. IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT 
# PERMITTED TO USE THIS SOFTWARE.
#*******************************************************************************


tool_dir=$1
hex_dir=$2
hex_name=$3
bin_name=$4
cmd_file=loadbin.txt

cd $hex_dir

"$tool_dir/hex2bin" -e bin $hex_name


echo si SWD > $cmd_file
echo speed 4000 >> $cmd_file
echo device Cortex-M0 >> $cmd_file
echo r >> $cmd_file
echo w2 50000012, A6 >> $cmd_file
echo W2 50003308, 2e >> $cmd_file
echo loadbin "$bin_name", 0 >> $cmd_file
echo verifybin "$bin_name", 0 >> $cmd_file
echo r >> $cmd_file
echo g >> $cmd_file
echo exit >> $cmd_file

echo "Tools directory: $tool_dir"
echo "Using HEX file: $hex_dir $hex_name"
echo "--- Generated JLink Script File ---"
cat $cmd_file
echo "---"

"$tool_dir/JLinkExe" -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M0 -CommanderScript $cmd_file


