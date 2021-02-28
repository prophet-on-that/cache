## Memory Management Convention

The following conventions are used to ease memory management:

  * When calling a function prefixed with `create` or `out` that
    returns a pointer, the caller becomes its owner and is responsible
    for deallocation.
  * When calling a function, and pointer arguments prefixed by `take`
    take ownership of the pointer and will handle deallocation. On
    return from the function, the pointer argument must no longer be
    used. Moreover, a pointer to the stack should never be passed to
    such an argument.
