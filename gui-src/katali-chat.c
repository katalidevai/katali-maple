#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winhttp.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>

#define ID_LOG     1001
#define ID_INPUT   1002
#define ID_SEND    1003
#define ID_STATUS  1004
#define ID_CLEAR   1005
#define WM_CHAT_DONE (WM_APP + 1)

typedef struct ChatResult {
    HWND hwnd;
    wchar_t *prompt;
    wchar_t *answer;
    wchar_t *error;
} ChatResult;

static HWND g_log, g_input, g_send, g_clear, g_status;
static HFONT g_font;

static const char TOOL_DEFS[] =
    "[{\"type\":\"function\",\"function\":{\"name\":\"calculator\",\"description\":\"Evaluate arithmetic or a percentage.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"expression\":{\"type\":\"string\"}},\"required\":[\"expression\"]}}},"
    "{\"type\":\"function\",\"function\":{\"name\":\"list_files\",\"description\":\"List files and folders under the Katali workspace.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"recursive\":{\"type\":\"boolean\"}},\"required\":[\"path\"]}}},"
    "{\"type\":\"function\",\"function\":{\"name\":\"read_file\",\"description\":\"Read a text file under the Katali workspace.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"}},\"required\":[\"path\"]}}},"
    "{\"type\":\"function\",\"function\":{\"name\":\"create_file\",\"description\":\"Create or replace a file under the Katali workspace.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"content\":{\"type\":\"string\"}},\"required\":[\"path\",\"content\"]}}},"
    "{\"type\":\"function\",\"function\":{\"name\":\"delete_file\",\"description\":\"Delete one file under the Katali workspace after explicit confirmation.\",\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"confirm\":{\"type\":\"boolean\"}},\"required\":[\"path\",\"confirm\"]}}}]";

static wchar_t *wdup(const wchar_t *s) {
    size_t n = s ? wcslen(s) : 0;
    wchar_t *p = (wchar_t *)calloc(n + 1, sizeof(wchar_t));
    if (p && s) memcpy(p, s, n * sizeof(wchar_t));
    return p;
}

static char *utf8_from_wide(const wchar_t *s) {
    int n;
    char *out;
    if (!s) return NULL;
    n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return NULL;
    out = (char *)malloc((size_t)n);
    if (!out) return NULL;
    if (!WideCharToMultiByte(CP_UTF8, 0, s, -1, out, n, NULL, NULL)) {
        free(out); return NULL;
    }
    return out;
}

static wchar_t *wide_from_utf8(const char *s, size_t n) {
    int need;
    wchar_t *out;
    if (!s) return NULL;
    if (n > 0x7fffffffU) return NULL;
    need = MultiByteToWideChar(CP_UTF8, 0, s, (int)n, NULL, 0);
    if (need <= 0) return wdup(L"");
    out = (wchar_t *)calloc((size_t)need + 1, sizeof(wchar_t));
    if (!out) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, s, (int)n, out, need);
    return out;
}

static char hex_digit(unsigned v) { return v < 10 ? (char)('0' + v) : (char)('a' + v - 10); }

static char *json_escape(const char *s) {
    size_t i, n = 0;
    char *o, *p;
    if (!s) return NULL;
    for (i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        n += (c == '"' || c == '\\' || c < 0x20) ? 2 : 1;
        if (c < 0x20) n += 5;
    }
    o = (char *)malloc(n + 1);
    if (!o) return NULL;
    p = o;
    for (i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') { *p++ = '\\'; *p++ = (char)c; }
        else if (c == '\n') { memcpy(p, "\\n", 2); p += 2; }
        else if (c == '\r') { memcpy(p, "\\r", 2); p += 2; }
        else if (c == '\t') { memcpy(p, "\\t", 2); p += 2; }
        else if (c < 0x20) {
            *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
            *p++ = hex_digit(c >> 4); *p++ = hex_digit(c & 15);
        } else *p++ = (char)c;
    }
    *p = 0;
    return o;
}

static char *read_http_body(HINTERNET req, DWORD *status_out) {
    DWORD status = 0, status_len = sizeof(status);
    char *body = NULL;
    size_t used = 0, cap = 0;
    if (WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_len, NULL) != TRUE)
        status = 0;
    for (;;) {
        DWORD avail = 0, got = 0;
        if (!WinHttpQueryDataAvailable(req, &avail)) { free(body); return NULL; }
        if (!avail) break;
        if (used + (size_t)avail + 1 > cap) {
            size_t nc = cap ? cap * 2 : 4096;
            while (nc < used + (size_t)avail + 1) nc *= 2;
            body = (char *)realloc(body, nc);
            if (!body) return NULL;
            cap = nc;
        }
        if (!WinHttpReadData(req, body + used, avail, &got)) { free(body); return NULL; }
        used += got;
        if (!got) break;
    }
    if (!body) body = (char *)calloc(1, 1);
    else body[used] = 0;
    if (status_out) *status_out = status;
    return body;
}

