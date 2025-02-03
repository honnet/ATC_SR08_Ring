@echo off


set drivers_location=%1


cd %drivers_location%


 
Set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
 
REG.exe Query %RegQry% > checkOS.txt
 
Find /i "x86" < CheckOS.txt > StringCheck.txt
 
If %ERRORLEVEL% == 0 (
    :: This is 32 Bit Operating System
    .\dpinst-x86.exe /q /se
) ELSE (
    :: This is 64 Bit Operating System
    .\dpinst-amd64.exe /q /se
)


del /F CheckOS.txt
del /F StringCheck.txt

exit /B 0

