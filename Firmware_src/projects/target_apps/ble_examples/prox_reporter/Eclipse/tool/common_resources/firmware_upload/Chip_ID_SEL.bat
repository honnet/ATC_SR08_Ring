@rem ***************************************************************************
@rem Copyright(C) [2022-2023] Renesas Electronics Corporation and/or its affiliates.
@rem All rights reserved. Confidential Information.
@rem
@rem This software (“Software”) is supplied by Renesas Electronics Corporation and/or its 
@rem affiliates (“Renesas”). Renesas grants you a personal, non-exclusive, non-transferable, 
@rem revocable, non-sub-licensable right and license to use the Software, solely if 
@rem used in or together with Renesas products. 
@rem You may make copies of this Software, provided this copyright notice and 
@rem disclaimer (“Notice”) is included in all such copies. Renesas reserves the right to 
@rem change or discontinue the Software at any time without notice.
@rem
@rem THE SOFTWARE IS PROVIDED “AS IS”. RENESAS DISCLAIMS ALL WARRANTIES OF 
@rem ANY KIND, WHETHER EXPRESS, IMPLIED, OR STATUORY, INCLUDING BUT NOT 
@rem LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
@rem PURPOSE AND NONINFRINGEMENT. TO THE MAXIMUM EXTENT PERMITTED 
@rem UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT, INDIRECT, 
@rem SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR 
@rem IN CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF 
@rem THE POSSIBILITY OF SUCH DAMAGES.
@rem
@rem USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS 
@rem CONTAINED IN AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. 
@rem IN CASE OF CONFLICT BETWEEN THE TERMS OF THIS NOTICE AND ANY SUCH 
@rem ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT SHALL TAKE 
@rem PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE 
@rem TERMS OF THIS NOTICE. IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT 
@rem PERMITTED TO USE THIS SOFTWARE.
@rem ***************************************************************************
@ echo off
setlocal EnableDelayedExpansion

set FILENAME=%1
set JTAG_SERIAL=%2

:PROGRAM

echo  > jlinkscript.txt
echo r >> jlinkscript.txt
echo h >> jlinkscript.txt
echo mem8  0x50003200, 3 >> jlinkscript.txt		rem DA1458x, DA14585, DA1468x
echo mem32 0x50003200, 4 >> jlinkscript.txt		rem DA14531
echo mem32 0x50050200, 4 >> jlinkscript.txt 	rem DA1459x
echo mem32 0x50040200, 4 >> jlinkscript.txt		rem DA1469x
echo mem32 0x50040000, 4 >> jlinkscript.txt		rem DA1470x
echo mem32 0x50000012, 1 >> jlinkscript.txt		rem DA145xx SYS_CTRL_REG
echo mem32 0x50000024, 1 >> jlinkscript.txt		rem DA146xx/DA147xx SYS_CTRL_REG
echo qc >> jlinkscript.txt
Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt
echo "------------"

set /a sys_ctrl_reg_12 = 0
set /a sys_ctrl_reg_24 = 0

for /f "tokens=1-3" %%a in ('findstr /C:"50000012 =" jlink_result.txt') do (
	set sys_ctrl_reg_12=%%c
)	
for /f "tokens=1-3" %%a in ('findstr /C:"50000024 =" jlink_result.txt') do (
	set sys_ctrl_reg_24=%%c
)	

rem DA1458x
>nul find "50003200 = 35 38 30" jlink_result.txt && (
	goto :DEV_ID_DA1458x
)

rem DA14585
>nul find "50003200 = 35 38 35" jlink_result.txt && (
	goto :DEV_ID_DA14585
)

rem DA14531
>nul find "50003200 = 00000032 00000036 00000033 00000032" jlink_result.txt && (
	rem This is required because DA14531 does not have fixed JTAG pins
	set sys_ctrl_pre_new=%sys_ctrl_reg_12:~0,-1%2
	set sys_ctrl_new=%sys_ctrl_reg_12:~0,4%8%sys_ctrl_reg_12:~5,-1%2

    goto :DEV_ID_DA1453x
)

rem DA14535
>nul find "50003200 = 00000033 00000030 00000038 00000031" jlink_result.txt && (
	rem This is required because DA14535 does not have fixed JTAG pins
	set sys_ctrl_pre_new=%sys_ctrl_reg_12:~0,-1%2
	set sys_ctrl_new=%sys_ctrl_reg_12:~0,4%8%sys_ctrl_reg_12:~5,-1%2

	echo DEV_ID_DA1453x
    goto :DEV_ID_DA1453x
)

rem DA1468x
>nul find "50003200 = 36 38 30" jlink_result.txt && (
	goto :DEV_ID_DA1468x
)

rem DA1459x
>nul find "50050200 = 00000032 00000036 00000033 00000034" jlink_result.txt && (
	goto :DEV_ID_DA1459x
)

rem DA1469x
>nul find "50040200 = 00000032 00000035 00000032 00000032" jlink_result.txt && (
	set sys_ctrl_new=%sys_ctrl_reg_24:~0,6%A3
	goto :DEV_ID_DA1469x
)
>nul find "50040200 = 00000033 00000030 00000038 00000030" jlink_result.txt && (
	set sys_ctrl_new=%sys_ctrl_reg_24:~0,6%A3
	goto :DEV_ID_DA1469x
)

rem DA1470x
>nul find "50040000 = 00000032 00000037 00000039 00000038" jlink_result.txt && (
	goto :DEV_ID_DA1470x
)
>nul find "50040000 = 00000033 00000031 00000030 00000037" jlink_result.txt && (
	goto :DEV_ID_DA1470x
)

