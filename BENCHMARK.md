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

CUDA verification reported 6,144 routed expert calls on GPU and zero GPU fallbacks. Attention and the language-model head remained CPU-side in this hybrid configuration.

For comparison, the existing Katali Qwen baseline was approximately 3.95 decode tok/s on the same system. Maple reached about 2.37x that rate with CUDA.

These are warm-cache local measurements, not a guarantee for other hardware or prompts.
