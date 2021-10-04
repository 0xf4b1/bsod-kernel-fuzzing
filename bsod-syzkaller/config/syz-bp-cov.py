#!/usr/bin/env python3

import sys
import subprocess
import time


# Get ssh port for qemu instance with matching kvmi socket
ps = subprocess.check_output(['ps', 'a'])
for line in ps.decode("utf-8").splitlines():
    if sys.argv[1] in line:
        port = line[line.index('::')+2:line.index('-:')]
        print(port)
        break

# Get kernel module addresses from guest
output = subprocess.check_output(['ssh', '-p', port, '-o', 'StrictHostKeyChecking=no', 'root@localhost', 'cat', '/proc/modules'])

for line in output.decode("utf-8").splitlines():
    split = line.split(' ')
    name = split[0]
    addr = split[5]

    for i in range(3, len(sys.argv), 2):
        if name == sys.argv[i]:
            sys.argv[i] = addr

# Connect introspection tool
subprocess.Popen(['../syz-bp-cov/syz-bp-cov'] + sys.argv[1:])

# Wait short amount of time to delay fuzzer start
time.sleep(30)