exit 


:DEV_ID_DA1458x
	echo selected DA1458x
	echo > jlinkscript.txt
	echo w2 50000012, A2 >> jlinkscript.txt
	echo w2 50003308, 2E >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo w2 50003300, 8 >> jlinkscript.txt
	echo w2 50003102, 1 >> jlinkscript.txt
	echo w2 50003100, 5 >> jlinkscript.txt
	echo w2 50000012, A2 >> jlinkscript.txt
	echo w2 50003308, 2E >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0 >> jlinkscript.txt
    echo w2 50000012, 80A6 >> jlinkscript.txt
	echo sleep 50 >> jlinkscript.txt
	echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 


:DEV_ID_DA14585
	echo selected DA14585
	echo > jlinkscript.txt
	echo r >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo w2 50003300, 8 >> jlinkscript.txt
	echo w2 50003102, 1 >> jlinkscript.txt
	echo w2 50003100, 5 >> jlinkscript.txt
	echo w2 50000012, A2 >> jlinkscript.txt		
	echo loadbin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0 >> jlinkscript.txt
    echo w2 50000012, 80A6 >> jlinkscript.txt
	echo sleep 50 >> jlinkscript.txt
	echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 


:DEV_ID_DA1453x
	echo > jlinkscript.txt
	echo r >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo w2 50003300, 08 >> jlinkscript.txt			rem set_freeze_reg
	echo w2 50003102, 01 >> jlinkscript.txt			rem wdog_ctrl_reg
	echo w2 50000012, %sys_ctrl_pre_new% >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 07FC0000 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 07FC0000 >> jlinkscript.txt
	echo sleep 10 >> jlinkscript.txt
	echo w4 50000012, %sys_ctrl_new% >> jlinkscript.txt
	echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 


:DEV_ID_DA1468x
	echo selected DA1468x
	echo > jlinkscript.txt
	echo r >> jlinkscript.txt
	echo sleep 10 >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo w4 7fd0000,deadbeef >> jlinkscript.txt
	echo w4 7fd0004,deadbeef >> jlinkscript.txt
	echo w4 7fd0008,deadbeef >> jlinkscript.txt
	echo w4 7fd000c,dead10cc >> jlinkscript.txt
	echo w4 400c3050, 1 >> jlinkscript.txt
	echo sleep 500 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo w4 7fd0000, 0 >> jlinkscript.txt
	echo w2 50000012, A3 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo rx 50 >> jlinkscript.txt
	echo g >> jlinkscript.txt
	echo q >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 1000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 


:DEV_ID_DA1459x
	echo selected DA1459x
	echo > jlinkscript.txt
	echo w4 20010000, deadbeef >> jlinkscript.txt
	echo w4 20010004, deadbeef >> jlinkscript.txt
	echo w4 20010008, deadbeef >> jlinkscript.txt
	echo w4 2001000c, dead10cc >> jlinkscript.txt
	echo w4 1A0C0050, 1 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo sleep 10 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo h >> jlinkscript.txt
	echo w4 50000024, C3 >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo sleep 10 >> jlinkscript.txt
	echo w4 50000024, 80C3 >> jlinkscript.txt
	echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 


:DEV_ID_DA1469x
	echo selected DA1469x
	echo > jlinkscript.txt
	rem echo w4 20010000 , deadbeef >> jlinkscript.txt
	rem echo w4 20010004 , deadbeef >> jlinkscript.txt
	rem echo w4 20010008 , deadbeef >> jlinkscript.txt
	rem echo w4 2001000c , dead10cc >> jlinkscript.txt
	rem echo w4 100c0050 , 1 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	rem echo h >> jlinkscript.txt
	echo w4 50000024, A3 >> jlinkscript.txt
	echo w4 50040300, 8 >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 0 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0 >> jlinkscript.txt

	echo rx 50 >> jlinkscript.txt
	echo g >> jlinkscript.txt
	echo exit >> jlinkscript.txt

	rem echo w4 50000024, A3 >> jlinkscript.txt
	rem echo sleep 50 >> jlinkscript.txt
	rem echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 2000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded

	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt

	exit 

:DEV_ID_DA1470x
	echo selected DA1470x
	echo > jlinkscript.txt
	echo w4 0F001000, deadbeef >> jlinkscript.txt
	echo w4 0F001004, deadbeef >> jlinkscript.txt
	echo w4 0F001008, deadbeef >> jlinkscript.txt
	echo w4 0F00100c, dead10cc >> jlinkscript.txt
	echo w4 100c0050, 1 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo sleep 50 >> jlinkscript.txt
	echo r >> jlinkscript.txt
	echo w4 50000024, 000000C5
	echo w4 50000010, 4 >> jlinkscript.txt
	echo loadbin %FILENAME%.bin, 0x20010000 >> jlinkscript.txt
	echo verifybin %FILENAME%.bin, 0x20010000 >> jlinkscript.txt
	echo w4 50000024, 80C5 >> jlinkscript.txt
	echo qc >> jlinkscript.txt
	echo Loading file
	Jlink -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt
	echo File loaded
	
	timeout 2 >nul
	
	echo h > jlinkscript.txt
	echo qc >> jlinkscript.txt
	Jlink -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M33 -SelectEmuBySN %JTAG_SERIAL% -CommanderScript jlinkscript.txt > jlink_result.txt

	exit 
