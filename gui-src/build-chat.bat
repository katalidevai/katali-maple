@echo off
setlocal
gcc -O2 -municode -mwindows katali-chat.c -o ..\katali-chat.exe -lwinhttp -luser32 -lgdi32
if errorlevel 1 exit /b 1
echo OK: ..\katali-chat.exe
endlocal
