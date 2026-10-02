# Katali Maple Preview — **26.485 tok/s**

## Verified headline result

**26.485 tok/s decode on an NVIDIA RTX 4060** with Katali's native CUDA
attention, routed MoE, and resident LM head. This is the latest verified
measurement for the current Maple runtime.

```text
Maple TQ2_0 · native Katali engine · RTX 4060 8 GB · 35-token decode
26.485 tok/s · 24/24 attention layers on CUDA · exact output verified
```

See the complete [benchmark](BENCHMARK.md) and start with the optimized flags
below.

Binary-only release of the Katali native engine with Maple `TQ2_0` support.

## Included

- `katali-lab.exe` — native Katali engine and local HTTP API server
- `katali-chat.exe` — lightweight native Windows desktop chat interface
- `run-katali-chat.bat` — starts the API and opens the desktop chat
- `katali-maple-gui.zip` — original packaged GUI release
- `katali_cuda.dll` — optional native CUDA backend
- `cudart64_13.dll` — CUDA runtime dependency
- [API_TUTORIAL.md](API_TUTORIAL.md) — local OpenAI-compatible API instructions
- [BENCHMARK.md](BENCHMARK.md) — Maple CPU/CUDA measurements

The GitHub release contains compiled binaries and runtime documentation only.
It does not include engine source code, GUI source code, patches, model weights,
or the development checkout.

## Requirements

- Windows 10/11 x64
- NVIDIA driver and an NVIDIA GPU for CUDA mode
- Maple `maple-tq2_0.gguf` model downloaded separately from the [Maple GGUF repository](https://huggingface.co/stamsam/maple-preview-gguf)

## Model links

- [Maple-Preview original model](https://huggingface.co/deepgrove/maple-preview)
- [Maple-Preview GGUF conversions](https://huggingface.co/stamsam/maple-preview-gguf)

The engine runs CPU-only when CUDA is unavailable. CUDA MoE mode is opt-in with `KATALI_CUDA_MOE=1`.

## Desktop GUI

The included `katali-chat.exe` is a native C/Win32 desktop chat window. It has
a conversation view, multiline prompt box, Send button, and Clear history
button. It talks to the local OpenAI-compatible API on port `8119`.

The chat sends the built-in Katali tools to Maple automatically. The available
tools are calculator, search_files, list_files, read_file, edit_file,
create_file, git_status, git_diff, run_tests, and guarded delete_file. File
tools are restricted to one selected workspace, and deletion requires
`confirm=true`. The **Workspace...** button chooses the folder that Maple may
read, create, edit, search, and inspect with Git. The selected folder is sent
to the API with every chat request, so the model and tools share the same
environment. The default is the folder containing `katali-chat.exe`.

Set up the model at `C:\models\maple-tq2_0.gguf`, then double-click
`run-katali-chat.bat`. The launcher starts the optimized API and opens the chat
window. No Python, .NET runtime, browser, or separate GUI installation is
required.

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
.\katali-lab.exe api --port 8119
```

Then follow [API_TUTORIAL.md](API_TUTORIAL.md).
