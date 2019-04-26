# drv_shed

This task manager isn't meant to try and be as fast as can be, but it is meant
to be easy to reason about. Upfront allocations/setup at startup.

This will not be suitable for some applications that wish to handle huge scale
ranges.


## Design Goals

- Simplicity: Fixed array size.
- Simplicity: Advanced topics such as task stealing are ignored.
- Simplicity: No scaling up/down of resources such as fiber/task queues.
- Must have mechanisms for profiling.


## Code Goals

- OS thread abstractions
- C89


## Issues

Since this impl uses a single queue that is shared between threads that causes
some minor perf issues. The cache line gets pinged around, and there is some
contention issues. I did have a version that dealt with both those issues,
but it increased the complexity of the code, I felt simpler was better.

The original version could scale fiber count as required. This combined with 
the above also added to the complexity, so was abandoned. The result is you 
must decided how many fibers (thus memory) you require up front.