#pragma once

#define cpp_embed_date 202502L // P1967R14
// @todo use 202606L (P3540R3) or greater when C++29 is finalized

#if defined(__cpp_pp_embed) && __cpp_pp_embed >= cpp_embed_date // C++26 #embed
    #ifdef __has_embed
        #define HAS_EMBED 2
    #else
        #define HAS_EMBED 1
    #endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= cpp_embed_date // C23 #embed
    #ifdef __has_embed
        #define HAS_EMBED 2
    #else
        #define HAS_EMBED 1
    #endif
#elif defined(__has_embed)
    #define HAS_EMBED 1
#endif