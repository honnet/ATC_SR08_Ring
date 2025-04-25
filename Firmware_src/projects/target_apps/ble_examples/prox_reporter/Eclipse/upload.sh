#! /bin/sh

python ../make_bin_ota.py


exit
# WIP: the following uses JTAG probe
# (not useful for over the air programming)


java -jar ../tool/SmartSnippetsToolbox.jar \
  -firmware tool/jtag_programmer.bin \
  -type spi \
  -clk P0_0 \
  -cs P0_3 \
  -miso P0_5 \
  -mosi P0_6 \
  -chip DA14585-00 \
  -jtag 483057454 \
  -offset 0x18000 \
  -verify \
  -cmd write \
  -y \
  -file ../output.bin

