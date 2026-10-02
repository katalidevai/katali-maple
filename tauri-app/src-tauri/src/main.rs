#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::Serialize;
use serde_json::{json, Value};
use std::io::{Read, Write};
use std::net::{Shutdown, TcpStream};
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::Mutex;
use std::time::Duration;
use tauri::{AppHandle, Manager, State};

const PORT: u16 = 8119;

struct RuntimeState {
    api: Mutex<Option<Child>>,
}

impl Default for RuntimeState {
    fn default() -> Self {
        Self {
            api: Mutex::new(None),
        }
    }
}

#[derive(Serialize)]
struct ChatResult {
    answer: String,
    raw: Value,
}

fn engine_path(app: &AppHandle) -> PathBuf {
    if let Ok(path) = std::env::var("KATALI_ENGINE_EXE") {
        return PathBuf::from(path);
    }
    if let Ok(dir) = app.path().resource_dir() {
        let packaged = dir.join("katali-lab.exe");
        if packaged.exists() {
            return packaged;
        }
    }
    let dev = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("katali-lab.exe");
    if dev.exists() {
        return dev;
    }
    std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.join("katali-lab.exe")))
        .unwrap_or(dev)
}

fn model_path() -> String {
    std::env::var("KATALI_API_MODEL")
        .unwrap_or_else(|_| r"C:\models\LFM2-24B-A2B-Q4_K_M.gguf".to_string())
}

fn http_request(method: &str, path: &str, body: Option<&str>) -> Result<String, String> {
    let mut stream = TcpStream::connect(("127.0.0.1", PORT))
        .map_err(|e| format!("Katali API is not reachable: {e}"))?;
    stream
        .set_read_timeout(Some(Duration::from_secs(180)))
        .map_err(|e| e.to_string())?;
    stream
        .set_write_timeout(Some(Duration::from_secs(10)))
        .map_err(|e| e.to_string())?;
    let payload = body.unwrap_or("");
    let request = format!(
        "{method} {path} HTTP/1.1\r\nHost: 127.0.0.1:{PORT}\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
        payload.as_bytes().len(),
        payload
    );
    stream
        .write_all(request.as_bytes())
        .map_err(|e| format!("Could not send request: {e}"))?;
    let _ = stream.shutdown(Shutdown::Write);
    let mut response = Vec::new();
    stream
        .read_to_end(&mut response)
        .map_err(|e| format!("Could not read API response: {e}"))?;
    let text = String::from_utf8_lossy(&response);
    let (head, content) = text
        .split_once("\r\n\r\n")
        .ok_or_else(|| "Malformed response from Katali API".to_string())?;
    if !head.starts_with("HTTP/1.1 2") && !head.starts_with("HTTP/1.0 2") {
        return Err(content.trim().to_string());
    }
    Ok(content.to_string())
}

fn api_ready() -> bool {
    http_request("GET", "/health", None).is_ok()
}

fn ensure_api(app: &AppHandle, state: &RuntimeState, workspace: &str) -> Result<(), String> {
    if api_ready() {
        return Ok(());
    }
    let mut child = state
        .api
        .lock()
        .map_err(|_| "API state lock poisoned".to_string())?;
    if child.is_none() {
        let exe = engine_path(app);
        if !exe.exists() {
            return Err(format!("Katali engine not found at {}", exe.display()));
        }
        let root = if Path::new(workspace).is_dir() {
            workspace.to_string()
        } else {
            exe.parent().unwrap_or(Path::new(".")).display().to_string()
        };
        let process = Command::new(&exe)
            .arg("api")
            .arg("--port")
            .arg(PORT.to_string())
            .env("KATALI_CUDA_MOE", "1")
            .env("KATALI_CUDA_MOE_ASYNC", "1")
            .env("KATALI_CUDA_DP4A", "1")
            .env("KATALI_VRAM_GB", "6.5")
            .env("KATALI_EC_WARMUP", "0")
            .env("KATALI_API_MODEL", model_path())
            .env("KATALI_TOOL_ROOT", &root)
            .current_dir(&root)
            .stdin(Stdio::null())
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()
            .map_err(|e| format!("Could not start Katali engine: {e}"))?;
        *child = Some(process);
    }
    drop(child);
    for _ in 0..120 {
        if api_ready() {
            return Ok(());
        }
        std::thread::sleep(Duration::from_millis(250));
    }
    Err("Katali API did not become ready within 30 seconds".to_string())
}

