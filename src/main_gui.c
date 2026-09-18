/* main_gui.c -- native Win32 frontend for the Tractatus engine. */
#include "tractatus.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <richedit.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define ID_INPUT 101
#define ID_GO 102
#define ID_OUT 103
#define ID_TREE 104
#define ID_LANG 105
#define ID_HEADER 106
#define ID_SUBHEAD 107

#define RGB_BG       RGB(30,30,30)
#define RGB_PANEL    RGB(38,38,38)
#define RGB_FIELD    RGB(48,48,48)
#define RGB_FG       RGB(232,232,232)
#define RGB_MUTED    RGB(166,166,166)
#define RGB_BUTTON   RGB(235,219,178)
#define RGB_BUTTON_PRESSED RGB(213,196,161)
#define RGB_BUTTON_TEXT RGB(0,0,0)
#define RGB_YELLOW   RGB(250,189,47)
#define RGB_AQUA     RGB(142,192,124)
#define RGB_BLUE     RGB(131,165,152)
#define RGB_ORANGE   RGB(254,128,25)
#define RGB_GREEN    RGB(184,187,38)
#define RGB_RED      RGB(251,73,52)
#define RGB_BORDER   RGB(112,112,112)

static HFONT g_ui, g_title, g_mono;
static HBRUSH g_bg, g_panel, g_field;
static TrLanguage g_language = TR_ENGLISH;

static void app(char *buf, size_t cap, const char *fmt, ...)
{
    char tmp[8192]; va_list ap; size_t used = strlen(buf);
    if (used >= cap - 1) return;
    va_start(ap, fmt); vsnprintf(tmp, sizeof(tmp), fmt, ap); va_end(ap);
    tmp[sizeof(tmp)-1] = '\0';
    strncat(buf, tmp, cap - used - 1);
}

static wchar_t *utf8_to_wide(const char *text)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t *wide;
    if (n <= 0) return NULL;
    wide = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, (size_t)n * sizeof(wchar_t));
    if (!wide) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, n);
    return wide;
}

static void set_utf8(HWND control, const char *text)
{
    wchar_t *wide = utf8_to_wide(text);
    if (wide) { SetWindowTextW(control, wide); HeapFree(GetProcessHeap(), 0, wide); }
}

static void format_range(HWND control, LONG start, LONG end, COLORREF color, int bold)
{
    CHARRANGE range;
    CHARFORMAT2W fmt;
    range.cpMin = start; range.cpMax = end;
    SendMessageW(control, EM_EXSETSEL, 0, (LPARAM)&range);
    memset(&fmt, 0, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_COLOR | CFM_BOLD;
    fmt.crTextColor = color;
    fmt.dwEffects = bold ? CFE_BOLD : 0;
    SendMessageW(control, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
}

static void color_output(HWND control)
{
    int length = GetWindowTextLengthW(control);
    if (length <= 0) return;
    format_range(control, 0, length, RGB_FG, 0);
    SendMessageW(control, EM_SETSEL, 0, 0);
}

static void get_utf8(HWND control, char *out, size_t cap)
{
    int n = GetWindowTextLengthW(control) + 1;
    wchar_t *wide = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, (size_t)n * sizeof(wchar_t));
    if (!wide) { out[0] = '\0'; return; }
    GetWindowTextW(control, wide, n);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)cap, NULL, NULL);
    out[cap - 1] = '\0'; HeapFree(GetProcessHeap(), 0, wide);
}

static void build_report(const char *input, char *out, size_t cap)
{
    TrAnalysis a; int i;
    out[0] = '\0'; tr_analyse(input, &a);
    app(out, cap, "INPUT\r\n  “%s”\r\n\r\n", input);
    app(out, cap, "CLASSIFICATION\r\n  %s\r\n\r\n", tr_kind_name(a.kind));
    app(out, cap, "STATUS\r\n  %s\r\n  sayable: %s\r\n\r\n",
        a.sense ? "sinnvoll — a bipolar picture" : a.senseless ? "sinnlos — legitimate, but without sense" : "unsinnig — no determinate sense",
        a.sayable ? "yes" : "no — it can only be shown (4.1212)");
    app(out, cap, "RULING\r\n  %s\r\n\r\n", a.verdict);
    app(out, cap, "LOCATION IN THE NUMBERING\r\n");
    for (i = 0; a.chain[i] && i < TR_MAX_CHAIN; i++) {
        const TrProp *p = tr_find(a.chain[i]);
        app(out, cap, "%*s%s─ %s  %s\r\n", i * 3, "", i ? "└" : "◆", a.chain[i], p ? tr_text(p, g_language) : "");
    }
    app(out, cap, "\r\nWHY\r\n  %s\r\n\r\n", a.why);
    app(out, cap, "PICTURE-THEORETIC DECOMPOSITION  (2.13, 4.22, 4.24)\r\n  names:");
    if (!a.name_count) app(out, cap, " none");
    for (i = 0; i < a.name_count; i++) app(out, cap, " “%s”", a.names[i]);
    app(out, cap, "\r\n  asserted form: %s\r\n  true if: %s\r\n  false if: %s\r\n\r\n", a.form, a.truth_true, a.truth_false);
    app(out, cap, "FURTHER RELEVANT REMARKS\r\n");
    for (i = 0; a.hits[i] && i < TR_MAX_HITS; i++) {
        const TrProp *p = tr_find(a.hits[i]);
        if (p) app(out, cap, "  %-8s %s\r\n", p->number, tr_text(p, g_language));
    }
    if (!a.sayable) app(out, cap, "\r\n7  %s\r\n", tr_text(tr_find("7"), g_language));
}

