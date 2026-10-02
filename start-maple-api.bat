@echo off
setlocal

rem Katali native Maple API (OpenAI-compatible loopback server)
set "KATALI_CUDA_MOE=1"
set "KATALI_DENSE_GPU=1"
set "KATALI_DENSE_TIER=attn,lm"
set "KATALI_MOE_ATTN_GPU=1"
set "KATALI_MOE_LM_GPU=1"
set "KATALI_CUDA_GQA=1"
set "KATALI_API_MODEL=C:\models\maple-tq2_0.gguf"

set "KATALI_API_PORT=%KATALI_API_PORT%"
if not defined KATALI_API_PORT set "KATALI_API_PORT=8119"

echo Starting Katali Maple API on http://127.0.0.1:%KATALI_API_PORT%
echo Endpoint: POST /v1/chat/completions
echo Health:   GET  /health
"%~dp0katali-lab.exe" api --port %KATALI_API_PORT%

endlocal
