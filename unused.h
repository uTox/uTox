
//Some libraries define it (yes, I'm looking at you, libvpx).
//Ensure that our definition prevails.
#ifdef UNUSED
# undef UNUSED
#endif

#ifdef __GNUC__
# define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif
