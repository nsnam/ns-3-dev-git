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
- Check / fix local #include headers with "ns3/" prefix. Respects clang-format guards.
- Check / apply clang-format. Respects clang-format guards.
- Check / trim trailing whitespace. Always checked.
- Check / replace tabs with spaces. Respects clang-format guards.

This script can be applied to all text files in a given path or to individual files.

NOTE: The formatting check requires clang-format (version >= 14) to be found on the path.
Trimming of trailing whitespace and conversion of tabs to spaces (via the "--no-formatting"
option) do not depend on clang-format.
"""

import argparse
import concurrent.futures
import itertools
import os
import re
import shutil
import subprocess
import sys

from typing import Callable, Dict, List, Tuple

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

FILE_EXTENSIONS_TO_CHECK_INCLUDE_PREFIXES = FILE_EXTENSIONS_TO_CHECK_FORMATTING

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
def should_analyze_directory(dirpath: str) -> bool:
    """
    Check whether a directory should be analyzed.

    @param dirpath Directory path.
    @return Whether the directory should be analyzed.
    """

    _, directory = os.path.split(dirpath)

    return not (directory in DIRECTORIES_TO_SKIP or
                (directory.startswith('.') and directory != '.'))


def should_analyze_file(path: str,
                        files_to_check: List[str],
                        file_extensions_to_check: List[str],
                        ) -> bool:
    """
    Check whether a file should be analyzed.

    @param path Path to the file.
    @param files_to_check List of files that shall be checked.
    @param file_extensions_to_check List of file extensions that shall be checked.
    @return Whether the file should be analyzed.
    """

    filename = os.path.split(path)[1]

    if filename in FILES_TO_SKIP:
        return False

    basename, extension = os.path.splitext(filename)

    return (basename in files_to_check or
            extension in file_extensions_to_check)


def find_files_to_check_style(path: str) -> Tuple[List[str], List[str], List[str], List[str]]:
    """
    Find all files to be checked in a given path.

    @param path Path to check.
    @return Tuple [List of files to check include prefixes,
                   List of files to check formatting,
                   List of files to check trailing whitespace,
                   List of files to check tabs].
    """

    files_to_check: List[str] = []
    abs_path = os.path.abspath(os.path.expanduser(path))

    if os.path.isfile(abs_path):
        files_to_check = [path]

    elif os.path.isdir(abs_path):
        for dirpath, dirnames, filenames in os.walk(path, topdown=True):
            if not should_analyze_directory(dirpath):
                # Remove directory and its subdirectories
                dirnames[:] = []
                continue

            files_to_check.extend([os.path.join(dirpath, f) for f in filenames])

    else:
        raise ValueError(f'Error: {path} is not a file nor a directory')

    files_to_check.sort()

    files_to_check_include_prefixes: List[str] = []
    files_to_check_formatting: List[str] = []
    files_to_check_whitespace: List[str] = []
    files_to_check_tabs: List[str] = []

    for f in files_to_check:
        if should_analyze_file(f, [], FILE_EXTENSIONS_TO_CHECK_INCLUDE_PREFIXES):
            files_to_check_include_prefixes.append(f)

        if should_analyze_file(f, [], FILE_EXTENSIONS_TO_CHECK_FORMATTING):
            files_to_check_formatting.append(f)

        if should_analyze_file(f, FILES_TO_CHECK_WHITESPACE, FILE_EXTENSIONS_TO_CHECK_WHITESPACE):
            files_to_check_whitespace.append(f)

        if should_analyze_file(f, [], FILE_EXTENSIONS_TO_CHECK_TABS):
            files_to_check_tabs.append(f)

    return (
        files_to_check_include_prefixes,
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
# CHECK STYLE MAIN FUNCTIONS
###########################################################
def check_style_clang_format(path: str,
                             enable_check_include_prefixes: bool,
                             enable_check_formatting: bool,
                             enable_check_whitespace: bool,
                             enable_check_tabs: bool,
                             fix: bool,
                             verbose: bool,
                             n_jobs: int = 1,
                             ) -> bool:
    """
    Check / fix the coding style of a list of files.

    @param path Path to the files.
    @param enable_check_include_prefixes Whether to enable checking #include headers from the same module with the "ns3/" prefix.
    @param enable_check_formatting Whether to enable checking code formatting.
    @param enable_check_whitespace Whether to enable checking trailing whitespace.
    @param enable_check_tabs Whether to enable checking tabs.
    @param fix Whether to fix (True) or just check (False) the file.
    @param verbose Show the lines that are not compliant with the style.
    @param n_jobs Number of parallel jobs.
    @return Whether all files are compliant with all enabled style checks.
    """

    (files_to_check_include_prefixes,
     files_to_check_formatting,
     files_to_check_whitespace,
     files_to_check_tabs) = find_files_to_check_style(path)

    check_include_prefixes_successful = True
    check_formatting_successful = True
    check_whitespace_successful = True
    check_tabs_successful = True

    if enable_check_include_prefixes:
        check_include_prefixes_successful = check_style_file(
            files_to_check_include_prefixes,
            check_include_prefixes_file,
            '#include headers from the same module with the "ns3/" prefix',
            fix,
            verbose,
            n_jobs,
        )

        print('')

    if enable_check_formatting:
        check_formatting_successful = check_style_file(
            files_to_check_formatting,
            check_formatting_file,
            'bad code formatting',
            fix,
            verbose,
            n_jobs,
            clang_format_path=find_clang_format_path(),
        )

        print('')

    if enable_check_whitespace:
        check_whitespace_successful = check_style_file(
            files_to_check_whitespace,
            check_trailing_whitespace_file,
            'trailing whitespace',
            fix,
            verbose,
            n_jobs,
        )

        print('')

    if enable_check_tabs:
        check_tabs_successful = check_style_file(
            files_to_check_tabs,
            check_tabs_file,
            'tabs',
            fix,
            verbose,
            n_jobs,
        )

    return all([
        check_include_prefixes_successful,
        check_formatting_successful,
        check_whitespace_successful,
        check_tabs_successful,
    ])


def check_style_file(filenames: List[str],
                     check_style_file_function: Callable,
                     style_check_str: str,
                     fix: bool,
                     verbose: bool,
                     n_jobs: int,
                     **kwargs,
                     ) -> bool:
    """
    Check / fix style of a list of files.

    @param filename Name of the file to be checked.
    @param check_style_file_function Function used to check the file.
    @param style_check_str Description of the check to be performed.
    @param fix Whether to fix (True) or just check (False) the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @param n_jobs Number of parallel jobs.
    @param kwargs Additional keyword arguments to the check_style_file_function.
    @return Whether all files are compliant with the style.
    """

    # Check files
    non_compliant_files: List[str] = []
    files_verbose_infos: Dict[str, List[str]] = {}

    with concurrent.futures.ProcessPoolExecutor(n_jobs) as executor:
        non_compliant_files_results = executor.map(
            check_style_file_function,
            filenames,
            itertools.repeat(fix),
            itertools.repeat(verbose),
            *[arg if isinstance(arg, list) else itertools.repeat(arg) for arg in kwargs.values()],
        )

        for (filename, is_file_compliant, verbose_infos) in non_compliant_files_results:
            if not is_file_compliant:
                non_compliant_files.append(filename)

            if verbose:
                files_verbose_infos[filename] = verbose_infos

    # Output results
    if not non_compliant_files:
        print(f'- No files detected with {style_check_str}')
        return True

    else:
        n_non_compliant_files = len(non_compliant_files)

        if fix:
            print(f'- Fixed {style_check_str} in the files ({n_non_compliant_files}):')
        else:
            print(f'- Detected {style_check_str} in the files ({n_non_compliant_files}):')

        for f in non_compliant_files:
            if verbose:
                print(*[f'    {l}' for l in files_verbose_infos[f]], sep='\n')
            else:
                print(f'    - {f}')

        # If all files were fixed, there are no more non-compliant files
        return fix


###########################################################
# CHECK STYLE FUNCTIONS
###########################################################
def check_include_prefixes_file(filename: str,
                                fix: bool,
                                verbose: bool,
                                ) -> Tuple[str, bool, List[str]]:
    """
    Check / fix #include headers from the same module with the "ns3/" prefix in a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the style of the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    is_file_compliant = True
    clang_format_enabled = True

    verbose_infos: List[str] = []

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

        # Check if the line is an #include and extract its header file
        header_file = re.findall(r'^#include ["<]ns3/(.*\.h)[">]', line_stripped)

        if not header_file:
            continue

        # Check if the header file belongs to the same module and remove the "ns3/" prefix
        header_file = header_file[0]
        parent_path = os.path.split(filename)[0]

        if not os.path.exists(os.path.join(parent_path, header_file)):
            continue

        is_file_compliant = False
        file_lines[i] = line_stripped.replace(
            f'ns3/{header_file}', header_file).replace('<', '"').replace('>', '"') + '\n'

        if verbose:
            header_index = len('#include "')

            verbose_infos.extend([
                f'{filename}:{i + 1}:{header_index + 1}: error: #include headers from the same module with the "ns3/" prefix detected',
                f'    {line_stripped}',
                f'    {"":{header_index}}^',
            ])

        # Optimization: If running in non-verbose check mode, only one error is needed to check that the file is not compliant
        if not fix and not verbose:
            break

    # Update file with the fixed lines
    if fix and not is_file_compliant:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(file_lines)

    return (filename, is_file_compliant, verbose_infos)


