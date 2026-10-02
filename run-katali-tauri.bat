@echo off
setlocal
set "KATALI_CUDA_MOE=1"
set "KATALI_CUDA_MOE_ASYNC=1"
set "KATALI_CUDA_DP4A=1"
set "KATALI_VRAM_GB=6.5"
set "KATALI_EC_WARMUP=0"
if not defined KATALI_API_MODEL set "KATALI_API_MODEL=C:\models\LFM2-24B-A2B-Q4_K_M.gguf"
start "Katali Maple Tauri" "%~dp0tauri-app\src-tauri\target\release\katali-tauri.exe"
endlocal
