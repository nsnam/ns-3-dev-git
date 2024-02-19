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
Check and apply the ns-3 coding style recursively to all files in the PATH arguments.

The coding style is defined with the clang-format tool, whose definitions are in
the ".clang-format" file. This script performs the following checks / fixes:
- Check / fix local #include headers with "ns3/" prefix. Respects clang-format guards.
- Check / apply clang-format. Respects clang-format guards.
- Check / trim trailing whitespace. Always checked.
- Check / replace tabs with spaces. Respects clang-format guards.

This script can be applied to all text files in a given path or to individual files.

NOTE: The formatting check requires clang-format (version >= 14) to be found on the path.
The remaining checks do not depend on clang-format and can be executed by disabling clang-format
checking with the "--no-formatting" option.
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
    17,
    16,
    15,
    14,
]

CLANG_FORMAT_GUARD_ON = "// clang-format on"
CLANG_FORMAT_GUARD_OFF = "// clang-format off"

DIRECTORIES_TO_SKIP = [
    "__pycache__",
    ".git",
    ".venv",
    "bindings",
    "build",
    "cmake-cache",
    "testpy-output",
    "venv",
]

# List of files entirely copied from elsewhere that should not be checked,
# in order to optimize the performance of this script
FILES_TO_SKIP = [
    "valgrind.h",
]

FILE_EXTENSIONS_TO_CHECK_FORMATTING = [
    ".c",
    ".cc",
    ".h",
]

FILE_EXTENSIONS_TO_CHECK_INCLUDE_PREFIXES = FILE_EXTENSIONS_TO_CHECK_FORMATTING

FILE_EXTENSIONS_TO_CHECK_TABS = [
    ".c",
    ".cc",
    ".cmake",
    ".css",
    ".h",
    ".html",
    ".js",
    ".json",
    ".m",
    ".md",
    ".pl",
    ".py",
    ".rst",
    ".sh",
    ".toml",
    ".yml",
]

FILES_TO_CHECK_TABS = [
    ".clang-format",
    ".clang-tidy",
    ".codespellrc",
    "CMakeLists.txt",
    "codespell-ignored-lines",
    "codespell-ignored-words",
    "ns3",
]

FILE_EXTENSIONS_TO_CHECK_WHITESPACE = FILE_EXTENSIONS_TO_CHECK_TABS + [
    ".click",
    ".cfg",
    ".conf",
    ".dot",
    ".gnuplot",
    ".gp",
    ".mob",
    ".ns_params",
    ".ns_movements",
    ".params",
    ".plt",
    ".seqdiag",
    ".txt",
]