static char *extract_content(const char *json) {
    const char *p = strstr(json ? json : "", "\"content\"");
    char *out;
    size_t cap = 256, used = 0;
    if (!p) return NULL;
    p = strchr(p, ':');
    if (!p) return NULL;
    while (*p && *p != '"') p++;
    if (*p != '"') return NULL;
    p++;
    out = (char *)malloc(cap);
    if (!out) return NULL;
    while (*p && *p != '"') {
        unsigned char c = (unsigned char)*p++;
        if (c == '\\' && *p) {
            char e = *p++;
            if (e == 'n') c = '\n'; else if (e == 'r') c = '\r';
            else if (e == 't') c = '\t'; else c = (unsigned char)e;
        }
        if (used + 2 > cap) { cap *= 2; out = (char *)realloc(out, cap); if (!out) return NULL; }
        out[used++] = (char)c;
    }
    out[used] = 0;
    return out;
}

static wchar_t *chat_request(const wchar_t *prompt, wchar_t **error_out) {
    char *u8 = NULL, *esc = NULL, *json = NULL, *body = NULL;
    wchar_t *answer = NULL;
    HINTERNET ses = NULL, con = NULL, req = NULL;
    DWORD status = 0;
    size_t json_n;
    u8 = utf8_from_wide(prompt);
    esc = json_escape(u8);
    if (!esc) goto fail;
    json_n = strlen(esc) + strlen(TOOL_DEFS) + 256;
    json = (char *)malloc(json_n);
    if (!json) goto fail;
    _snprintf_s(json, json_n, _TRUNCATE,
                "{\"model\":\"maple\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"tools\":%s,\"tool_choice\":\"auto\",\"max_tokens\":512}",
                esc, TOOL_DEFS);

    ses = WinHttpOpen(L"KataliChat/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    con = ses ? WinHttpConnect(ses, L"127.0.0.1", 8119, 0) : NULL;
    req = con ? WinHttpOpenRequest(con, L"POST", L"/v1/chat/completions", NULL,
                                   WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0) : NULL;
    if (!req) goto fail;
    if (!WinHttpAddRequestHeaders(req, L"Content-Type: application/json\r\n", (DWORD)-1L,
                                  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) goto fail;
    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, json,
                            (DWORD)strlen(json), (DWORD)strlen(json), 0)) goto fail;
    if (!WinHttpReceiveResponse(req, NULL)) goto fail;
    body = read_http_body(req, &status);
    if (!body) goto fail;
    if (status < 200 || status >= 300) {
        if (error_out) *error_out = wide_from_utf8(body, strlen(body));
        goto done;
    }
    {
        char *content = extract_content(body);
        if (content) answer = wide_from_utf8(content, strlen(content));
        free(content);
    }
    if (!answer && error_out) *error_out = wdup(L"The API returned an unexpected response.");
done:
    free(body); free(json); free(esc); free(u8);
    if (req) WinHttpCloseHandle(req); if (con) WinHttpCloseHandle(con); if (ses) WinHttpCloseHandle(ses);
    return answer;
fail:
    if (error_out) *error_out = wdup(L"Could not connect to Katali on 127.0.0.1:8119. Start start-maple-api.bat first.");
    goto done;
}

