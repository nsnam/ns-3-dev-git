#!/usr/bin/env python3

# Copyright (c) 2022 Eduardo Nuno Almeida.
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
# Author: Eduardo Nuno Almeida <enmsa@outlook.pt> [INESC TEC and FEUP, Portugal]

"""
Check and apply the ns-3 coding style to all files in the PATH argument.

The coding style is defined with the clang-format tool, whose definitions are in
the ".clang-format" file. This script performs the following checks / fixes:
- Check / apply clang-format.
- Check / trim trailing whitespace.
- Check / replace tabs with spaces.

The clang-format and tabs checks respect clang-format guards, which mark code blocks
that should not be checked. Trailing whitespace is always checked regardless of
clang-format guards.

This script can be applied to all text files in a given path or to individual files.

NOTE: The formatting check requires clang-format (version >= 14) to be found on the path.
Trimming of trailing whitespace and conversion of tabs to spaces (via the "--no-formatting"
option) do not depend on clang-format.
"""

import argparse
import concurrent.futures
import itertools
import os
import shutil
import subprocess
import sys
from typing import List, Tuple


###########################################################
# PARAMETERS
###########################################################
CLANG_FORMAT_VERSIONS = [
    16,
    15,
    14,
]

CLANG_FORMAT_GUARD_ON = '// clang-format on'
CLANG_FORMAT_GUARD_OFF = '// clang-format off'

DIRECTORIES_TO_SKIP = [
    '__pycache__',
    '.vscode',
    'bindings',
    'build',
    'cmake-cache',
    'testpy-output',
]

# List of files entirely copied from elsewhere that should not be checked,
# in order to optimize the performance of this script
FILES_TO_SKIP = [
    'valgrind.h',
]

FILE_EXTENSIONS_TO_CHECK_FORMATTING = [
    '.c',
    '.cc',
    '.h',
]

