# Maple benchmark — **26.485 tok/s latest result**

The latest verified Maple runtime measurement is **26.485 tok/s decode** on an
NVIDIA RTX 4060 using Katali's native CUDA attention, routed MoE execution, and
resident LM head. The Manila smoke test returned the correct answer with zero
GPU fallbacks.

Measured on the development desktop:

- CPU: Intel Core i5-10400, 6 cores / 12 threads
- GPU: NVIDIA GeForce RTX 4060, 8 GB
- Model: Maple `maple-tq2_0.gguf`, approximately 5.08 GiB on disk
- Decode sample: 35 tokens
- Engine: Katali native runtime; no llama.cpp inference path

The verified run used:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_DENSE_GPU = "1"
$env:KATALI_DENSE_TIER = "attn,lm"
$env:KATALI_MOE_ATTN_GPU = "1"
$env:KATALI_MOE_LM_GPU = "1"
```

This is a warm-cache local measurement. Actual speed varies with hardware,
context length, prompt, and cache state. Tool calls add extra generation
round-trips, so their end-to-end response time is longer than one decode pass.

## LFM2-24B-A2B measurement

The native LFM2 path was also exercised with
`C:\models\LFM2-24B-A2B-Q4_K_M.gguf` (24B total parameters, about 2B active,
`lfm2moe` architecture). On the same RTX 4060, the best stable short decode
measurement was **13.4 tok/s sustained over 16 decode tokens** using CUDA
routed experts, DP4A kernels, asynchronous MoE execution, and a 7 GiB expert
cache. The dense short-convolution tier is intentionally disabled in this
configuration because it gives lower end-to-end throughput for LFM2:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_DENSE_GPU = "0"
$env:KATALI_CUDA_MOE_ASYNC = "1"
$env:KATALI_CUDA_DP4A = "1"
$env:KATALI_VRAM_GB = "7.0"
$env:KATALI_EC_WARMUP = "0"
.\katali-lab.exe generate C:\models\LFM2-24B-A2B-Q4_K_M.gguf `
  "Capital of the Philippines?" --max 16 --ctx 512 --cache-gb 10
```

The limiting cost is streaming selected expert weights into the 8 GB GPU;
reaching 20 tok/s needs a larger resident expert set or a fused LFM2
short-convolution/MoE CUDA path.
