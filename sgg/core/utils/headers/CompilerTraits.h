#ifndef COMPILERTRAITS_H
#define COMPILERTRAITS_H

#ifdef _MSC_VER
    #define ATTR_GNU_HOT
    #define ATTR_GNU_FLATTEN
    #define ATTR_GNU_ALWAYS_INLINE
#else
    #define ATTR_GNU_HOT           [[gnu::hot]]
    #define ATTR_GNU_FLATTEN       [[gnu::flatten]]
    #define ATTR_GNU_ALWAYS_INLINE [[gnu::always_inline]]
#endif

#endif //COMPILERTRAITS_H
