$body = @{
    model = "maple"
    messages = @(
        @{ role = "user"; content = "What is the capital of the Philippines?" }
    )
    max_tokens = 16
} | ConvertTo-Json -Depth 5

$port = if ($env:KATALI_API_PORT) { $env:KATALI_API_PORT } else { '8119' }
# The Maple worker loads and warms several gigabytes before its first request.
# Do not send a request during that load phase; the native API can return an
# empty completion while initialization is still in progress.
Start-Sleep -Seconds 15

for ($attempt = 1; $attempt -le 5; $attempt++) {
    $response = Invoke-RestMethod `
        -Uri "http://127.0.0.1:$port/v1/chat/completions" `
        -Method Post `
        -ContentType "application/json" `
        -Body $body

    $answer = [string]$response.choices[0].message.content
    if ($answer -match '(?i)\bManila\b') {
        Write-Output "PASS: $answer"
        exit 0
    }

    Start-Sleep -Seconds 1
}

throw "Maple factual smoke test failed after warm-up. Last response: $answer"
