import Artefact, os

class File(Artefact.Artefact):
    def __init__(self, path):
        self.path = os.path.abspath(path)

    def digests(self):
        yield Artefact.digest(self.path)
        try:
            with open(self.path, 'r') as f:
                yield Artefact.digest(f.read())
        except IOError:
            yield None
