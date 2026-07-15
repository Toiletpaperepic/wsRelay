#ifndef WEBSOCKET_DLL_EXPORT_H
#define WEBSOCKET_DLL_EXPORT_H

#if defined(DLL_EXPORT)
// already included
#elif defined(_WIN32)
// MSVC Windows: DLL export
#define DLL_EXPORT __declspec(dllexport)
#else
// leave it empty if system is on not on windows.
#define DLL_EXPORT 
#endif
#endif
