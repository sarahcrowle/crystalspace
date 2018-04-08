class NoNewLineWriter():

    '''
    Helper class to wrap old style write func while migrating to XML class
    '''

    def __init__(self, f):
        self.f = f
        self.content = ''

    def __call__(self, data):
        self.content += data
        if self.content.endswith('\n'):
            self.f(self.content[:-1])
            self.content = ''


class XML(object):

    '''
    XML builder with direct to output writing, no DOM is build.
    Example:
    xml = XML(NoNewLineWriter(func))

    xml.meshfact({'name': self.data.uname})\
        .plugin('crystalspace.mesh.loader.factory.terrain2').close()\
        .param('/lev/terrain/heightmap_257x257.png', {'name': 'heightmap source'}, close=True)\
        .shadows(close=True)\
        .params()\
            .renderer('crystalspace.mesh.object.terrain2.bruteblockrenderer').close()\
        .close()\
    .close()
    '''

    def __init__(self, func, depth=0):
        self.names = []
        self.func = func
        self.depth = depth

    def __del__(self):
        while len(self.names):
            self.close()

    def __getattr__(self, name):
        if len(self.names):
            self.names[-1][1] = True

        self.names.append([name, False])

        return self._element

    def close(self):
        name, newline = self.names[-1]

        if newline:
            self._intend()
        self._write('</%s>\n' % (name))

        self.names.pop()

        return self

    def _element(self, *args, **kwargs):
        args = [a for a in args]
        if len(args) == 1:
            args.append(None)
        elif len(args) == 0:
            args.append(None)
            args.append(None)

        if type(args[0]) == type({}):
            attributes, content = args
        else:
            content, attributes = args

        self._intend()._write('<%s' % (self.names[-1][0]))
        if attributes:
            for key, value in attributes.items():
                self._write(' %s="%s"' % (key, value))

        doClose = 'close' in kwargs and kwargs['close']

        if not content and doClose:
            self._write(' />\n')
            self.names.pop()
        elif content and doClose:
            self._write('>')
            self._write(content)
            self._write('</%s>\n' % (self.names[-1][0]))
            self.names.pop()
        elif content:
            self._write('>')
            self._write(content)
        else:
            self._write('>\n')

        return self

    def _intend(self):
        depth = ((len(self.names) - 1) * 2) + self.depth
        self._write(' ' * depth)
        return self

    def _write(self, data):
        self.func(data)
        return self


if __name__ == '__main__':
    import sys

    def Write():
        def write(data):
            sys.stdout.write(data)
        return write

    xml = XML(Write())

    xml.meshfact({'name': 'fff'})\
        .plugin('crystalspace.mesh.loader.factory.terrain2').close()\
        .shadows(close=True)\
        .params()\
        .renderer('crystalspace.mesh.object.terrain2.bruteblockrenderer').close()\
        .collider('crystalspace.mesh.object.terrain2.collider').close()\
        .feeder('crystalspace.mesh.object.terrain2.simpledatafeeder', close=True)\
        .cells()\
        .celldefault()\
        .gridsize({'width': 257, 'height': 257}, close=True)\
        .size({'x': 256, 'y': 32, 'z': 256}, close=True)\
        .close()\
                \
        .cell()\
        .name(0, close=True)\
        .position({'x': -127, 'y': -127}, close=True)\
        .basematerial('Base', close=True)\
        .feederproperties()\
        .param('/lev/terrain/heightmap_257x257.png', {'name': 'heightmap source'}, close=True)\
        .alphamap('alpha_marble.png', {'material': 'Marble'}).close()\
        .alphamap('alpha_stone.png', {'material': 'Stone'}).close()\

    del xml
