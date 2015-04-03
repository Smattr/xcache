#!/usr/bin/env python

'''
This test case checks that we can trace a process that forks.

We create and compile a program that opens two files, one for reading and one
for writing. The read file is opened by the original process while the written
process is opened in a forked child. If everything's correct we should note
both the input and the output.
'''

import os, sqlite3, subprocess, sys

# Source code of the test program.
fork_test_source = '''
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void parent(void) {
    int f = open("hello.txt", O_RDONLY);
    char buf[10];
    (void)read(f, buf, sizeof(buf));
    close(f);
}

void child(void) {
    int f = open("world.txt", O_WRONLY|O_CREAT);
    char buf[] = "hello world";
    (void)write(f, buf, sizeof(buf));
    close(f);
}

int main(void) {
    pid_t pid = fork();
    switch (pid) {
        case 0: /* child */
            child();
            break;
        case -1:
            fprintf(stderr, "fork failed\\n");
            return -1;
        default: /* parent */
            parent();
            wait(NULL);
    }
    return 0;
}
'''

def write_test_source():
    with open('fork-test.c', 'w') as f:
        print >>f, fork_test_source

def compile_source():
    subprocess.check_call(['gcc', 'fork-test.c'])

def trace_test_case():
    subprocess.check_call(['xcache', '--cache-dir', os.getcwd(), './a.out'])

write_test_source()

compile_source()

trace_test_case()

# Check that we noted the input.
db = sqlite3.connect('cache.db')
inputs = db.execute('select * from input where filename = \'%s\'' % \
    os.path.join(os.getcwd(), 'hello.txt'))
if inputs.fetchone() is None:
    raise Exception('failed to note input read by parent')

# Check that we noted the output.
outputs = db.execute('select * from output where filename = \'%s\'' % \
    os.path.join(os.getcwd(), 'world.txt'))
if outputs.fetchone() is None:
    raise Exception('failed to note output written by child')
