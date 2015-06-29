#ifndef SLOG_LIKELY_H_
#define SLOG_LIKELY_H_

#if defined(__GNUC__)
#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#endif

