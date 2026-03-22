#if defined(_WIN32) && !defined(DLL_EXPORT)
// MSVC Windows: DLL export
#define DLL_EXPORT __declspec(dllexport)
#else
// leave it empty if system is on not on windows or already included.
#define DLL_EXPORT 
#endif