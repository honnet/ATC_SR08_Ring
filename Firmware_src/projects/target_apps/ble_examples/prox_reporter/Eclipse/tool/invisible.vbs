if WScript.Arguments.Count = 1 then
    CreateObject("Wscript.Shell").Run """" & WScript.Arguments(0) & """", 0, False 
end if

if WScript.Arguments.Count = 2 then
    CreateObject("Wscript.Shell").Run """" & WScript.Arguments(0) & """" & Chr(34) & WScript.Arguments(1), 0, False 
end if
