@echo off
setlocal

rem Katali native Maple API (OpenAI-compatible loopback server)
if not defined KATALI_CUDA_MOE set "KATALI_CUDA_MOE=1"
if not defined KATALI_DENSE_GPU set "KATALI_DENSE_GPU=1"
if not defined KATALI_DENSE_TIER set "KATALI_DENSE_TIER=attn,lm"
if not defined KATALI_MOE_ATTN_GPU set "KATALI_MOE_ATTN_GPU=1"
if not defined KATALI_MOE_LM_GPU set "KATALI_MOE_LM_GPU=1"
if not defined KATALI_CUDA_GQA set "KATALI_CUDA_GQA=1"
if not defined KATALI_API_MODEL set "KATALI_API_MODEL=C:\models\maple-tq2_0.gguf"
if not defined KATALI_TOOL_ROOT set "KATALI_TOOL_ROOT=%~dp0"

set "KATALI_API_PORT=%KATALI_API_PORT%"
if not defined KATALI_API_PORT set "KATALI_API_PORT=8119"

echo Starting Katali Maple API on http://127.0.0.1:%KATALI_API_PORT%
echo Endpoint: POST /v1/chat/completions
echo Health:   GET  /health
"%~dp0katali-lab.exe" api --port %KATALI_API_PORT%

endlocal
