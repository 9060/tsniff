#!/usr/bin/env python
# encoding: utf-8

import Params

srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.add_option('--disable-cusbfx2', action='store_false', default=True, help='Disable CUSBFX2 support', dest='cusbfx2')
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')

    conf.check_pkg('glib-2.0', destvar='GLIB', mandatory=True)
    if Params.g_options.cusbfx2:
        conf.check_pkg('libusb-1.0', destvar='LIBUSB', mandatory=True)

    conf.sub_config('extra/b25')
#     conf.sub_config('lib/firmware/lib')

    conf.write_config_header('config.h')

def build(bld):
    bld.add_subdirs('extra/b25 lib tsniff')
#     bld.add_subdirs('extra/b25 lib lib/firmware/lib tsniff')
