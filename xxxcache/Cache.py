import Artefact, cPickle, File, os

class Cache(object):
    def __init__(self, root):
        self.root = root
        self.db = os.path.join(self.root, 'xxxcache.data')
        try:
            with open(self.db, 'r') as f:
                self.data = cPickle.load(f)
        except IOError:
            self.data = {}

    def get(self, *keys):
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
        os.makedirs(self.root)
        with open(self.db, 'w') as f:
            cPickle.dump(self.data, f)
            
    def set(self, *args):
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
