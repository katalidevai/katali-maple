@echo off
start "Katali Maple API" /min cmd /c ""%~dp0start-maple-api.bat""
timeout /t 3 /nobreak >nul
start "Katali Maple Chat" "%~dp0katali-chat.exe"
