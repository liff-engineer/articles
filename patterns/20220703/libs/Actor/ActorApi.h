
#ifndef ACTOR_EXPORT_H
#define ACTOR_EXPORT_H

#ifdef ACTOR_STATIC_DEFINE
#  define ACTOR_EXPORT
#  define ACTOR_NO_EXPORT
#else
#  ifndef ACTOR_EXPORT
#    ifdef Actor_EXPORTS
        /* We are building this library */
#      define ACTOR_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ACTOR_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ACTOR_NO_EXPORT
#    define ACTOR_NO_EXPORT 
#  endif
#endif

#ifndef ACTOR_DEPRECATED
#  define ACTOR_DEPRECATED __declspec(deprecated)
#endif

#ifndef ACTOR_DEPRECATED_EXPORT
#  define ACTOR_DEPRECATED_EXPORT ACTOR_EXPORT ACTOR_DEPRECATED
#endif

#ifndef ACTOR_DEPRECATED_NO_EXPORT
#  define ACTOR_DEPRECATED_NO_EXPORT ACTOR_NO_EXPORT ACTOR_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ACTOR_NO_DEPRECATED
#    define ACTOR_NO_DEPRECATED
#  endif
#endif

#endif /* ACTOR_EXPORT_H */
