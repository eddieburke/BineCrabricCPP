#include <windows.h>
#include <commdlg.h>
#include <urlmon.h>
int a,s,m;wchar_t j[4096],d[MAX_PATH];
void P(wchar_t*c){STARTUPINFOW si={sizeof si};PROCESS_INFORMATION p={};if(CreateProcessW(0,c,0,0,0,CREATE_NO_WINDOW,0,0,&si,&p))WaitForSingleObject(p.hProcess,-1),CloseHandle(p.hThread),CloseHandle(p.hProcess);}
void D(HWND h,LPCWSTR n,LPCWSTR f,LPCWSTR q){wchar_t t[MAX_PATH],z[MAX_PATH],u[MAX_PATH],c[4096];GetTempPathW(MAX_PATH,t);GetTempFileNameW(t,L"mc",0,z);DeleteFileW(z);lstrcatW(z,L".zip");wsprintfW(u,L"https://github.com/eddieburke/BineCrabricCPP/releases/latest/download/%s.zip",n);URLDownloadToFileW(0,u,z,0,0);wsprintfW(c,L"powershell -c \"Expand-Archive '%s' '%s..\\%s\\' -Force\"",z,d,f);P(c);DeleteFileW(z);PostMessageW(h,WM_APP+1,0,(LPARAM)q);}
DWORD WINAPI T(void*v){HWND h=(HWND)v;wchar_t c[4096];if(a){wsprintfW(c,L"powershell -c \"$r=$env:APPDATA+'\\.minecraft\\resources';mkdir $r -fo>$null;add-type -As System.IO.Compression.FileSystem;try{[IO.Compression.ZipFile]::ExtractToDirectory('%s',$r)}catch{};ri ($r+'\\META-INF') -r -fo -ea 0;ri ($r+'\\*.class') -r -fo -ea 0\"",j);P(c);PostMessageW(h,WM_APP+1,0,(LPARAM)L"Resources extracted.");}if(s)D(h,L"shaders",L"shaderpacks",L"Shaders downloaded.");if(m)D(h,L"mods",L"mods",L"Mods downloaded.");PostMessageW(h,WM_APP+1,1,0);return 0;}
void B(HWND h){OPENFILENAMEW o={sizeof o};o.hwndOwner=h;o.lpstrFile=j;o.nMaxFile=4096;o.lpstrFilter=L"JAR\0*.jar\0";o.Flags=OFN_FILEMUSTEXIST;GetOpenFileNameW(&o)&&SetWindowTextW(GetDlgItem(h,101),j);}
void I(HWND h){GetWindowTextW(GetDlgItem(h,101),j,4096);a=IsDlgButtonChecked(h,103);s=IsDlgButtonChecked(h,104);m=IsDlgButtonChecked(h,105);EnableWindow(GetDlgItem(h,106),0);EnableWindow(GetDlgItem(h,102),0);SetWindowTextW(GetDlgItem(h,107),L"Installing...");CloseHandle(CreateThread(0,0,T,h,0,0));}

// Modern-ish visual style via WM_CTLCOLORSTATIC / WM_CTLCOLORBTN
HFONT gFont=0,gFontSm=0;
HBRUSH gBg=0;

void MkFont(){
    LOGFONTW lf={};
    lf.lfHeight=-13;lf.lfWeight=FW_NORMAL;lf.lfQuality=CLEARTYPE_QUALITY;
    lstrcpyW(lf.lfFaceName,L"Segoe UI");
    gFont=CreateFontIndirectW(&lf);
    lf.lfHeight=-11;lf.lfWeight=FW_NORMAL;
    lstrcpyW(lf.lfFaceName,L"Segoe UI");
    gFontSm=CreateFontIndirectW(&lf);
}

void C(HWND h,LPCWSTR c,LPCWSTR t,int x,int y,int X,int Y,int n,DWORD ex=0){
    DWORD sty=WS_CHILD|WS_VISIBLE;
    if(n==101)sty|=WS_BORDER|ES_AUTOHSCROLL;
    else if(n>=103&&n<=105)sty|=BS_AUTOCHECKBOX;
    else if(n==102||n==106)sty|=0;
    HWND w=CreateWindowExW(ex,c,t,sty,x,y,X,Y,h,(HMENU)(INT_PTR)n,0,0);
    if(gFont)SendMessageW(w,WM_SETFONT,(WPARAM)gFont,1);
}

