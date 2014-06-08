import Artefact

class String(Artefact.Artefact):
    def __init__(self, s):
        self.s = s

    def digests(self):
        yield Artefact.digest(self.s)
