import Artefact, cPickle, File, os, shutil

class Cache(object):
    '''Cache for mapping process inputs to outputs.'''

    def __init__(self, root):
        '''Create a new cache or load a previously existing one, hosted under
        the directory 'root'.'''
        self.root = root
        self.db = os.path.join(self.root, 'xxxcache.data')
        try:
            with open(self.db, 'r') as f:
                self.data = cPickle.load(f)
        except IOError:
            self.data = {}

    def get(self, *keys):
        '''Lookup a particular output based on a list of inputs. The elements
        of 'keys' should be Artefacts.'''
        d = self.data
        for k in keys:
            assert isinstance(k, Artefact.Artefact)
            for elem in k.digests():
                if elem is None:
                    return None
                d = d.get(elem)
                if d is None:
                    return None
        return os.path.join(self.root, d)

    def sync(self):
        '''Save the current copy of the cache to disk. Externals should never
        need to call this explicitly.'''
        if not os.path.exists(self.root):
            os.makedirs(self.root)
        with open(self.db, 'w') as f:
            cPickle.dump(self.data, f)
            
    def set(self, *args):
        '''Save a particular output to the cache. The elements of 'args' should
        be Artefacts representing inputs, except for the last that should be a
        filename of the output the caller is seeking.'''
        keys, value = args[:-1], args[-1]
        d = self.data
        for k in keys:
            assert isinstance(k, Artefact.Artefact)
            for elem in k.digests():
                if elem not in d:
                    d[elem] = {}
                d = d[elem]
        assert isinstance(value, File.File)
        path_hash, content_hash = list(value.digests())
        d[path_hash] = content_hash
        self.sync()
        cached_path = os.path.join(self.root, content_hash)
        shutil.copy2(value.path, cached_path)
