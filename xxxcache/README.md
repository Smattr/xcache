# xxxcache

This is a Python module designed as a standin for the full blown xcache. It is
intended to be used for debugging, development or when you have a good
understanding of your target and don't need the speed or flexibility of xcache.
Usage should be pretty straightforward if you're familiar with the design and
intention of xcache.

Note that this module doesn't do any ptracing or attempt to infer behaviour of
your target. You need to tell it everything. The upside of this is simplicity
of the implementation. The downside is that if you describe your target
incorrectly your cache will malfunction and you'll get strange,
difficult-to-debug aberrations. Consider yourself warned.
