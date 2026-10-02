const invoke = window.__TAURI__?.core?.invoke;
const messages = document.querySelector('#messages');
const promptBox = document.querySelector('#prompt');
const workspaceBox = document.querySelector('#workspace');
const sendButton = document.querySelector('#send');
const statusText = document.querySelector('#status-text');
const status = document.querySelector('.status');
const browseButton = document.querySelector('#browse');
const dialogOpen = window.__TAURI__?.dialog?.open;
let conversationId = `tauri-${Date.now()}`;
let resetNext = false;

function addMessage(role, text) {
  document.querySelector('.welcome')?.remove();
  const row = document.createElement('div'); row.className = `msg ${role}`;
  const avatar = document.createElement('div'); avatar.className = 'avatar'; avatar.textContent = role === 'user' ? 'You' : 'K';
  const bubble = document.createElement('div'); bubble.className = 'bubble'; bubble.textContent = text;
  row.append(avatar, bubble); messages.append(row); messages.scrollTop = messages.scrollHeight;
}
function setStatus(text, kind='') { statusText.textContent = text; status.className = `status ${kind}`; }
async function checkHealth() {
  if (!invoke) { setStatus('Tauri runtime unavailable', 'error'); return; }
  setStatus('Starting engine…');
  try { const h = await invoke('health', { workspace: workspaceBox.value || '.' }); setStatus(h.cuda_device_usable ? 'CUDA ready' : 'CPU mode', 'ready'); }
  catch (e) { setStatus(String(e), 'error'); }
}
document.querySelector('#health').addEventListener('click', checkHealth);
browseButton.addEventListener('click', async () => {
  if (!dialogOpen) { setStatus('Folder picker unavailable', 'error'); return; }
  try {
    const selected = await dialogOpen({ directory: true, multiple: false, title: 'Select workspace folder' });
    if (typeof selected === 'string' && selected) {
      workspaceBox.value = selected;
      await checkHealth();
    }
  } catch (e) { setStatus(String(e), 'error'); }
});
document.querySelector('#clear').addEventListener('click', () => { messages.innerHTML = ''; conversationId = `tauri-${Date.now()}`; resetNext = true; addMessage('assistant', 'History cleared. What should we do next?'); });
document.querySelector('#composer').addEventListener('submit', async (event) => {
  event.preventDefault(); const message = promptBox.value.trim(); if (!message || !invoke) return;
  addMessage('user', message); promptBox.value = ''; sendButton.disabled = true; setStatus('Thinking…');
  try { const result = await invoke('send_chat', { message, workspace: workspaceBox.value || '.', conversationId, reset: resetNext }); resetNext = false; addMessage('assistant', result.answer || '(No text returned)'); setStatus('CUDA ready', 'ready'); }
  catch (e) { addMessage('assistant', `Error: ${e}`); setStatus('Error', 'error'); }
  finally { sendButton.disabled = false; promptBox.focus(); }
});
promptBox.addEventListener('keydown', (event) => { if (event.key === 'Enter' && (event.ctrlKey || event.metaKey)) { event.preventDefault(); document.querySelector('#composer').requestSubmit(); } });
checkHealth();