static void build_tree(char *out, size_t cap)
{
    int i, n = tr_prop_count();
    out[0] = '\0';
    app(out, cap, "TRACTATUS LOGICO-PHILOSOPHICUS\r\nCOMPLETE %s TEXT · %d NUMBERED PROPOSITIONS\r\n\r\n",
        g_language == TR_GERMAN ? "ORIGINAL GERMAN" : "OGDEN/RAMSEY ENGLISH", n);
    for (i = 0; i < n; i++) {
        const TrProp *p = tr_prop_at(i); const char *dot = strchr(p->number, '.');
        int depth = dot ? (int)strlen(dot + 1) : 0;
        if (depth == 0 && i) app(out, cap, "\r\n────────────────────────────────────────────────────────────────────────\r\n\r\n");
        if (depth == 0) app(out, cap, "◆ %-8s %s\r\n", p->number, tr_text(p, g_language));
        else app(out, cap, "%*s├─ %-8s %s\r\n", (depth - 1) * 3, "", p->number, tr_text(p, g_language));
    }
}

static void show_report(HWND hwnd)
{
    static char input[TR_MAX_INPUT]; static char out[1048576];
    get_utf8(GetDlgItem(hwnd, ID_INPUT), input, sizeof(input));
    if (!input[0]) return;
    build_report(input, out, sizeof(out)); set_utf8(GetDlgItem(hwnd, ID_OUT), out);
    color_output(GetDlgItem(hwnd, ID_OUT));
}

