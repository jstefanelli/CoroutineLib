#pragma once

#if defined(WIN32) || defined(_WIN32)
#ifdef CRLIB_EXPORTS
#define CRLIB_API __declspec(dllexport)
#else
#define CRLIB_API __declspec(dllimport)
#endif
#else
#define CRLIB_API
#endif