static void append(HWND h, const wchar_t *s) {
    SendMessageW(h, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageW(h, EM_REPLACESEL, FALSE, (LPARAM)s);
}

static void clear_history(void) {
    SetWindowTextW(g_log, L"Katali Maple\r\nReady.\r\n");
    SetWindowTextW(g_status, L"Ready  |  API 127.0.0.1:8119");
    SetFocus(g_input);
}

static unsigned __stdcall chat_thread(void *arg) {
    ChatResult *r = (ChatResult *)arg;
    r->answer = chat_request(r->prompt, &r->error);
    PostMessageW(r->hwnd, WM_CHAT_DONE, 0, (LPARAM)r);
    return 0;
}

static void send_prompt(HWND hwnd) {
    int n = GetWindowTextLengthW(g_input);
    ChatResult *r;
    if (n <= 0) return;
    r = (ChatResult *)calloc(1, sizeof(*r));
    if (!r) return;
    r->hwnd = hwnd;
    r->prompt = (wchar_t *)calloc((size_t)n + 1, sizeof(wchar_t));
    if (!r->prompt) { free(r); return; }
    GetWindowTextW(g_input, r->prompt, n + 1);
    append(g_log, L"\r\nYou\r\n"); append(g_log, r->prompt); append(g_log, L"\r\n\r\n");
    SetWindowTextW(g_input, L"");
    EnableWindow(g_send, FALSE);
    EnableWindow(g_clear, FALSE);
    SetWindowTextW(g_status, L"Thinking...  GPU Maple");
    uintptr_t th = _beginthreadex(NULL, 0, chat_thread, r, 0, NULL);
    if (th) CloseHandle((HANDLE)th);
    else {
        EnableWindow(g_send, TRUE);
        EnableWindow(g_clear, TRUE);
        SetWindowTextW(g_status, L"Unable to start request");
    }
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Katali Maple\r\nReady.\r\n",
                                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
                                ES_AUTOVSCROLL | WS_VSCROLL, 0, 0, 0, 0, hwnd,
                                (HMENU)ID_LOG, NULL, NULL);
        g_input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
                                  WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)ID_INPUT, NULL, NULL);
        g_send = CreateWindowW(L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                               0, 0, 0, 0, hwnd, (HMENU)ID_SEND, NULL, NULL);
        g_clear = CreateWindowW(L"BUTTON", L"Clear history", WS_CHILD | WS_VISIBLE,
                                0, 0, 0, 0, hwnd, (HMENU)ID_CLEAR, NULL, NULL);
        g_status = CreateWindowW(L"STATIC", L"Ready  |  API 127.0.0.1:8119",
                                 WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                                 (HMENU)ID_STATUS, NULL, NULL);
        SendMessageW(g_log, WM_SETFONT, (WPARAM)g_font, TRUE);
        SendMessageW(g_input, WM_SETFONT, (WPARAM)g_font, TRUE);
        SendMessageW(g_send, WM_SETFONT, (WPARAM)g_font, TRUE);
        SendMessageW(g_clear, WM_SETFONT, (WPARAM)g_font, TRUE);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)g_font, TRUE);
        SetFocus(g_input);
        return 0;
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp), bottom = 112;
        int button_w = 104, gap = 8;
        int input_w = w - 24 - button_w * 2 - gap * 3;
        if (input_w < 160) input_w = 160;
        MoveWindow(g_log, 12, 12, w - 24, h - bottom - 24, TRUE);
        MoveWindow(g_input, 12, h - bottom + 4, input_w, bottom - 42, TRUE);
        MoveWindow(g_send, 12 + input_w + gap, h - bottom + 4,
                   button_w, bottom - 42, TRUE);
        MoveWindow(g_clear, 12 + input_w + gap * 2 + button_w,
                   h - bottom + 4, button_w + gap, bottom - 42, TRUE);
        MoveWindow(g_status, 12, h - 28, w - 24, 22, TRUE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == ID_SEND && HIWORD(wp) == BN_CLICKED) send_prompt(hwnd);
        if (LOWORD(wp) == ID_CLEAR && HIWORD(wp) == BN_CLICKED) clear_history();
        return 0;
    case WM_CHAT_DONE: {
        ChatResult *r = (ChatResult *)lp;
        if (r->answer) { append(g_log, L"Maple\r\n"); append(g_log, r->answer); append(g_log, L"\r\n"); SetWindowTextW(g_status, L"Ready  |  GPU Maple"); }
        else { append(g_log, L"Error\r\n"); append(g_log, r->error ? r->error : L"Unknown error"); append(g_log, L"\r\n"); SetWindowTextW(g_status, L"API connection error"); }
        EnableWindow(g_send, TRUE);
        EnableWindow(g_clear, TRUE);
        free(r->prompt); free(r->answer); free(r->error); free(r);
        SetFocus(g_input);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *info = (MINMAXINFO *)lp;
        info->ptMinTrackSize.x = 640;
        info->ptMinTrackSize.y = 460;
        return 0;
    }
    case WM_DESTROY:
        if (g_font) DeleteObject(g_font);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show) {
    WNDCLASSW wc;
    MSG msg;
    HWND hwnd;
    (void)prev; (void)cmd;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = wnd_proc; wc.hInstance = inst;
    wc.lpszClassName = L"KataliMapleChat";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    hwnd = CreateWindowExW(0, wc.lpszClassName, L"Katali Maple Chat",
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
                           NULL, NULL, inst, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return (int)msg.wParam;
}