static void draw_button(const DRAWITEMSTRUCT *di)
{
    char text[64]; RECT r = di->rcItem;
    COLORREF bg = (di->itemState & ODS_SELECTED) ? RGB_BUTTON_PRESSED : RGB_BUTTON;
    COLORREF border = (di->itemState & ODS_FOCUS) ? RGB_FG : RGB_BORDER;
    HBRUSH brush = CreateSolidBrush(bg); HPEN pen = CreatePen(PS_SOLID, 1, border);
    FillRect(di->hDC, &r, brush); SelectObject(di->hDC, pen); Rectangle(di->hDC, r.left, r.top, r.right, r.bottom);
    GetWindowTextA(di->hwndItem, text, sizeof(text));
    SetBkMode(di->hDC, OPAQUE); SetBkColor(di->hDC, bg); SetTextColor(di->hDC, RGB_BUTTON_TEXT); SelectObject(di->hDC, g_ui);
    DrawTextA(di->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (di->itemState & ODS_FOCUS) { InflateRect(&r, -4, -4); DrawFocusRect(di->hDC, &r); }
    DeleteObject(pen); DeleteObject(brush);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        static char out[1048576]; HWND lang;
        g_bg=CreateSolidBrush(RGB_BG); g_panel=CreateSolidBrush(RGB_PANEL); g_field=CreateSolidBrush(RGB_FIELD);
        g_ui=CreateFontW(18,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        g_title=CreateFontW(30,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        g_mono=CreateFontW(17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,FIXED_PITCH,L"Consolas");
        CreateWindowW(L"STATIC",L"Tractatus Logico-Philosophicus Engine",WS_CHILD|WS_VISIBLE,24,18,760,38,hwnd,(HMENU)ID_HEADER,NULL,NULL);
        CreateWindowW(L"STATIC",L"Locate a sentence within the Tractatus",WS_CHILD|WS_VISIBLE,25,58,650,24,hwnd,(HMENU)ID_SUBHEAD,NULL,NULL);
        CreateWindowW(L"EDIT",L"the sky is red",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,24,98,620,36,hwnd,(HMENU)ID_INPUT,NULL,NULL);
        lang=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,654,98,190,220,hwnd,(HMENU)ID_LANG,NULL,NULL);
        SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"English — Ogden/Ramsey"); SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"Deutsch — Original"); SendMessageW(lang,CB_SETCURSEL,0,0);
        CreateWindowW(L"BUTTON",L"Analyze",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,854,98,112,36,hwnd,(HMENU)ID_GO,NULL,NULL);
        CreateWindowW(L"BUTTON",L"Whole book",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,976,98,132,36,hwnd,(HMENU)ID_TREE,NULL,NULL);
        CreateWindowW(MSFTEDIT_CLASS,L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_WANTRETURN,24,154,1084,650,hwnd,(HMENU)ID_OUT,NULL,NULL);
        SendDlgItemMessageW(hwnd,ID_OUT,EM_SETLIMITTEXT,1000000,0);
        SendDlgItemMessageW(hwnd,ID_OUT,EM_SETTARGETDEVICE,0,0);
        SendDlgItemMessageW(hwnd,ID_HEADER,WM_SETFONT,(WPARAM)g_title,TRUE); SendDlgItemMessageW(hwnd,ID_SUBHEAD,WM_SETFONT,(WPARAM)g_ui,TRUE);
        SendDlgItemMessageW(hwnd,ID_INPUT,WM_SETFONT,(WPARAM)g_ui,TRUE); SendDlgItemMessageW(hwnd,ID_LANG,WM_SETFONT,(WPARAM)g_ui,TRUE);
        SendDlgItemMessageW(hwnd,ID_OUT,WM_SETFONT,(WPARAM)g_mono,TRUE);
        SendDlgItemMessageW(hwnd,ID_OUT,EM_SETBKGNDCOLOR,0,RGB_PANEL);
        build_report("the sky is red",out,sizeof(out)); set_utf8(GetDlgItem(hwnd,ID_OUT),out); color_output(GetDlgItem(hwnd,ID_OUT)); return 0;
    }
    case WM_SIZE: {
        int w=LOWORD(lp), h=HIWORD(lp), margin=24;
        SetWindowPos(GetDlgItem(hwnd,ID_INPUT),NULL,margin,98,w-488,36,SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd,ID_LANG),NULL,w-454,98,190,220,SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd,ID_GO),NULL,w-254,98,112,36,SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd,ID_TREE),NULL,w-132,98,108,36,SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd,ID_OUT),NULL,margin,154,w-margin*2,h-178,SWP_NOZORDER); return 0;
    }
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(hwnd,ID_HEADER)) SetTextColor((HDC)wp,RGB_YELLOW);
        else if ((HWND)lp == GetDlgItem(hwnd,ID_SUBHEAD)) SetTextColor((HDC)wp,RGB_MUTED);
        else SetTextColor((HDC)wp,RGB_FG);
        SetBkColor((HDC)wp,RGB_BG); return (LRESULT)g_bg;
    case WM_CTLCOLOREDIT: SetTextColor((HDC)wp,RGB_FG); SetBkColor((HDC)wp,RGB_PANEL); return (LRESULT)g_panel;
    case WM_CTLCOLORLISTBOX: SetTextColor((HDC)wp,RGB_FG); SetBkColor((HDC)wp,RGB_FIELD); return (LRESULT)g_field;
    case WM_ERASEBKGND: { RECT r; GetClientRect(hwnd,&r); FillRect((HDC)wp,&r,g_bg); return 1; }
    case WM_DRAWITEM: draw_button((const DRAWITEMSTRUCT *)lp); return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp)==ID_GO) { show_report(hwnd); return 0; }
        if (LOWORD(wp)==ID_TREE) { static char out[1048576]; build_tree(out,sizeof(out)); set_utf8(GetDlgItem(hwnd,ID_OUT),out); color_output(GetDlgItem(hwnd,ID_OUT)); return 0; }
        if (LOWORD(wp)==ID_LANG && HIWORD(wp)==CBN_SELCHANGE) { g_language=SendDlgItemMessageW(hwnd,ID_LANG,CB_GETCURSEL,0,0)==1?TR_GERMAN:TR_ENGLISH; show_report(hwnd); return 0; }
        break;
    case WM_DESTROY:
        DeleteObject(g_ui); DeleteObject(g_title); DeleteObject(g_mono); DeleteObject(g_bg); DeleteObject(g_panel); DeleteObject(g_field); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

int WINAPI WinMain(HINSTANCE hi,HINSTANCE hp,LPSTR cmd,int show)
{
    WNDCLASSW wc; HWND hwnd; MSG msg; (void)hp; (void)cmd;
    LoadLibraryW(L"Msftedit.dll");
    memset(&wc,0,sizeof(wc)); wc.lpfnWndProc=WndProc; wc.hInstance=hi; wc.lpszClassName=L"TractatusEngine"; wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    RegisterClassW(&wc);
    hwnd=CreateWindowW(L"TractatusEngine",L"Tractatus Logico-Philosophicus Engine",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1160,880,NULL,NULL,hi,NULL);
    ShowWindow(hwnd,show); UpdateWindow(hwnd);
    while(GetMessageW(&msg,NULL,0,0)>0){ if(!IsDialogMessageW(hwnd,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);} }
    return 0;
}
#else
typedef int tractatus_gui_not_on_this_platform;
#endif