LRESULT CALLBACK W(HWND h,UINT q,WPARAM x,LPARAM y){
    switch(q){
    case WM_CREATE:{
        MkFont();
        gBg=CreateSolidBrush(0x1e1e1e);
        // Title area label
        HWND hT=CreateWindowExW(0,L"STATIC",L"Minecraft Installer",WS_CHILD|WS_VISIBLE|SS_CENTER,0,0,440,40,h,(HMENU)200,0,0);
        LOGFONTW lf={};lf.lfHeight=-18;lf.lfWeight=FW_SEMIBOLD;lf.lfQuality=CLEARTYPE_QUALITY;lstrcpyW(lf.lfFaceName,L"Segoe UI");
        SendMessageW(hT,WM_SETFONT,(WPARAM)CreateFontIndirectW(&lf),1);

        C(h,L"STATIC",L"Path to minecraft.jar",10,50,420,16,0);
        C(h,L"EDIT",L"",10,68,315,24,101);
        C(h,L"BUTTON",L"Browse...",330,68,100,24,102);
        C(h,L"BUTTON",L"Extract resources from minecraft.jar",10,102,380,20,103);
        C(h,L"BUTTON",L"Download shaders pack from GitHub",10,126,380,20,104);
        C(h,L"BUTTON",L"Download mods from GitHub",10,150,380,20,105);
        C(h,L"BUTTON",L"Install",10,182,120,28,106);

        // Status
        HWND hS=CreateWindowExW(0,L"STATIC",L"Waiting.",WS_CHILD|WS_VISIBLE,10,220,420,18,h,(HMENU)107,0,0);
        SendMessageW(hS,WM_SETFONT,(WPARAM)gFont,1);

        HWND hD=CreateWindowExW(0,L"STATIC",
            L"This may need to run with administrator privileges.  "
            L"You can review the source at src\\installer\\installer.cpp \u2014 "
            L"or paste it into an AI if you want a second opinion.",
            WS_CHILD|WS_VISIBLE,10,246,420,36,h,(HMENU)201,0,0);
        SendMessageW(hD,WM_SETFONT,(WPARAM)gFontSm,1);

        for(int i=103;i<=105;i++)SendDlgItemMessageW(h,i,BM_SETCHECK,BST_CHECKED,0);
        return 0;
    }
    case WM_COMMAND:
        if(HIWORD(x)==BN_CLICKED)switch(LOWORD(x)){
            case 102:B(h);break;
            case 106:I(h);
        }
        break;
    case WM_APP+1:
        if(x){EnableWindow(GetDlgItem(h,106),1);EnableWindow(GetDlgItem(h,102),1);SetWindowTextW(GetDlgItem(h,107),L"Done!");}
        else SetWindowTextW(GetDlgItem(h,107),(LPCWSTR)y);
        return 0;
    case WM_CTLCOLORSTATIC:{
        HDC dc=(HDC)x;
        SetTextColor(dc,0xe0e0e0);
        SetBkMode(dc,TRANSPARENT);
        return(LRESULT)gBg;
    }
    case WM_CTLCOLOREDIT:{
        HDC dc=(HDC)x;
        SetTextColor(dc,0xf0f0f0);
        SetBkColor(dc,0x2d2d2d);
        static HBRUSH eb=CreateSolidBrush(0x2d2d2d);
        return(LRESULT)eb;
    }
    case WM_ERASEBKGND:
        {RECT r;GetClientRect(h,&r);FillRect((HDC)x,&r,gBg);return 1;}
    case WM_DESTROY:
        PostQuitMessage(0);break;
    }
    return DefWindowProcW(h,q,x,y);
}

int WINAPI WinMain(HINSTANCE i,HINSTANCE,LPSTR,int){
    WNDCLASSW c={};
    c.lpfnWndProc=W;c.hInstance=i;c.lpszClassName=L"m";
    c.hCursor=LoadCursorW(0,(LPCWSTR)(ULONG_PTR)(WORD)32512);
    c.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&c);
    GetModuleFileNameW(0,d,MAX_PATH);
    for(int x=lstrlenW(d);d[x]!=L'\\';)d[x--]=0;
    HWND hw=CreateWindowExW(0,L"m",L"Minecraft Installer",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,460,320,0,0,i,0);
    MSG g;
    while(GetMessageW(&g,0,0,0)>0)TranslateMessage(&g),DispatchMessageW(&g);
    return 0;
}