FILE_EXTENSIONS_TO_CHECK_WHITESPACE = [
    '.c',
    '.cc',
    '.click',
    '.cmake',
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

FILES_TO_CHECK_WHITESPACE = [
    'Makefile',
    'ns3',
]

FILE_EXTENSIONS_TO_CHECK_TABS = [
    '.c',
    '.cc',
    '.h',
    '.md',
    '.py',
    '.rst',
    '.sh',
    '.yml',
]
TAB_SIZE = 4


###########################################################
# AUXILIARY FUNCTIONS
###########################################################
def skip_directory(dirpath: str) -> bool:
    """
    Check if a directory should be skipped.

    @param dirpath Directory path.
    @return Whether the directory should be skipped or not.
    """

    _, directory = os.path.split(dirpath)

    return (directory in DIRECTORIES_TO_SKIP or
            (directory.startswith('.') and directory != '.'))


def skip_file_formatting(path: str) -> bool:
    """
    Check if a file should be skipped from formatting analysis.

    @param path Path to the file.
    @return Whether the file should be skipped or not.
    """

    filename = os.path.split(path)[1]

    if filename in FILES_TO_SKIP:
        return True

    _, extension = os.path.splitext(filename)

    return extension not in FILE_EXTENSIONS_TO_CHECK_FORMATTING


def skip_file_whitespace(path: str) -> bool:
    """
    Check if a file should be skipped from trailing whitespace analysis.

    @param path Path to the file.
    @return Whether the file should be skipped or not.
    """

    filename = os.path.split(path)[1]

    if filename in FILES_TO_SKIP:
        return True

    basename, extension = os.path.splitext(filename)

    return (basename not in FILES_TO_CHECK_WHITESPACE and
            extension not in FILE_EXTENSIONS_TO_CHECK_WHITESPACE)


def skip_file_tabs(path: str) -> bool:
    """
    Check if a file should be skipped from tabs analysis.

    @param path Path to the file.
    @return Whether the file should be skipped or not.
    """

    filename = os.path.split(path)[1]

    if filename in FILES_TO_SKIP:
        return True

    _, extension = os.path.splitext(filename)

    return extension not in FILE_EXTENSIONS_TO_CHECK_TABS


def find_files_to_check_style(path: str) -> Tuple[List[str], List[str], List[str]]:
    """
    Find all files to be checked in a given path.

    @param path Path to check.
    @return Tuple [List of files to check formatting,
                   List of files to check trailing whitespace,
                   List of files to check tabs].
    """

    files_to_check_formatting: List[str] = []
    files_to_check_whitespace: List[str] = []
    files_to_check_tabs: List[str] = []

    abs_path = os.path.normpath(os.path.abspath(os.path.expanduser(path)))

    if os.path.isfile(abs_path):
        if not skip_file_formatting(path):
            files_to_check_formatting.append(path)

        if not skip_file_whitespace(path):
            files_to_check_whitespace.append(path)

        if not skip_file_tabs(path):
            files_to_check_tabs.append(path)

    elif os.path.isdir(abs_path):
        for dirpath, dirnames, filenames in os.walk(path, topdown=True):
            if skip_directory(dirpath):
                # Remove directory and its subdirectories
                dirnames[:] = []
                continue

            filenames = [os.path.join(dirpath, f) for f in filenames]

            for f in filenames:
                if not skip_file_formatting(f):
                    files_to_check_formatting.append(f)

                if not skip_file_whitespace(f):
                    files_to_check_whitespace.append(f)

                if not skip_file_tabs(f):
                    files_to_check_tabs.append(f)

    else:
        raise ValueError(f'Error: {path} is not a file nor a directory')

    return (
        files_to_check_formatting,
        files_to_check_whitespace,
        files_to_check_tabs,
    )


def find_clang_format_path() -> str:
    """
    Find the path to one of the supported versions of clang-format.
    If no supported version of clang-format is found, raise an exception.

    @return Path to clang-format.
    """

    # Find exact version
    for version in CLANG_FORMAT_VERSIONS:
        clang_format_path = shutil.which(f'clang-format-{version}')

        if clang_format_path:
            return clang_format_path

    # Find default version and check if it is supported
    clang_format_path = shutil.which('clang-format')

    if clang_format_path:
        process = subprocess.run(
            [clang_format_path, '--version'],
            capture_output=True,
            text=True,
            check=True,
        )

        version = process.stdout.strip().split(' ')[-1]
        major_version = int(version.split('.')[0])

        if major_version in CLANG_FORMAT_VERSIONS:
            return clang_format_path

    # No supported version of clang-format found
    raise RuntimeError(
        f'Could not find any supported version of clang-format installed on this system. '
        f'List of supported versions: {CLANG_FORMAT_VERSIONS}.'
    )


###########################################################
# CHECK STYLE
###########################################################
def check_style(path: str,
                enable_check_formatting: bool,
                enable_check_whitespace: bool,
                enable_check_tabs: bool,
                fix: bool,
                n_jobs: int = 1,
                ) -> None:
    """
    Check / fix the coding style of a list of files, including formatting and
    trailing whitespace.

    @param path Path to the files.
    @param fix Whether to fix the style of the file (True) or
               just check if the file is well-formatted (False).
    @param enable_check_formatting Whether to enable code formatting checking.
    @param enable_check_whitespace Whether to enable trailing whitespace checking.
    @param enable_check_tabs Whether to enable tabs checking.
    @param n_jobs Number of parallel jobs.
    """

    (files_to_check_formatting,
     files_to_check_whitespace,
     files_to_check_tabs) = find_files_to_check_style(path)

    check_formatting_successful = True
    check_whitespace_successful = True
    check_tabs_successful = True

    if enable_check_formatting:
        check_formatting_successful = check_formatting(
            files_to_check_formatting, fix, n_jobs)

        print('')

    if enable_check_whitespace:
        check_whitespace_successful = check_trailing_whitespace(
            files_to_check_whitespace, fix, n_jobs)

        print('')

    if enable_check_tabs:
        check_tabs_successful = check_tabs(
            files_to_check_tabs, fix, n_jobs)

    if check_formatting_successful and \
       check_whitespace_successful and \
       check_tabs_successful:
        sys.exit(0)
    else:
        sys.exit(1)


###########################################################
# CHECK FORMATTING
###########################################################
def check_formatting(filenames: List[str], fix: bool, n_jobs: int) -> bool:
    """
    Check / fix the coding style of a list of files with clang-format.

    @param filenames List of filenames to be checked.
    @param fix Whether to fix the formatting of the file (True) or
               just check if the file is well-formatted (False).
    @param n_jobs Number of parallel jobs.
    @return True if all files are well formatted after the check process.
            False if there are non-formatted files after the check process.
    """

    # Check files
    clang_format_path = find_clang_format_path()
    files_not_formatted: List[str] = []

    with concurrent.futures.ProcessPoolExecutor(n_jobs) as executor:
        files_not_formatted_results = executor.map(
            check_formatting_file,
            filenames,
            itertools.repeat(clang_format_path),
            itertools.repeat(fix),
        )

        for (filename, formatted) in files_not_formatted_results:
            if not formatted:
                files_not_formatted.append(filename)

    files_not_formatted.sort()

    # Output results
    if not files_not_formatted:
        print('All files are well formatted')
        return True

    else:
        n_non_formatted_files = len(files_not_formatted)

        if fix:
            print(f'Fixed formatting of the files ({n_non_formatted_files}):')
        else:
            print(f'Detected bad formatting in the files ({n_non_formatted_files}):')

        for f in files_not_formatted:
            print(f'- {f}')

        # Return True if all files were fixed
        return fix


def check_formatting_file(filename: str,
                          clang_format_path: str,
                          fix: bool,
                          ) -> Tuple[str, bool]:
    """
    Check / fix the coding style of a file with clang-format.

    @param filename Name of the file to be checked.
    @param clang_format_path Path to clang-format.
    @param fix Whether to fix the style of the file (True) or
               just check if the file is well-formatted (False).
    @return Tuple [Filename, Whether the file is well-formatted].
    """

    # Check if the file is well formatted
    process = subprocess.run(
        [
            clang_format_path,
            filename,
            '-style=file',
            '--dry-run',
            '--Werror',
            # Optimization: Only 1 error is needed to check that the file is not formatted
            '--ferror-limit=1',
        ],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    file_formatted = (process.returncode == 0)

    # Fix file
    if fix and not file_formatted:
        process = subprocess.run(
            [
                clang_format_path,
                filename,
                '-style=file',
                '-i',
            ],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    return (filename, file_formatted)


###########################################################
# CHECK TRAILING WHITESPACE
###########################################################
def check_trailing_whitespace(filenames: List[str], fix: bool, n_jobs: int) -> bool:
    """
    Check / fix trailing whitespace in a list of files.

    @param filename Name of the file to be checked.
    @param fix Whether to fix the file (True) or
               just check if it has trailing whitespace (False).
    @param n_jobs Number of parallel jobs.
    @return True if no files have trailing whitespace after the check process.
            False if there are trailing whitespace after the check process.
    """

    # Check files
    files_with_whitespace: List[str] = []

    with concurrent.futures.ProcessPoolExecutor(n_jobs) as executor:
        files_with_whitespace_results = executor.map(
            check_trailing_whitespace_file,
            filenames,
            itertools.repeat(fix),
        )

        for (filename, has_whitespace) in files_with_whitespace_results:
            if has_whitespace:
                files_with_whitespace.append(filename)

    files_with_whitespace.sort()

    # Output results
    if not files_with_whitespace:
        print('No files detected with trailing whitespace')
        return True

    else:
        n_files_with_whitespace = len(files_with_whitespace)

        if fix:
            print(
                f'Fixed trailing whitespace in the files ({n_files_with_whitespace}):')
        else:
            print(
                f'Detected trailing whitespace in the files ({n_files_with_whitespace}):')

        for f in files_with_whitespace:
            print(f'- {f}')

        # If all files were fixed, there are no more trailing whitespace
        return fix


def check_trailing_whitespace_file(filename: str, fix: bool) -> Tuple[str, bool]:
    """
    Check / fix trailing whitespace in a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix the file (True) or
               just check if it has trailing whitespace (False).
    @return Tuple [Filename, Whether the file has trailing whitespace].
    """

    has_trailing_whitespace = False

    with open(filename, 'r', encoding='utf-8') as f:
        file_lines = f.readlines()

    # Check if there are trailing whitespace and fix them
    for (i, line) in enumerate(file_lines):
        line_fixed = line.rstrip() + '\n'

        if line_fixed != line:
            has_trailing_whitespace = True

            # Optimization: if only checking, skip the rest of the file
            if not fix:
                break

            file_lines[i] = line_fixed

    # Update file with the fixed lines
    if fix and has_trailing_whitespace:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(file_lines)

    return (filename, has_trailing_whitespace)


###########################################################
# CHECK TABS
###########################################################
def check_tabs(filenames: List[str], fix: bool, n_jobs: int) -> bool:
    """
    Check / fix tabs in a list of files.

    @param filename Name of the file to be checked.
    @param fix Whether to fix the file (True) or just check if it has tabs (False).
    @param n_jobs Number of parallel jobs.
    @return True if no files have tabs after the check process.
            False if there are tabs after the check process.
    """

    # Check files
    files_with_tabs: List[str] = []

    with concurrent.futures.ProcessPoolExecutor(n_jobs) as executor:
        files_with_tabs_results = executor.map(
            check_tabs_file,
            filenames,
            itertools.repeat(fix),
        )

        for (filename, has_tabs) in files_with_tabs_results:
            if has_tabs:
                files_with_tabs.append(filename)

    files_with_tabs.sort()

    # Output results
    if not files_with_tabs:
        print('No files detected with tabs')
        return True

    else:
        n_files_with_tabs = len(files_with_tabs)

        if fix:
            print(
                f'Fixed tabs in the files ({n_files_with_tabs}):')
        else:
            print(
                f'Detected tabs in the files ({n_files_with_tabs}):')

        for f in files_with_tabs:
            print(f'- {f}')

        # If all files were fixed, there are no more trailing whitespace
        return fix


def check_tabs_file(filename: str, fix: bool) -> Tuple[str, bool]:
    """
    Check / fix tabs in a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix the file (True) or just check if it has tabs (False).
    @return Tuple [Filename, Whether the file has tabs].
    """

    has_tabs = False
    clang_format_enabled = True

    with open(filename, 'r', encoding='utf-8') as f:
        file_lines = f.readlines()

    for (i, line) in enumerate(file_lines):

        # Check clang-format guards
        line_stripped = line.strip()

        if line_stripped == CLANG_FORMAT_GUARD_ON:
            clang_format_enabled = True
        elif line_stripped == CLANG_FORMAT_GUARD_OFF:
            clang_format_enabled = False

        if (not clang_format_enabled and
                line_stripped not in (CLANG_FORMAT_GUARD_ON, CLANG_FORMAT_GUARD_OFF)):
            continue

        # Check if there are tabs and fix them
        if line.find('\t') != -1:
            has_tabs = True

            # Optimization: if only checking, skip the rest of the file
            if not fix:
                break

            file_lines[i] = line.expandtabs(TAB_SIZE)

    # Update file with the fixed lines
    if fix and has_tabs:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(file_lines)

    return (filename, has_tabs)


###########################################################
# MAIN
###########################################################
if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Check and apply the ns-3 coding style to all files in a given PATH. '
        'The script checks the formatting of the file with clang-format. '
        'Additionally, it checks the presence of trailing whitespace and tabs. '
        'Formatting and tabs checks respect clang-format guards. '
        'When used in "check mode" (default), the script checks if all files are well '
        'formatted and do not have trailing whitespace nor tabs. '
        'If it detects non-formatted files, they will be printed and this process exits with a '
        'non-zero code. When used in "fix mode", this script automatically fixes the files.')

    parser.add_argument('path', action='store', type=str,
                        help='Path to the files to check')

    parser.add_argument('--no-formatting', action='store_true',
                        help='Do not check / fix code formatting')

    parser.add_argument('--no-whitespace', action='store_true',
                        help='Do not check / fix trailing whitespace')

    parser.add_argument('--no-tabs', action='store_true',
                        help='Do not check / fix tabs')

    parser.add_argument('--fix', action='store_true',
                        help='Fix coding style issues detected in the files')

    parser.add_argument('-j', '--jobs', type=int, default=max(1, os.cpu_count() - 1),
                        help='Number of parallel jobs')

    args = parser.parse_args()

    try:
        check_style(
            path=args.path,
            enable_check_formatting=(not args.no_formatting),
            enable_check_whitespace=(not args.no_whitespace),
            enable_check_tabs=(not args.no_tabs),
            fix=args.fix,
            n_jobs=args.jobs,
        )

    except Exception as e:
        print(e)
        sys.exit(1)
