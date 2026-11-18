// TimeShift fix: proxy dinput8.dll
//  1) GetSystemInfo reports <= 4 CPUs (startup crash on >8-thread CPUs)
//  2) IDirect3D9::GetAdapterIdentifier reports an old-style driver version
//     (game expects NVIDIA x.15.11.>=6700 on Vista+)
//  3) In-memory patch of the Steam (SteamStub-decrypted) TimeShift.Exe:
//     the "Incompatible Video Driver" branch at 0x773417 (jne -> jmp).
//     Applied only if the bytes match exactly; otherwise nothing is touched.
//  The .exe on disk is never modified (SteamStub rejects modified exes).
//  Build: 32-bit (x86) DLL, see build.bat. License: MIT.
#include <windows.h>
#include <d3d9.h>
#define MAXCPU 4
// Spoofed driver version 8.15.11.8593 (Vista/7 ForceWare numbering; game wants x.15.11.>=6700)
#define DRV_HI MAKELONG(15, 8)
#define DRV_LO MAKELONG(8593, 11)

typedef struct { BYTE *target; BYTE orig[5], patch[5]; } Hook;
static CRITICAL_SECTION cs;
static Hook hGSI, hD3D;
static HMODULE real;
static FARPROC pCreate,pCanUnload,pGetClass,pReg,pUnreg;
typedef HRESULT (WINAPI *GAI_t)(IDirect3D9*,UINT,DWORD,D3DADAPTER_IDENTIFIER9*);
static GAI_t origGAI;

static void put(Hook *h, BYTE *src){DWORD o;VirtualProtect(h->target,5,PAGE_EXECUTE_READWRITE,&o);memcpy(h->target,src,5);VirtualProtect(h->target,5,o,&o);FlushInstructionCache(GetCurrentProcess(),h->target,5);}
static void hook(Hook *h, BYTE *target, void *fn){
    if(!target)return; h->target=target; memcpy(h->orig,target,5);
    h->patch[0]=0xE9; *(DWORD*)(h->patch+1)=(DWORD)((BYTE*)fn-(target+5)); put(h,h->patch);
}

static void WINAPI HookGSI(LPSYSTEM_INFO si){
    EnterCriticalSection(&cs);
    put(&hGSI,hGSI.orig); ((void(WINAPI*)(LPSYSTEM_INFO))hGSI.target)(si); put(&hGSI,hGSI.patch);
    LeaveCriticalSection(&cs);
    if(si->dwNumberOfProcessors>MAXCPU){si->dwNumberOfProcessors=MAXCPU;si->dwActiveProcessorMask&=(1u<<MAXCPU)-1;}
}

static HRESULT WINAPI HookGAI(IDirect3D9 *self,UINT a,DWORD f,D3DADAPTER_IDENTIFIER9 *id){
    HRESULT r=origGAI(self,a,f,id);
    if(SUCCEEDED(r)&&id){id->DriverVersion.HighPart=DRV_HI;id->DriverVersion.LowPart=DRV_LO;}
    return r;
}

static IDirect3D9* WINAPI HookCreate9(UINT sdk){
    IDirect3D9 *d;
    EnterCriticalSection(&cs);
    put(&hD3D,hD3D.orig); d=((IDirect3D9*(WINAPI*)(UINT))hD3D.target)(sdk); put(&hD3D,hD3D.patch);
    {   // Skip "Incompatible Video Driver" dialog: test esi,esi / jne +0x49  ->  jmp +0x49
        // Only patched if the (Steam-decrypted, in-memory) bytes match exactly.
        BYTE *p=(BYTE*)0x773415; static const BYTE want[4]={0x85,0xF6,0x75,0x49};
        MEMORY_BASIC_INFORMATION mi;
        if(VirtualQuery(p,&mi,sizeof mi)&&mi.State==MEM_COMMIT&&!memcmp(p,want,4)){
            DWORD o;VirtualProtect(p+2,1,PAGE_EXECUTE_READWRITE,&o);p[2]=0xEB;VirtualProtect(p+2,1,o,&o);FlushInstructionCache(GetCurrentProcess(),p+2,1);
        }
    }
    if(d){
        void **vt=*(void***)d; // GetAdapterIdentifier is vtable slot 5
        if(vt[5]!=(void*)HookGAI){DWORD o;origGAI=(GAI_t)vt[5];VirtualProtect(&vt[5],4,PAGE_READWRITE,&o);vt[5]=(void*)HookGAI;VirtualProtect(&vt[5],4,o,&o);}
    }
    LeaveCriticalSection(&cs);
    return d;
}

static void install(void){
    HMODULE kb=GetModuleHandleA("kernelbase.dll"), d9;
    InitializeCriticalSection(&cs);
    hook(&hGSI,(BYTE*)GetProcAddress(kb?kb:GetModuleHandleA("kernel32.dll"),"GetSystemInfo"),HookGSI);
    d9=GetModuleHandleA("d3d9.dll"); if(!d9)d9=LoadLibraryA("d3d9.dll");
    if(d9)hook(&hD3D,(BYTE*)GetProcAddress(d9,"Direct3DCreate9"),HookCreate9);
}
static void loadreal(void){
    char p[MAX_PATH]; if(real)return;
    GetSystemDirectoryA(p,MAX_PATH); lstrcatA(p,"\\dinput8.dll"); real=LoadLibraryA(p);
    pCreate=GetProcAddress(real,"DirectInput8Create"); pCanUnload=GetProcAddress(real,"DllCanUnloadNow");
    pGetClass=GetProcAddress(real,"DllGetClassObject"); pReg=GetProcAddress(real,"DllRegisterServer"); pUnreg=GetProcAddress(real,"DllUnregisterServer");
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD r,LPVOID x){ if(r==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(h);install();} return TRUE;}
HRESULT WINAPI P_DirectInput8Create(HINSTANCE a,DWORD b,REFIID c,LPVOID*d,LPUNKNOWN e){loadreal();return ((HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN))pCreate)(a,b,c,d,e);}
HRESULT WINAPI P_DllCanUnloadNow(void){loadreal();return ((HRESULT(WINAPI*)(void))pCanUnload)();}
HRESULT WINAPI P_DllGetClassObject(REFCLSID a,REFIID b,LPVOID*c){loadreal();return ((HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*))pGetClass)(a,b,c);}
HRESULT WINAPI P_DllRegisterServer(void){loadreal();return ((HRESULT(WINAPI*)(void))pReg)();}
HRESULT WINAPI P_DllUnregisterServer(void){loadreal();return ((HRESULT(WINAPI*)(void))pUnreg)();}
