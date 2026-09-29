# Katali Maple Preview

Binary-only release of the Katali native engine with Maple `TQ2_0` support.

## Included

- `katali-lab.exe` — native Katali engine and local HTTP API server
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

## Quick start

When KATALI_API_MODEL is set, the API opens Maple once and reuses a persistent worker for sequential requests.

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_API_MODEL = "C:\models\maple-tq2_0.gguf"
.\katali-lab.exe api --port 8080
```

Then follow [API_TUTORIAL.md](API_TUTORIAL.md).
