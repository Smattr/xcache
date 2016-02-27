#!/usr/bin/env python

'''Run all tests from this directory'''

import atexit, copy, os, shutil, subprocess, sys, tempfile

def run(cmd, **kwargs):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        **kwargs)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        sys.stdout.write(stdout)
        sys.stderr.write(stderr)
    return p.returncode

def main():
    me = os.path.abspath(__file__)
    my_dir = os.path.dirname(me)

    sys.stderr.write('Building all targets... ')
    sys.stderr.flush()
    src = os.path.abspath(os.path.join(my_dir, '../src'))
    tmp = tempfile.mkdtemp()
    atexit.register(shutil.rmtree, tmp)
    ret = run(['cmake', src, '-G', 'Ninja'], cwd=tmp)
    if ret != 0:
        sys.stderr.write('Failed\n')
        return ret
    ret = run(['ninja'], cwd=tmp)
    if ret == 0:
        sys.stderr.write('Passed\n')
    else:
        # If this fails it's critical; bail out
        sys.stderr.write('Failed\n')
        return ret

    failed = False

    sys.stderr.write('Running unit tests... ')
    ret = run([os.path.join(tmp, 'xcache-tests')])
    if ret == 0:
        sys.stderr.write('Passed\n')
    else:
        sys.stderr.write('Failed\n')
        failed = True

    env = copy.deepcopy(os.environ)
    env['PATH'] = '%s:%s' % (env.get('PATH', ''), tmp)
    env['LD_LIBRARY_PATH'] = src

    for t in os.listdir(my_dir):
        test = os.path.abspath(os.path.join(my_dir, t))
        if os.path.isfile(test) and os.access(test, os.X_OK) and test != me:
            sys.stderr.write('Running %s... ' % test)
            sys.stderr.flush()
            cwd = tempfile.mkdtemp()
            atexit.register(shutil.rmtree, cwd)
            ret = run([test], env=env, cwd=cwd)
            if ret == 0:
                sys.stderr.write('Passed\n')
            else:
                sys.stderr.write('Failed\n')
                failed = True

    if failed:
        return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
