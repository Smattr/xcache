#!/usr/bin/env python

'''Run all tests from this directory'''

import copy, os, subprocess, sys

def run(cmd, **kwargs):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        **kwargs)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        print stdout
        print >>sys.stderr, stderr
    return p.returncode

def main(argv):
    me = os.path.abspath(__file__)

    print >>sys.stderr, 'Building all targets...',
    src = os.path.abspath(os.path.join(os.path.dirname(me), '../src'))
    ret = run(['make'], cwd=src)
    if ret == 0:
        print >>sys.stderr, 'Passed'
    else:
        print >>sys.stderr, 'Failed'
        return ret

    env = copy.deepcopy(os.environ)
    env['PATH'] = '%s:%s' % (env.get('PATH', ''), src)
    env['LD_LIBRARY_PATH'] = src

    for t in os.listdir(os.path.dirname(me)):
        test = os.path.abspath(os.path.join(os.path.dirname(me), t))
        if os.path.isfile(test) and os.access(test, os.X_OK) and test != me:
            print >>sys.stderr, 'Running %s...' % test,
            ret = run([test], env=env)
            if ret == 0:
                print >>sys.stderr, 'Passed'
            else:
                print >>sys.stderr, 'Failed'
                return ret

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