def check_formatting_file(filename: str,
                          fix: bool,
                          verbose: bool,
                          clang_format_path: str,
                          ) -> Tuple[str, bool, List[str]]:
    """
    Check / fix the coding style of a file with clang-format.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the style of the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @param clang_format_path Path to clang-format.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    verbose_infos: List[str] = []

    # Check if the file is well formatted
    process = subprocess.run(
        [
            clang_format_path,
            filename,
            '-style=file',
            '--dry-run',
            '--Werror',
            # Optimization: In non-verbose mode, only one error is needed to check that the file is not compliant
            f'--ferror-limit={0 if verbose else 1}',
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    is_file_compliant = (process.returncode == 0)

    if verbose:
        verbose_infos = process.stderr.splitlines()

    # Fix file
    if fix and not is_file_compliant:
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

    return (filename, is_file_compliant, verbose_infos)


def check_trailing_whitespace_file(filename: str,
                                   fix: bool,
                                   verbose: bool,
                                   ) -> Tuple[str, bool, List[str]]:
    """
    Check / fix trailing whitespace in a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the style of the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    is_file_compliant = True
    verbose_infos: List[str] = []

    with open(filename, 'r', encoding='utf-8') as f:
        file_lines = f.readlines()

    # Check if there are trailing whitespace and fix them
    for (i, line) in enumerate(file_lines):
        line_fixed = line.rstrip() + '\n'

        if line_fixed == line:
            continue

        is_file_compliant = False
        file_lines[i] = line_fixed

        if verbose:
            line_fixed_stripped_expanded = line_fixed.rstrip().expandtabs(TAB_SIZE)

            verbose_infos.extend([
                f'{filename}:{i + 1}:{len(line_fixed_stripped_expanded) + 1}: error: Trailing whitespace detected',
                f'    {line_fixed_stripped_expanded}',
                f'    {"":{len(line_fixed_stripped_expanded)}}^',
            ])

        # Optimization: If running in non-verbose check mode, only one error is needed to check that the file is not compliant
        if not fix and not verbose:
            break

    # Update file with the fixed lines
    if fix and not is_file_compliant:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(file_lines)

    return (filename, is_file_compliant, verbose_infos)


