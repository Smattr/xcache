#!/usr/bin/env python

'''Run all tests from this directory'''

import copy, os, subprocess, sys

def main(argv):
    me = os.path.abspath(__file__)

    print >>sys.stderr, 'Building all targets...'
    src = os.path.abspath(os.path.join(os.path.dirname(me), '../src'))
    ret = subprocess.call(['make'], cwd=src)
    if ret != 0:
        print >>sys.stderr, 'Failed'
        return ret

    env = copy.deepcopy(os.environ)
    env['PATH'] = '%s:%s' % (env.get('PATH', ''), src)
    env['LD_LIBRARY_PATH'] = src

    for t in os.listdir(os.path.dirname(me)):
        test = os.path.abspath(os.path.join(os.path.dirname(me), t))
        if os.path.isfile(test) and os.access(test, os.X_OK) and test != me:
            print >>sys.stderr, 'Running %s...' % test
            ret = subprocess.call([test], env=env)
            if ret != 0:
                print >>sys.stderr, 'Failed'
                return ret

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