fn tool_definitions() -> Value {
    json!([
        {"type":"function","function":{"name":"calculator","description":"Evaluate arithmetic expressions.","parameters":{"type":"object","properties":{"expression":{"type":"string"}},"required":["expression"]}}},
        {"type":"function","function":{"name":"list_files","description":"List files and folders inside the selected workspace.","parameters":{"type":"object","properties":{"path":{"type":"string"},"recursive":{"type":"boolean"}},"required":["path"]}}},
        {"type":"function","function":{"name":"read_file","description":"Read a text file inside the selected workspace.","parameters":{"type":"object","properties":{"path":{"type":"string"}},"required":["path"]}}},
        {"type":"function","function":{"name":"create_file","description":"Create or replace a file inside the selected workspace.","parameters":{"type":"object","properties":{"path":{"type":"string"},"content":{"type":"string"}},"required":["path","content"]}}},
        {"type":"function","function":{"name":"edit_file","description":"Apply a controlled search and replace inside a workspace file.","parameters":{"type":"object","properties":{"path":{"type":"string"},"find":{"type":"string"},"replace":{"type":"string"},"replace_all":{"type":"boolean"}},"required":["path","find","replace"]}}},
        {"type":"function","function":{"name":"apply_patch","description":"Apply precise Update File patches inside the workspace.","parameters":{"type":"object","properties":{"patch":{"type":"string"}},"required":["patch"]}}},
        {"type":"function","function":{"name":"delete_file","description":"Delete one file after explicit confirmation=true.","parameters":{"type":"object","properties":{"path":{"type":"string"},"confirm":{"type":"boolean"}},"required":["path","confirm"]}}},
        {"type":"function","function":{"name":"project_info","description":"Inspect project manifests and detected languages.","parameters":{"type":"object","properties":{}}}},
        {"type":"function","function":{"name":"run_cargo","description":"Run an allowlisted Cargo command: check, test, build, run, or metadata.","parameters":{"type":"object","properties":{"command":{"type":"string"}},"required":["command"]}}},
        {"type":"function","function":{"name":"run_tauri","description":"Run an allowlisted Tauri command: check, build, or info.","parameters":{"type":"object","properties":{"command":{"type":"string"}},"required":["command"]}}},
        {"type":"function","function":{"name":"task_plan","description":"Create a short plan for a multi-step task.","parameters":{"type":"object","properties":{"steps":{"type":"array","items":{"type":"string"}}},"required":["steps"]}}},
        {"type":"function","function":{"name":"task_checkpoint","description":"Record progress and the next checkpoint.","parameters":{"type":"object","properties":{"completed":{"type":"string"},"next":{"type":"string"}},"required":["completed","next"]}}}
    ])
}

#[tauri::command]
fn health(
    app: AppHandle,
    state: State<'_, RuntimeState>,
    workspace: String,
) -> Result<Value, String> {
    ensure_api(&app, &state, &workspace)?;
    let body = http_request("GET", "/health", None)?;
    serde_json::from_str(&body).map_err(|e| e.to_string())
}

#[tauri::command]
fn send_chat(
    app: AppHandle,
    state: State<'_, RuntimeState>,
    message: String,
    workspace: String,
    conversation_id: String,
    reset: bool,
) -> Result<ChatResult, String> {
    ensure_api(&app, &state, &workspace)?;
    let request = json!({
        "model": "maple",
        "workspace": workspace,
        "conversation_id": conversation_id,
        "reset": reset,
        "prompt": message,
        "messages": [{"role":"user","content":message}],
        "tools": tool_definitions(),
        "tool_choice": "auto",
        "max_tokens": 512
    });
    let body = http_request("POST", "/v1/chat/completions", Some(&request.to_string()))?;
    let raw: Value = serde_json::from_str(&body).map_err(|e| format!("Invalid API JSON: {e}"))?;
    let answer = raw
        .pointer("/choices/0/message/content")
        .and_then(Value::as_str)
        .or_else(|| raw.pointer("/choices/0/text").and_then(Value::as_str))
        .unwrap_or("")
        .to_string();
    Ok(ChatResult { answer, raw })
}

fn main() {
    tauri::Builder::default()
        .manage(RuntimeState::default())
        .invoke_handler(tauri::generate_handler![health, send_chat])
        .run(tauri::generate_context!())
        .expect("error while running Katali Tauri");
}
