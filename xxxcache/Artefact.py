import hashlib

def digest(data):
    return hashlib.sha1(data).hexdigest()

class Artefact(object):
    def digests(self):
        raise NotImplementedError
