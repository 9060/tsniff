#!/usr/bin/env python
# encoding: utf-8

import Params

srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.add_option('--without-libusb', action='store_false', default=True, help='Without libusb', dest='libusb')
    opt.tool_options('compiler_cc')
    opt.sub_options('extra/b25')

def configure(conf):
    conf.check_tool('compiler_cc')

    conf.check_pkg('glib-2.0', destvar='GLIB', mandatory=True)
    conf.check_pkg('gthread-2.0', destvar='GTHREAD', mandatory=True)

    # libusb
    if Params.g_options.libusb:
        if not conf.check_pkg('libusb-1.0', destvar='LIBUSB', mandatory=False):
            conf.env['HAVE_LIBUSB'] = False
    else:
        conf.env['HAVE_LIBUSB'] = False

    conf.sub_config('extra/b25')
#     conf.sub_config('lib/firmware/lib')

    conf.write_config_header('config.h')

def build(bld):
    bld.add_subdirs('extra/b25 lib tsniff')
#     bld.add_subdirs('extra/b25 lib lib/firmware/lib tsniff')
