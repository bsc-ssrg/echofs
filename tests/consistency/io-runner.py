#!/usr/bin/python3
# -----------------------------------------------------------------------
# (C) Copyright 2016 Barcelona Supercomputing Center
#                    Centro Nacional de Supercomputacion
# 
# This file is part of the Echo Filesystem NG.
# 
# See AUTHORS file in the top level directory for information
# regarding developers and contributors.
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# 
# The Echo Filesystem NG is distributed in the hope that it will 
# be useful, but WITHOUT ANY WARRANTY; without even the implied 
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU Lesser General Public License for more
# details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with Echo Filesystem NG; if not, write to the Free 
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# -----------------------------------------------------------------------
#
# Helper script to ease executing system calls on top of a filesystem
#

import os,sys,errno
import argparse
import binascii
import hexdump

COMMANDS = dict()

class Command:
    def __init__(self):
        self.name = "";

    def help(self):
        pass

    def run(self, fd, args):
        pass

class pread_cmd(Command):
    def __init__(self):
        self.name = "pread"

    def help(self):
        print("\n"
              " reads a range of bytes in a specified block size from the given offset\n"
              "\n"
              " Example:\n"
              " 'pread -v 512 20' - dumps 20 bytes read from 512 bytes into the file\n"
              "\n"
              " Reads a segment of the currently open file, optionally dumping it to the\n"
              " standard output stream (with -v option) for subsequent inspection.\n"
              " The reads are performed in sequential blocks starting at offset, with the\n"
              " blocksize tunable using the -b option (default blocksize is 4096 bytes),\n"
              " unless a different pattern is requested.\n"
              " -B      -- read backwards through the range from offset (backwards N bytes)\n"
              " -F      -- read forwards through the range of bytes from offset (default)\n"
              " -v      -- be verbose, dump out buffers (used when reading forwards)\n"
              " -R      -- read at random offsets in the range of bytes\n"
              " -Z N    -- zeed the random number generator (used when reading randomly)\n"
              "            (heh, zorry, the -s/-S arguments were already in use in pwrite)\n"
              "\n"
              " When in \"random\" mode, the number of read operations will equal the\n"
              " number required to do a complete forward/backward scan of the range.\n"
              " Note that the offset within the range is chosen at random each time\n"
              " (an offset may be read more than once when operating in this mode).\n"
              "\n");

    def run(self, fd, args):

        parser = argparse.ArgumentParser()

        # arguments
        parser.add_argument('offset', type=int);
        parser.add_argument('buffersize', type=int);

        # flags
        parser.add_argument('-v', action='store_true');
        parser.add_argument('-B', action='store_true');
        parser.add_argument('-F', action='store_true');
        parser.add_argument('-R', action='store_true');
        parser.add_argument('-Z', action='store_true');

        pargs = parser.parse_args(args)

        offset = pargs.offset
        buffersize = pargs.buffersize

        try:
            data = os.pread(fd, buffersize, offset);
            hexdump.hexdump(data)
        except OSError as e:
            print("pread error: [" + errno.errorcode[e.errno] + " \"" + e.strerror + "\"]")
            sys.exit(1)


def init_commands():

    COMMANDS['pread'] = pread_cmd()

def init(argv):

    init_commands()

    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--command', metavar='CMD',
                        help='run command CMD', action='append')
    parser.add_argument('file', metavar='INFILE', type=str, nargs=1)
    parser.add_argument('-o', '--outfile', metavar='OUTFILE', #default=,
                        help="write command output to OUTFILE instead of STDOUT")
    args = parser.parse_args()

    commands = args.command
    target_file = args.file

    return (target_file[0], args.outfile, commands)

def command_loop(target_file, outfile, commands):

    # open 'target_file'
    fd = os.open(target_file, os.O_RDWR)

    if outfile is not None:
        saved_stdout = sys.stdout
        sys.stdout = open(outfile, 'w')

    for cmd in commands:
        cmd_fields = cmd.split(' ')

        cmd_name = cmd_fields[0]
        cmd_args = cmd_fields[1:]

        COMMANDS[cmd_name].run(fd, cmd_args)

    if outfile is not None:
        sys.stdout.close()
        sys.stdout = saved_stdout

    os.close(fd)

def do_pread(fd, offset, buffersize):
    try:
        data = os.pread(fd, buffersize, offset);
        hexdump.hexdump(data)
    except OSError as e:
        print("pread error: [" + errno.errorcode[e.errno] + " \"" + e.strerror + "\"]")
        sys.exit(1)

if __name__ == "__main__":

    target_file, outfile, commands = init(sys.argv)

    command_loop(target_file, outfile, commands)

    sys.exit(0)
