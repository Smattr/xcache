import os

def resolve(target, mustnotbe=None):
    '''Find an executable (as you would with `which`), excepting anything in
    'mustnotbe'. The purpose of this is to let callers look themselves up to
    find a later executable in your $PATH. FYI this is also used in ccache. The
    point of this functionality is to let you just symlink to a cacher ahead of
    a real executable.'''
    if mustnotbe is None:
        mustnotbe = set()
    syspath = os.environ.get('PATH')
    roots = syspath.split(':') if syspath is not None else []
    for r in roots:
        candidate = os.path.realpath(os.path.join(r, target))
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK) and \
                candidate not in mustnotbe:
            return candidate
    return None
