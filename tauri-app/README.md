# Katali Maple Tauri client

This is the Rust/Tauri desktop client for Katali Maple. The Rust process owns
the desktop window, engine lifecycle, local API bridge, workspace selection,
conversation reset, and tool schema while the release uses the bundled Katali
runtime and optional CUDA backend for model execution.

## Development

From this directory:

```powershell
cargo tauri dev
```

The app starts the native engine on `127.0.0.1:8119` when the first health or
chat command runs. Set `KATALI_API_MODEL` to use another GGUF file. The default
is `C:\models\LFM2-24B-A2B-Q4_K_M.gguf`.

## Release build

```powershell
cargo tauri build --no-bundle
```

The verified release executable is written to
`src-tauri/target/release/katali-tauri.exe`. A normal Tauri bundle can be
produced with `cargo tauri build` when NSIS is installed.

The selected workspace is passed to the native API on every request. File and
project tools therefore stay inside the same workspace that the user sees in
the chat window.