FILES_TO_CHECK_WHITESPACE = FILES_TO_CHECK_TABS + [
    "Makefile",
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

    return directory not in DIRECTORIES_TO_SKIP


def should_analyze_file(
    path: str,
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

    extension = os.path.splitext(filename)[1]

    return filename in files_to_check or extension in file_extensions_to_check


def find_files_to_check_style(
    paths: List[str],
) -> Tuple[List[str], List[str], List[str], List[str]]:
    """
    Find all files to be checked in a given list of paths.

    @param paths List of paths to the files to check.
    @return Tuple [List of files to check include prefixes,
                   List of files to check formatting,
                   List of files to check trailing whitespace,
                   List of files to check tabs].
    """

    files_to_check: List[str] = []

    for path in paths:
        abs_path = os.path.abspath(os.path.expanduser(path))

        if os.path.isfile(abs_path):
            files_to_check.append(path)

        elif os.path.isdir(abs_path):
            for dirpath, dirnames, filenames in os.walk(path, topdown=True):
                if not should_analyze_directory(dirpath):
                    # Remove directory and its subdirectories
                    dirnames[:] = []
                    continue

                files_to_check.extend([os.path.join(dirpath, f) for f in filenames])

        else:
            raise ValueError(f"Error: {path} is not a file nor a directory")

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

        if should_analyze_file(f, FILES_TO_CHECK_TABS, FILE_EXTENSIONS_TO_CHECK_TABS):
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
        clang_format_path = shutil.which(f"clang-format-{version}")

        if clang_format_path:
            return clang_format_path

    # Find default version and check if it is supported
    clang_format_path = shutil.which("clang-format")

    if clang_format_path:
        process = subprocess.run(
            [clang_format_path, "--version"],
            capture_output=True,
            text=True,
            check=True,
        )

        version = process.stdout.strip().split(" ")[-1]
        major_version = int(version.split(".")[0])

        if major_version in CLANG_FORMAT_VERSIONS:
            return clang_format_path

    # No supported version of clang-format found
    raise RuntimeError(
        f"Could not find any supported version of clang-format installed on this system. "
        f"List of supported versions: {CLANG_FORMAT_VERSIONS}."
    )


###########################################################
# CHECK STYLE MAIN FUNCTIONS
###########################################################
def check_style_clang_format(
    paths: List[str],
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

    @param paths List of paths to the files to check.
    @param enable_check_include_prefixes Whether to enable checking #include headers from the same module with the "ns3/" prefix.
    @param enable_check_formatting Whether to enable checking code formatting.
    @param enable_check_whitespace Whether to enable checking trailing whitespace.
    @param enable_check_tabs Whether to enable checking tabs.
    @param fix Whether to fix (True) or just check (False) the file.
    @param verbose Show the lines that are not compliant with the style.
    @param n_jobs Number of parallel jobs.
    @return Whether all files are compliant with all enabled style checks.
    """

    (
        files_to_check_include_prefixes,
        files_to_check_formatting,
        files_to_check_whitespace,
        files_to_check_tabs,
    ) = find_files_to_check_style(paths)

    check_include_prefixes_successful = True
    check_formatting_successful = True
    check_whitespace_successful = True
    check_tabs_successful = True

    if enable_check_include_prefixes:
        check_include_prefixes_successful = check_style_files(
            '#include headers from the same module with the "ns3/" prefix',
            check_manually_file,
            files_to_check_include_prefixes,
            fix,
            verbose,
            n_jobs,
            respect_clang_format_guards=True,
            check_style_line_function=check_include_prefixes_line,
        )

        print("")

    if enable_check_formatting:
        check_formatting_successful = check_style_files(
            "bad code formatting",
            check_formatting_file,
            files_to_check_formatting,
            fix,
            verbose,
            n_jobs,
            clang_format_path=find_clang_format_path(),
        )

        print("")

    if enable_check_whitespace:
        check_whitespace_successful = check_style_files(
            "trailing whitespace",
            check_manually_file,
            files_to_check_whitespace,
            fix,
            verbose,
            n_jobs,
            respect_clang_format_guards=False,
            check_style_line_function=check_whitespace_line,
        )

        print("")

    if enable_check_tabs:
        check_tabs_successful = check_style_files(
            "tabs",
            check_manually_file,
            files_to_check_tabs,
            fix,
            verbose,
            n_jobs,
            respect_clang_format_guards=True,
            check_style_line_function=check_tabs_line,
        )

    return all(
        [
            check_include_prefixes_successful,
            check_formatting_successful,
            check_whitespace_successful,
            check_tabs_successful,
        ]
    )


def check_style_files(
    style_check_str: str,
    check_style_file_function: Callable[..., Tuple[str, bool, List[str]]],
    filenames: List[str],
    fix: bool,
    verbose: bool,
    n_jobs: int,
    **kwargs,
) -> bool:
    """
    Check / fix style of a list of files.

    @param style_check_str Description of the check to be performed.
    @param check_style_file_function Function used to check the file.
    @param filename Name of the file to be checked.
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

        for filename, is_file_compliant, verbose_infos in non_compliant_files_results:
            if not is_file_compliant:
                non_compliant_files.append(filename)

            if verbose:
                files_verbose_infos[filename] = verbose_infos

    # Output results
    if not non_compliant_files:
        print(f"- No files detected with {style_check_str}")
        return True

    else:
        n_non_compliant_files = len(non_compliant_files)

        if fix:
            print(f"- Fixed {style_check_str} in the files ({n_non_compliant_files}):")
        else:
            print(f"- Detected {style_check_str} in the files ({n_non_compliant_files}):")

        for f in non_compliant_files:
            if verbose:
                print(*[f"    {l}" for l in files_verbose_infos[f]], sep="\n")
            else:
                print(f"    - {f}")

        # If all files were fixed, there are no more non-compliant files
        return fix


###########################################################
# CHECK STYLE FUNCTIONS
###########################################################
def check_formatting_file(
    filename: str,
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
            "-style=file",
            "--dry-run",
            "--Werror",
            # Optimization: In non-verbose mode, only one error is needed to check that the file is not compliant
            f"--ferror-limit={0 if verbose else 1}",
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    is_file_compliant = process.returncode == 0

    if verbose:
        verbose_infos = process.stderr.splitlines()

    # Fix file
    if fix and not is_file_compliant:
        process = subprocess.run(
            [
                clang_format_path,
                filename,
                "-style=file",
                "-i",
            ],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    return (filename, is_file_compliant, verbose_infos)


def check_manually_file(
    filename: str,
    fix: bool,
    verbose: bool,
    respect_clang_format_guards: bool,
    check_style_line_function: Callable[[str, str, int], Tuple[bool, str, List[str]]],
) -> Tuple[str, bool, List[str]]:
    """
    Check / fix a file manually using a function to check / fix each line.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the style of the file (True).
    @param verbose Show the lines that are not compliant with the style.
    @param respect_clang_format_guards Whether to respect clang-format guards.
    @param check_style_line_function Function used to check each line.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    is_file_compliant = True
    verbose_infos: List[str] = []
    clang_format_enabled = True

    with open(filename, "r", encoding="utf-8") as f:
        file_lines = f.readlines()

    for i, line in enumerate(file_lines):
        # Check clang-format guards
        if respect_clang_format_guards:
            line_stripped = line.strip()

            if line_stripped == CLANG_FORMAT_GUARD_ON:
                clang_format_enabled = True
            elif line_stripped == CLANG_FORMAT_GUARD_OFF:
                clang_format_enabled = False

            if not clang_format_enabled and line_stripped not in (
                CLANG_FORMAT_GUARD_ON,
                CLANG_FORMAT_GUARD_OFF,
            ):
                continue

        # Check if the line is compliant with the style and fix it
        (is_line_compliant, line_fixed, line_verbose_infos) = check_style_line_function(
            line, filename, i
        )

        if not is_line_compliant:
            is_file_compliant = False
            file_lines[i] = line_fixed
            verbose_infos.extend(line_verbose_infos)

            # Optimization: If running in non-verbose check mode, only one error is needed to check that the file is not compliant
            if not fix and not verbose:
                break

    # Update file with the fixed lines
    if fix and not is_file_compliant:
        with open(filename, "w", encoding="utf-8") as f:
            f.writelines(file_lines)

    return (filename, is_file_compliant, verbose_infos)


def check_include_prefixes_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix #include headers from the same module with the "ns3/" prefix in a line.

    @param line The line to check.
    @param filename Name of the file to be checked.
    @param line_number The number of the line checked.
    @return Tuple [Whether the line is compliant with the style (before the check),
                   Fixed line,
                   Verbose information].
    """

    is_line_compliant = True
    line_fixed = line
    verbose_infos: List[str] = []

    # Check if the line is an #include and extract its header file
    line_stripped = line.strip()
    header_file = re.findall(r'^#include ["<]ns3/(.*\.h)[">]', line_stripped)

    if header_file:
        # Check if the header file belongs to the same module and remove the "ns3/" prefix
        header_file = header_file[0]
        parent_path = os.path.split(filename)[0]

        if os.path.exists(os.path.join(parent_path, header_file)):
            is_line_compliant = False
            line_fixed = (
                line_stripped.replace(f"ns3/{header_file}", header_file)
                .replace("<", '"')
                .replace(">", '"')
                + "\n"
            )

            header_index = len('#include "')

            verbose_infos.extend(
                [
                    f'{filename}:{line_number + 1}:{header_index + 1}: error: #include headers from the same module with the "ns3/" prefix detected',
                    f"    {line_stripped}",
                    f'    {"":{header_index}}^',
                ]
            )

    return (is_line_compliant, line_fixed, verbose_infos)


def check_whitespace_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix whitespace in a line.

    @param line The line to check.
    @param filename Name of the file to be checked.
    @param line_number The number of the line checked.
    @return Tuple [Whether the line is compliant with the style (before the check),
                   Fixed line,
                   Verbose information].
    """

    is_line_compliant = True
    line_fixed = line.rstrip() + "\n"
    verbose_infos: List[str] = []

    if line_fixed != line:
        is_line_compliant = False
        line_fixed_stripped_expanded = line_fixed.rstrip().expandtabs(TAB_SIZE)

        verbose_infos = [
            f"{filename}:{line_number + 1}:{len(line_fixed_stripped_expanded) + 1}: error: Trailing whitespace detected",
            f"    {line_fixed_stripped_expanded}",
            f'    {"":{len(line_fixed_stripped_expanded)}}^',
        ]

    return (is_line_compliant, line_fixed, verbose_infos)


def check_tabs_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix tabs in a line.

    @param line The line to check.
    @param filename Name of the file to be checked.
    @param line_number The number of the line checked.
    @return Tuple [Whether the line is compliant with the style (before the check),
                   Fixed line,
                   Verbose information].
    """

    is_line_compliant = True
    line_fixed = line
    verbose_infos: List[str] = []

    tab_index = line.find("\t")

    if tab_index != -1:
        is_line_compliant = False
        line_fixed = line.expandtabs(TAB_SIZE)

        verbose_infos = [
            f"{filename}:{line_number + 1}:{tab_index + 1}: error: Tab detected",
            f"    {line.rstrip()}",
            f'    {"":{tab_index}}^',
        ]

    return (is_line_compliant, line_fixed, verbose_infos)


###########################################################
# MAIN
###########################################################
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Check and apply the ns-3 coding style recursively to all files in the given PATHs. "
        "The script checks the formatting of the file with clang-format. "
        'Additionally, it checks #include headers from the same module with the "ns3/" prefix, '
        "the presence of trailing whitespace and tabs. "
        'Formatting, local #include "ns3/" prefixes and tabs checks respect clang-format guards. '
        'When used in "check mode" (default), the script checks if all files are well '
        "formatted and do not have trailing whitespace nor tabs. "
        "If it detects non-formatted files, they will be printed and this process exits with a "
        'non-zero code. When used in "fix mode", this script automatically fixes the files.'
    )

    parser.add_argument(
        "paths",
        action="store",
        type=str,
        nargs="+",
        help="List of paths to the files to check",
    )

    parser.add_argument(
        "--no-include-prefixes",
        action="store_true",
        help='Do not check / fix #include headers from the same module with the "ns3/" prefix',
    )

    parser.add_argument(
        "--no-formatting",
        action="store_true",
        help="Do not check / fix code formatting",
    )

    parser.add_argument(
        "--no-whitespace",
        action="store_true",
        help="Do not check / fix trailing whitespace",
    )

    parser.add_argument(
        "--no-tabs",
        action="store_true",
        help="Do not check / fix tabs",
    )

    parser.add_argument(
        "--fix",
        action="store_true",
        help="Fix coding style issues detected in the files",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show the lines that are not well-formatted",
    )

    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=max(1, os.cpu_count() - 1),
        help="Number of parallel jobs",
    )

    args = parser.parse_args()

    try:
        all_checks_successful = check_style_clang_format(
            paths=args.paths,
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

    if not all_checks_successful:
        if args.verbose:
            print("")
            print('NOTE: To fix the files automatically, run this script with the flag "--fix"')

        sys.exit(1)
