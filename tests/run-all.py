#!/usr/bin/env python

'''Run all tests from this directory'''

import atexit, copy, os, shutil, subprocess, sys, tempfile

def run(cmd, **kwargs):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        **kwargs)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        print stdout
        print >>sys.stderr, stderr
    return p.returncode

def main():
    me = os.path.abspath(__file__)

    print >>sys.stderr, 'Building all targets...',
    src = os.path.abspath(os.path.join(os.path.dirname(me), '../src'))
    tmp = tempfile.mkdtemp()
    atexit.register(shutil.rmtree, tmp)
    ret = run(['cmake', src, '-G', 'Ninja'], cwd=tmp)
    if ret != 0:
        print >>sys.stderr, 'Failed'
        return ret
    ret = run(['ninja'], cwd=tmp)
    if ret == 0:
        print >>sys.stderr, 'Passed'
    else:
        # If this fails it's critical; bail out
        print >>sys.stderr, 'Failed'
        return ret

    failed = False

    print >>sys.stderr, 'Running unit tests...',
    ret = run([os.path.join(tmp, 'xcache-tests')])
    if ret == 0:
        print >>sys.stderr, 'Passed'
    else:
        print >>sys.stderr, 'Failed'
        failed = True

    env = copy.deepcopy(os.environ)
    env['PATH'] = '%s:%s' % (env.get('PATH', ''), tmp)
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
                failed = True

    if failed:
        return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
