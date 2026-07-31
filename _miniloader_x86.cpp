// Mini-loader: laduje foo_bookbar.dll (x86), wola foobar2000_get_interface(NULL,hIns)
// i wypisuje get_version(). MUSI == 81, inaczej 32-bit foobar v2 odrzuci komponent po restarcie.
#include <windows.h>
#include <cstdio>
#include <cstdint>

// Minimalny odpowiednik foobar2000_client (potrzebny tylko offset get_version()).
// Layout vtable zgodny z SDK: get_version() to pierwsza metoda wirtualna.
struct fb2k_client_min {
    virtual uint32_t get_version() = 0;
    // reszta metod nas nie interesuje
};

typedef fb2k_client_min* (__cdecl *pget)(void*, HINSTANCE);

int main() {
    SetDllDirectoryA("D:\\projekty\\bookbar\\sdk\\foobar2000\\shared\\Release");
    HMODULE h = LoadLibraryA("D:\\projekty\\bookbar\\build_x86\\foo_bookbar.dll");
    if (!h) { printf("LOAD_FAIL err=%lu\n", GetLastError()); return 1; }
    pget f = (pget)GetProcAddress(h, "foobar2000_get_interface");
    if (!f) { printf("NO_EXPORT\n"); return 2; }
    fb2k_client_min* c = f(nullptr, (HINSTANCE)h);
    if (!c) { printf("NULL_CLIENT\n"); return 3; }
    printf("CLIENT_VERSION=%u\n", c->get_version());
    return 0;
}
