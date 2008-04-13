#!/usr/bin/env python
# encoding: utf-8

srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')

    conf.check_pkg('glib-2.0', destvar='GLIB', mandatory=True)
    conf.check_pkg('libusb-1.0', destvar='LIBUSB', mandatory=True)

    conf.sub_config('extra/b25')
#     conf.sub_config('lib/firmware/lib')

def build(bld):
    bld.add_subdirs('extra/b25 lib tsniff')
#     bld.add_subdirs('extra/b25 lib lib/firmware/lib tsniff')