def check_tabs_file(filename: str,
                    fix: bool,
                    verbose: bool,
                    ) -> Tuple[str, bool, List[str]]:
    """
    Check / fix tabs in a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the style of the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    is_file_compliant = True
    clang_format_enabled = True

    verbose_infos: List[str] = []

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
        tab_index = line.find('\t')

        if tab_index == -1:
            continue

        is_file_compliant = False
        file_lines[i] = line.expandtabs(TAB_SIZE)

        if verbose:
            verbose_infos.extend([
                f'{filename}:{i + 1}:{tab_index + 1}: error: Tab detected',
                f'    {line.rstrip()}',
                f'    {"":{tab_index}}^',
            ])

        # Optimization: If running in non-verbose check mode, only one error is needed to check that the file is not compliant
        if not fix and not verbose:
            break

    # Update file with the fixed lines
    if fix and not is_file_compliant:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(file_lines)

    return (filename, is_file_compliant, verbose_infos)


###########################################################
# MAIN
###########################################################
if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Check and apply the ns-3 coding style to all files in a given PATH. '
        'The script checks the formatting of the file with clang-format. '
        'Additionally, it checks #include headers from the same module with the "ns3/" prefix, '
        'the presence of trailing whitespace and tabs. '
        'Formatting, local #include "ns3/" prefixes and tabs checks respect clang-format guards. '
        'When used in "check mode" (default), the script checks if all files are well '
        'formatted and do not have trailing whitespace nor tabs. '
        'If it detects non-formatted files, they will be printed and this process exits with a '
        'non-zero code. When used in "fix mode", this script automatically fixes the files.')

    parser.add_argument('path', action='store', type=str,
                        help='Path to the files to check')

    parser.add_argument('--no-include-prefixes', action='store_true',
                        help='Do not check / fix #include headers from the same module with the "ns3/" prefix')

    parser.add_argument('--no-formatting', action='store_true',
                        help='Do not check / fix code formatting')

    parser.add_argument('--no-whitespace', action='store_true',
                        help='Do not check / fix trailing whitespace')

    parser.add_argument('--no-tabs', action='store_true',
                        help='Do not check / fix tabs')

    parser.add_argument('--fix', action='store_true',
                        help='Fix coding style issues detected in the files')

    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Show the lines that are not well-formatted')

    parser.add_argument('-j', '--jobs', type=int, default=max(1, os.cpu_count() - 1),
                        help='Number of parallel jobs')

    args = parser.parse_args()

    try:
        all_checks_successful = check_style_clang_format(
            path=args.path,
            enable_check_include_prefixes=(not args.no_include_prefixes),
            enable_check_formatting=(not args.no_formatting),
            enable_check_whitespace=(not args.no_whitespace),
            enable_check_tabs=(not args.no_tabs),
            fix=args.fix,
            verbose=args.verbose,
            n_jobs=args.jobs,
        )

    except Exception as e:
        print(e)
        sys.exit(1)

    if all_checks_successful:
        sys.exit(0)
    else:
        sys.exit(1)
