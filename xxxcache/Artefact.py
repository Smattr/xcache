import hashlib

def digest(data):
    return hashlib.sha1(data).hexdigest()

class Artefact(object):
    '''An input or output for a task. This class should never be instantiated
    directly.'''
    def digests(self):
        raise NotImplementedError
