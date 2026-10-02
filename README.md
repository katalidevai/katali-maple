# Katali Maple Preview — **26.485 tok/s**

## Verified headline result

**26.485 tok/s decode on an NVIDIA RTX 4060** with Katali's native CUDA
attention, routed MoE, and resident LM head. This is the latest verified
measurement for the current Maple runtime.

```text
Maple TQ2_0 · bundled Katali runtime · RTX 4060 8 GB · 35-token decode
26.485 tok/s · 24/24 attention layers on CUDA · exact output verified
```

See the complete [benchmark](BENCHMARK.md) and start with the optimized flags
below.

The runtime also contains a native LFM2-24B-A2B MoE path. Its Q4_K_M model
needs about 14.4 GB on disk and uses roughly 2B active parameters per token;
see the LFM2 section in [BENCHMARK.md](BENCHMARK.md) for the measured CUDA
throughput and launch flags.

Katali Maple release with Maple `TQ2_0` support and a Rust/Tauri desktop
client.

## Included

- `katali-lab.exe` — bundled inference runtime and local HTTP API server
- `run-katali-tauri.bat` — starts the Rust/Tauri desktop client
- `katali-tauri.exe` — Rust/Tauri desktop chat client
- `katali_cuda.dll` — optional CUDA backend
- `cudart64_13.dll` — CUDA runtime dependency
- [API_TUTORIAL.md](API_TUTORIAL.md) — local OpenAI-compatible API instructions
- [BENCHMARK.md](BENCHMARK.md) — Maple CPU/CUDA measurements

The GitHub release contains compiled binaries, runtime documentation, and the
Rust/Tauri client source under `tauri-app/`. Model weights and development
checkouts remain outside the release repository.

## Requirements

- Windows 10/11 x64
- NVIDIA driver and an NVIDIA GPU for CUDA mode
- Maple `maple-tq2_0.gguf` model downloaded separately from the [Maple GGUF repository](https://huggingface.co/stamsam/maple-preview-gguf)

## Model links

- [Maple-Preview original model](https://huggingface.co/deepgrove/maple-preview)
- [Maple-Preview GGUF conversions](https://huggingface.co/stamsam/maple-preview-gguf)

The runtime runs CPU-only when CUDA is unavailable. CUDA MoE mode is opt-in with `KATALI_CUDA_MOE=1`.

## Desktop GUI

The primary desktop client is `katali-tauri.exe`, built with Rust, Cargo, and
Tauri. It has a conversation view, multiline prompt box, Send button, Clear
history button, CUDA status, and workspace field. It talks to the local
OpenAI-compatible API on port `8119`.

The chat sends the built-in Katali tools to Maple automatically. The available
tools include task_plan, task_checkpoint, calculator, search_files, list_files,
read_file, file_info, edit_file, apply_patch, create_file, make_directory,
copy_file, move_file, git_status, git_diff, git_log, git_show,
git_diff_check, build_project, run_python, run_node, run_rust, run_go, run_cargo,
run_tauri, project_info, format_project, lint_project, test_project,
diff_review, run_process, process_status, cancel_process, run_tests, and guarded
delete_file. File
tools are restricted to one selected workspace, and deletion requires
`confirm=true`. Maple can also create directories, copy and move files, inspect
metadata, and inspect Git history and diff checks.
The workspace field chooses the folder that Maple may read, create,
edit, search, and inspect with Git. The selected folder is sent to the API with
every chat request, so the model and tools share the same environment. The
default is the folder containing `katali-tauri.exe`.

Set up the model at `C:\models\maple-tq2_0.gguf`, then double-click
`run-katali-tauri.bat`. The launcher opens the Rust/Tauri client, which starts
the optimized local API when it first checks health. No Python, .NET runtime,
browser, or separate GUI installation is required.

To try another model in the GUI, set `KATALI_API_MODEL` to its GGUF path before
starting `run-katali-tauri.bat`. The default launcher uses the LFM2 model path
shown in `run-katali-tauri.bat`.

The `katali-tauri.exe` release client moves the desktop window, runtime
lifecycle, workspace binding, conversation reset, and tool request bridge into
Rust. Use `run-katali-tauri.bat` to launch it. The Cargo/Tauri development
project is in `tauri-app/`.

For faster Maple decode on supported NVIDIA GPUs, enable the verified LM-head
resident path in addition to CUDA MoE:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_DENSE_GPU = "1"
$env:KATALI_DENSE_TIER = "lm"
$env:KATALI_MOE_LM_GPU = "1"
```

This keeps only the quantized LM-head matrix on CUDA; attention and routed
experts still use Katali's optimized paths. It is opt-in because performance is
hardware-dependent.

For the fastest verified Maple configuration on the development RTX 4060, use
CUDA attention and the resident LM head together:

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

When KATALI_API_MODEL is set, the API opens the selected model once and reuses a persistent worker for sequential requests.

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_API_MODEL = "C:\models\maple-tq2_0.gguf"
.\katali-lab.exe api --port 8119
```

Then follow [API_TUTORIAL.md](API_TUTORIAL.md).
