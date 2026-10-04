#include "WindowsAccess.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

bool hasAdministratorAccess()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    const bool success = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes) != 0;
    CloseHandle(token);
    return success && elevation.TokenIsElevated != 0;
}
