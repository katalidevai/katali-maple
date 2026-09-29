# Katali Maple API tutorial

Katali exposes a loopback HTTP server with an OpenAI-compatible chat route.

## Get the model

Download `maple-tq2_0.gguf` from the [Maple-Preview GGUF repository](https://huggingface.co/stamsam/maple-preview-gguf). The original model card is [deepgrove/maple-preview](https://huggingface.co/deepgrove/maple-preview). Keep the model outside this release repository.

## 1. Start the server

PowerShell:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_API_MODEL = "C:\models\maple-tq2_0.gguf"
.\katali-lab.exe api --port 8080
```

`KATALI_API_MODEL` supplies the default model, so clients do not need to send a local Windows path. An explicit `model` field still overrides it.

The server binds to `127.0.0.1` only.

The server loads the configured model once at startup and reuses the persistent native worker for subsequent requests.

## 2. Check health

```powershell
Invoke-RestMethod http://127.0.0.1:8080/health
```

CUDA mode should report `cpu_only: false` and `cuda_device_usable: true`.

## 3. Send a chat request

```powershell
$body = @{
  model = "maple"
  messages = @(
    @{ role = "user"; content = "Say hello in one short sentence." }
  )
  max_tokens = 32
} | ConvertTo-Json -Depth 5

Invoke-RestMethod `
  -Uri http://127.0.0.1:8080/v1/chat/completions `
  -Method Post `
  -ContentType "application/json" `
  -Body $body
```

The generated text is returned in `choices[0].message.content`.

## 4. Use curl

```powershell
curl.exe -X POST http://127.0.0.1:8080/v1/chat/completions `
  -H "Content-Type: application/json" `
  --data-raw '{"messages":[{"role":"user","content":"What is 2 plus 2?"}],"max_tokens":8}'
```

Streaming is available with `"stream":true`; the response uses `text/event-stream` and ends with `data: [DONE]`.

## Troubleshooting

- If port 8080 is busy, choose another port, such as `--port 8091`.
- If health reports CPU-only mode, confirm the NVIDIA driver, `katali_cuda.dll`, and `cudart64_13.dll` are beside the executable.
- Keep model weights outside this repository; the API reads them from the path in `KATALI_API_MODEL`.
