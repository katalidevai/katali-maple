# Katali Maple Preview — **35.78 tok/s**

## Verified headline result

**35.78 tok/s decode on an NVIDIA RTX 4060** with Katali's native CUDA
attention, routed MoE, and resident LM head. That is **3.5× faster** than the
previous 10.20 tok/s CUDA configuration, with byte-identical output.

```text
Maple TQ2_0 · native Katali engine · RTX 4060 8 GB · 32-token decode
35.78 tok/s · 24/24 attention layers on CUDA · exact output verified
```

See the complete [benchmark](BENCHMARK.md) and start with the optimized flags
below.

Binary-only release of the Katali native engine with Maple `TQ2_0` support.

## Included

- `katali-lab.exe` — native Katali engine and local HTTP API server
- `katali-maple-gui.zip` — polished Windows desktop chat interface and Katali logo
- `katali_cuda.dll` — optional native CUDA backend
- `cudart64_13.dll` — CUDA runtime dependency
- [API_TUTORIAL.md](API_TUTORIAL.md) — local OpenAI-compatible API instructions
- [BENCHMARK.md](BENCHMARK.md) — Maple CPU/CUDA measurements

The repository intentionally does not include engine source code, patches, model weights, or the development checkout.

## Requirements

- Windows 10/11 x64
- NVIDIA driver and an NVIDIA GPU for CUDA mode
- Maple `maple-tq2_0.gguf` model downloaded separately from the [Maple GGUF repository](https://huggingface.co/stamsam/maple-preview-gguf)

## Model links

- [Maple-Preview original model](https://huggingface.co/deepgrove/maple-preview)
- [Maple-Preview GGUF conversions](https://huggingface.co/stamsam/maple-preview-gguf)

The engine runs CPU-only when CUDA is unavailable. CUDA MoE mode is opt-in with `KATALI_CUDA_MOE=1`.

## Desktop GUI

Install the .NET 9 Desktop Runtime, extract `katali-maple-gui.zip`, keep the
GUI executable and `katali-logo.jpg` beside `katali-lab.exe`, then double-click
the GUI. Choose `maple-tq2_0.gguf` from
the model picker and use the Settings panel to enable the verified native CUDA
attention and LM-head path.

For faster Maple decode on supported NVIDIA GPUs, enable the verified LM-head
resident path in addition to CUDA MoE:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_DENSE_GPU = "1"
$env:KATALI_DENSE_TIER = "lm"
$env:KATALI_MOE_LM_GPU = "1"
```

This keeps only the quantized LM-head matrix on CUDA; attention and routed
experts still use Katali's native paths. It is opt-in because performance is
hardware-dependent.

For the fastest verified Maple configuration on the development RTX 4060, use
native CUDA attention and the resident LM head together:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_DENSE_GPU = "1"
$env:KATALI_DENSE_TIER = "attn,lm"
$env:KATALI_MOE_ATTN_GPU = "1"
$env:KATALI_MOE_LM_GPU = "1"
```

This keeps all 24 Maple attention layers and the LM head resident on CUDA.
See [BENCHMARK.md](BENCHMARK.md) for the measured result and exact-output
verification.

## Quick start

When KATALI_API_MODEL is set, the API opens Maple once and reuses a persistent worker for sequential requests.

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_API_MODEL = "C:\models\maple-tq2_0.gguf"
.\katali-lab.exe api --port 8080
```

Then follow [API_TUTORIAL.md](API_TUTORIAL.md).
