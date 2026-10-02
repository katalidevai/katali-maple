@echo off
setlocal

rem Katali native LFM2-24B-A2B API + GUI launcher
set "KATALI_CUDA_MOE=1"
set "KATALI_DENSE_GPU=1"
set "KATALI_DENSE_TIER=attn,gdn,lm"
set "KATALI_CUDA_MOE_ASYNC=0"
set "KATALI_CUDA_GQA=1"
set "KATALI_API_MODEL=C:\models\LFM2-24B-A2B-Q4_K_M.gguf"
set "KATALI_TOOL_ROOT=%~dp0"

start "Katali LFM2 API" /min cmd /c ""%~dp0start-maple-api.bat""
timeout /t 3 /nobreak >nul
start "Katali LFM2 Chat" "%~dp0katali-chat.exe"

endlocal
