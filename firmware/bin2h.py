#!/usr/bin/env python

import sys

fp = file(sys.argv[1], 'rb')
print "{"
while True:
    bytes = fp.read(16)
    if not bytes:
        break
    print ''.join(["0x%02x," % ord(i) for i in bytes])
print "};"
