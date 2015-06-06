from .digest import digest

def make_key(*args):
    assert all(isinstance(a, basestring) for a in args)
    return '|'.join(digest(x) for x in args)

def extend_key(key, *args):
    assert isinstance(key, basestring)
    assert all(isinstance(a, basestring) for a in args)
    if len(args) == 0:
        return key
    return '%s|%s' % (key, make_key(*args))
