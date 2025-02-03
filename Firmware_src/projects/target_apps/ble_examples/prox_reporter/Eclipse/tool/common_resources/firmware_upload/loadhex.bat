@echo off

set tool_dir=%~1
set hex_dir=%2
set hex_name=%3
set bin_name=%4
set cmd_file=loadbin.txt

IF NOT "%5"=="" (
	set emulator=%5
) else (
	set emulator=-1
) 


cd %hex_dir%

"%tool_dir%\hex2bin.exe" -e bin %hex_name%

echo r > %cmd_file%
echo w2 50000012, A6 >> %cmd_file%
echo W2 50003308, 2e >> %cmd_file%
echo loadbin %bin_name%, 0 >> %cmd_file%
echo verifybin %bin_name%, 0 >> %cmd_file%
echo r >> %cmd_file%
echo g >> %cmd_file%
echo exit >> %cmd_file%

echo.
echo Tools directory: %tool_dir%
echo Using HEX file: %hex_dir% %hex_name%
echo.
echo --- Generated JLink Script File ---
type %cmd_file%
echo ---
echo.

if %emulator% GEQ 0 (
	"%tool_dir%\JLink.exe" -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M0 -SelectEmuBySN %emulator% -CommanderScript %cmd_file%
) else (
	"%tool_dir%\JLink.exe" -if SWD -speed 4000 -autoconnect 1 -device CORTEX-M0 -CommanderScript %cmd_file%
)

::pause
