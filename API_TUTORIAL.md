# Katali Maple API tutorial

Katali exposes a loopback HTTP server with an OpenAI-compatible chat route.

## Get the model

Download `maple-tq2_0.gguf` from the [Maple-Preview GGUF repository](https://huggingface.co/stamsam/maple-preview-gguf). The original model card is [deepgrove/maple-preview](https://huggingface.co/deepgrove/maple-preview). Keep the model outside this release repository.

## 1. Start the server

PowerShell:

```powershell
$env:KATALI_CUDA_MOE = "1"
$env:KATALI_API_MODEL = "C:\models\maple-tq2_0.gguf"
.\katali-lab.exe api --port 8119
```

`KATALI_API_MODEL` supplies the default model, so clients do not need to send a local Windows path. An explicit `model` field still overrides it.

The server binds to `127.0.0.1` only.

The server loads the configured model once at startup and reuses the persistent native worker for subsequent requests.

## 2. Check health

```powershell
Invoke-RestMethod http://127.0.0.1:8119/health
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
  -Uri http://127.0.0.1:8119/v1/chat/completions `
  -Method Post `
  -ContentType "application/json" `
  -Body $body
```

The generated text is returned in `choices[0].message.content`.

## 5. Use Katali tools

Send an OpenAI-style `tools` array to enable the local tool loop. Katali lets
Maple call a tool, executes it, sends the result back to Maple, and returns the
final answer. The built-in tools are `calculator`, `search_files`, `list_files`,
`read_file`, `edit_file`, `create_file`, `git_status`, `git_diff`, `run_tests`,
and `delete_file`.

Example:

```powershell
$body = @{
  model = "maple"
  messages = @(
    @{ role = "user"; content = "Calculate 37.2% of 8412 using the calculator tool." }
  )
  tools = @(
    @{
      type = "function"
      function = @{
        name = "calculator"
        description = "Evaluate arithmetic or a percentage."
        parameters = @{
          type = "object"
          properties = @{ expression = @{ type = "string" } }
          required = @("expression")
        }
      }
    }
  )
  tool_choice = "auto"
  max_tokens = 64
} | ConvertTo-Json -Depth 10

Invoke-RestMethod `
  -Uri http://127.0.0.1:8119/v1/chat/completions `
  -Method Post `
  -ContentType "application/json" `
  -Body $body
```

File tools are limited to the workspace directory containing the launcher.
`edit_file` performs controlled search-and-replace and reports the replacement
count. It accepts `path`, `find`, `replace`, and optional `replace_all` fields.
`search_files` searches text files under the workspace. `git_status` and
`git_diff` are read-only repository tools. `run_tests` only accepts the
allowlisted `selftest` suite.
`delete_file` refuses to run unless its arguments include `confirm: true`.

## 4. Use curl

```powershell
curl.exe -X POST http://127.0.0.1:8119/v1/chat/completions `
  -H "Content-Type: application/json" `
  --data-raw '{"messages":[{"role":"user","content":"What is 2 plus 2?"}],"max_tokens":8}'
```

Streaming is available with `"stream":true`; the response uses `text/event-stream` and ends with `data: [DONE]`.

## Troubleshooting

- If port 8119 is busy, choose another available port and set `KATALI_API_PORT` for the launcher and client.
- If health reports CPU-only mode, confirm the NVIDIA driver, `katali_cuda.dll`, and `cudart64_13.dll` are beside the executable.
- Keep model weights outside this repository; the API reads them from the path in `KATALI_API_MODEL`.
