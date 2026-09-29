# Maple benchmark

Measured on the development desktop:

- CPU: Intel Core i5-10400, 6 cores / 12 threads
- GPU: NVIDIA GeForce RTX 4060, 8 GB
- Model: Maple `maple-tq2_0.gguf`, approximately 5.08 GiB on disk
- Context: 2048 tokens
- Decode sample: 32 generated tokens
- Engine: Katali native runtime; no llama.cpp inference path

## Results

| Mode | Prefill | Decode |
| --- | ---: | ---: |
| CPU | 3.36 tok/s | 3.42 tok/s |
| CUDA MoE | 8.36 tok/s | 9.36 tok/s |
| CUDA MoE + resident LM head | — | **10.20 tok/s** |

CUDA verification reported 6,144 routed expert calls on GPU and zero GPU fallbacks. In the baseline CUDA MoE row, attention and the language-model head remained CPU-side; the opt-in LM-head row moves only that final quantized matrix to CUDA.

The resident LM-head run used `KATALI_DENSE_GPU=1`, `KATALI_DENSE_TIER=lm`,
and `KATALI_MOE_LM_GPU=1`. It used 0.16 GiB of VRAM and produced byte-identical
32-token output to the CUDA MoE baseline (matching SHA-256 output). This is an
approximately 9% decode improvement over the 9.36 tok/s CUDA MoE run on the
same machine.

For comparison, the existing Katali Qwen baseline was approximately 3.95 decode tok/s on the same system. Maple reached about 2.37x that rate with CUDA.

These are warm-cache local measurements, not a guarantee for other hardware or prompts.
