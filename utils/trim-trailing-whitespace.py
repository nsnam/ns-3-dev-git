#!/usr/bin/env python3

# Copyright (c) 2022 Eduardo Almeida.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Eduardo Almeida [@edalm] [INESC TEC and FEUP]

"""
Check and trim trailing whitespace in files indicated in the PATH argument.
This script can be applied to all text files in a given path or to
individual files.
"""

import argparse
import os
import sys

FILE_EXTENSIONS_TO_CHECK = [
    '.c',
    '.cc',
    '.click',
    '.conf',
    '.css',
    '.dot',
    '.gnuplot',
    '.gp',
    '.h',
    '.html',
    '.js',
    '.json',
    '.m',
    '.md',
    '.mob',
    '.ns_params',
    '.ns_movements',
    '.params',
    '.pl',
    '.plt',
    '.py',
    '.rst',
    '.seqdiag',
    '.sh',
    '.txt',
    '.yml',
]

FILES_TO_CHECK = [
    'Makefile',
    'ns3',
]

DIRECTORIES_TO_SKIP = [
    'bindings',
    'build',
    'cmake-cache',
]

def skip_file(filename: str) -> bool:
    """
    Check if a file should be skipped.

    @param filename Name of the file.
    @return Whether the directory should be skipped or not.
    """

    basename, extension = os.path.splitext(os.path.split(filename)[1])

    return basename not in FILES_TO_CHECK and \
           extension not in FILE_EXTENSIONS_TO_CHECK

def skip_directory(dirpath: str) -> bool:
    """
    Check if a directory should be skipped.

    @param dirpath Directory path.
    @return Whether the directory should be skipped or not.
    """

    _, directory = os.path.split(dirpath)

    return (directory.startswith('.') and directory != '.') or \
            directory in DIRECTORIES_TO_SKIP

def trim_trailing_whitespace(path: str, trim: bool) -> None:
    """
    Trim trailing whitespace in all text files in the given path.

    @param path Path to the files.
    @param trim Whether to trim the file (True) or
                just check if the file has trailing whitespace (False).
    """

    files_with_trailing_whitespace = []

    abs_path = os.path.normpath(os.path.abspath(os.path.expanduser(path)))

    if os.path.isfile(abs_path):

        if not skip_file(abs_path):
            file_had_whitespace = trim_file(abs_path, trim)

            if file_had_whitespace:
                files_with_trailing_whitespace.append(path)

    elif os.path.isdir(abs_path):

        for dirpath, dirnames, filenames in os.walk(path, topdown=True):

            # Check if directory and its subdirectories should be skipped
            if skip_directory(dirpath):
                dirnames[:] = []
                continue

            # Process files with trailing whitespace
            filenames = [os.path.join(dirpath, f) for f in filenames]

            for filename in filenames:

                # Skip files that should not be checked
                if skip_file(filename):
                    continue

                file_had_whitespace = trim_file(
                    os.path.normpath(os.path.abspath(os.path.expanduser(filename))),
                    trim,
                )

                if file_had_whitespace:
                    files_with_trailing_whitespace.append(filename)

    else:
        print(f'Error: {path} is not a file nor a directory')
        sys.exit(1)

    # Output results
    n_files_with_trailing_whitespace = len(files_with_trailing_whitespace)

    if files_with_trailing_whitespace:
        if trim:
            print('Trimmed files with trailing whitespace:\n')
        else:
            print('Detected files with trailing whitespace:\n')

        for f in files_with_trailing_whitespace:
            print(f'- {f}')

        if trim:
            print(f'\n'
                  f'Number of files trimmed: {n_files_with_trailing_whitespace}')
        else:
            print(f'\n'
                  f'Number of files with trailing whitespace: {n_files_with_trailing_whitespace}')
            sys.exit(1)

    else:
        print('No files detected with trailing whitespace')

def trim_file(filename: str, trim: bool) -> bool:
    """
    Trim trailing whitespace in a given file.

    @param filename Name of the file to be trimmed.
    @param trim Whether to trim the file (True) or
                just check if the file has trailing whitespace (False).
    @return Whether the file had trailing whitespace (True) or not (False).
    """

    has_trailing_whitespace = False

    try:
        with open(filename, 'r') as f:
            file_lines = f.readlines()

        # Check if there are trailing whitespace and trim them
        for (i, line) in enumerate(file_lines):
            line_trimmed = line.rstrip() + '\n'

            if line_trimmed != line:
                has_trailing_whitespace = True

                # Optimization: if only checking, skip the rest of the file,
                # since it does have trailing whitespace
                if not trim:
                    break

                file_lines[i] = line_trimmed

        # Update the file with the trimmed lines
        if trim and has_trailing_whitespace:
            with open(filename, 'w') as f:
                f.writelines(file_lines)

    except Exception as e:
        print(e)
        sys.exit(1)

    return has_trailing_whitespace

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=
        'Trim trailing whitespace in all text files in a given PATH.')

    parser.add_argument('path', action='store', type=str, help=
        'Path to the files.')

    parser.add_argument('--check', action='store_true', help=
        'Check if the files have trailing whitespace, without modifying them. '
        'If they have, this process will exit with a non-zero exit code '
        'and list them in the output.')

    args = parser.parse_args()

    trim_trailing_whitespace(args.path, trim=(not args.check))
