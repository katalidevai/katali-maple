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
