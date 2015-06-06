import collections, os, shelve

class Cache(collections.MutableMapping):
    '''Cache for mapping process inputs to outputs.'''

    def __init__(self, root):
        '''Create a new cache or load a previously existing one, hosted under
        the directory 'root'.'''
        if not os.path.exists(root):
            os.makedirs(root)
        self.db = shelve.open(os.path.join(root, 'xxxcache.data'))

    def __getitem__(self, key):
        assert isinstance(key, basestring)
        return self.db[key]

    def __setitem__(self, key, value):
        assert isinstance(key, basestring)
        self.db[key] = value

    def __delitem__(self, key):
        assert isinstance(key, basestring)
        del self.db[key]

    def __iter__(self):
        return iter(self.db)

    def __len__(self):
        return len(self.db)
