#ifndef KV_API_H
#define KV_API_H

#if defined(_WIN32)
#if defined(KV_DYNAMIC_LINK)
#define KV_API __declspec(dllexport)
#else
#define KV_API
#endif
#else
#define KV_API
#endif

#endif
