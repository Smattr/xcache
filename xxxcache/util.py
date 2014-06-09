import os

def resolve(target, mustnotbe=None):
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
