# bignum

A library for arithmetic on large numbers, the size is only limited by memory size.
Before using this code, please note that GMP or MPIR (and other libraries) are better
tested and usually much faster.
If you do use this code, is released under the MIT license.

The goals of this library are largely educational and exploratory.
Educational because it allows me to practice using newer versions of C++,
and coding up some standard arithmetic algorithms.

The exploratory aspect means I can experiment with algorithms tailored for various use-cases.
I want to explore algorithms that are memory-efficient, for example multiplication that
does not have to allocate any temporary memory, so it is an in-place algorithm like quicksort.
I want to explore making some types with the same data format, so that subtypes are actually
subsets, and explore what correctness can be enforced by the compiler in such cases.
I also want to explore some alternative implementations of the Natural and Integer numbers,
and to understand what kinds of computations are sped-up by different implementations.

