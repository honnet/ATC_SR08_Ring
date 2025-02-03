python ..\make_bin_ota.py
..\tool\SmartSnippetsToolbox.exe -firmware jtag_programmer.bin -type spi -clk P0_0 -cs P0_3 -miso P0_5 -mosi P0_6 -chip DA14585-00 -jtag 483057454 -offset 0x18000 -verify -cmd write -y -file ..\output.bin
