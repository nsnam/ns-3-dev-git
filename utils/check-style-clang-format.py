#!/usr/bin/env python3

# Copyright (c) 2022 Eduardo Nuno Almeida.
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Eduardo Nuno Almeida <enmsa@outlook.pt> [INESC TEC and FEUP, Portugal]

"""
Check and apply the ns-3 coding style recursively to all files in the PATH arguments.

The coding style is defined with the clang-format tool, whose definitions are in
the ".clang-format" file. This script performs the following checks / fixes:
- Check / apply clang-format. Respects clang-format guards.
- Check / fix local #include headers with "ns3/" prefix. Respects clang-format guards.
- Check / fix ns-3 #include headers using angle brackets <> rather than quotes "". Respects clang-format guards.
- Check / fix Doxygen tags using @ rather than \\. Respects clang-format guards.
- Check / fix SPDX licenses rather than GPL text. Respects clang-format guards.
- Check / fix emacs file style comments. Respects clang-format guards.
- Check / trim trailing whitespace. Always checked.
- Check / replace tabs with spaces. Respects clang-format guards.
- Check file encoding. Always checked.

This script can be applied to all text files in a given path or to individual files.

NOTE: The formatting check requires clang-format to be found on the path (see the supported versions below).
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
CLANG_FORMAT_MAX_VERSION = 18
CLANG_FORMAT_MIN_VERSION = 15

FORMAT_GUARD_ON = [
    "// clang-format on",
    "# cmake-format: on",
    "# fmt: on",
]

FORMAT_GUARD_OFF = [
    "// clang-format off",
    "# cmake-format: off",
    "# fmt: off",
]

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

# List of checks
CHECKS = [
    "include_prefixes",
    "include_quotes",
    "doxygen_tags",
    "license",
    "emacs",
    "whitespace",
    "tabs",
    "formatting",
    "encoding",
]

# Files to check
FILES_TO_CHECK: Dict[str, List[str]] = {c: [] for c in CHECKS}

FILES_TO_CHECK["tabs"] = [
    ".clang-format",
    ".clang-tidy",
    ".codespellrc",
    "CMakeLists.txt",
    "codespell-ignored-lines",
    "codespell-ignored-words",
    "ns3",
]

FILES_TO_CHECK["whitespace"] = FILES_TO_CHECK["tabs"] + [
    "Makefile",
]

# File extensions to check
FILE_EXTENSIONS_TO_CHECK: Dict[str, List[str]] = {c: [] for c in CHECKS}

FILE_EXTENSIONS_TO_CHECK["formatting"] = [
    ".c",
    ".cc",
    ".h",
]

FILE_EXTENSIONS_TO_CHECK["include_prefixes"] = FILE_EXTENSIONS_TO_CHECK["formatting"]
FILE_EXTENSIONS_TO_CHECK["include_quotes"] = FILE_EXTENSIONS_TO_CHECK["formatting"]
FILE_EXTENSIONS_TO_CHECK["doxygen_tags"] = FILE_EXTENSIONS_TO_CHECK["formatting"]
FILE_EXTENSIONS_TO_CHECK["encoding"] = FILE_EXTENSIONS_TO_CHECK["formatting"]

FILE_EXTENSIONS_TO_CHECK["license"] = [
    ".c",
    ".cc",
    ".cmake",
    ".h",
    ".py",
]

FILE_EXTENSIONS_TO_CHECK["emacs"] = [
    ".c",
    ".cc",
    ".h",
    ".py",
    ".rst",
]

FILE_EXTENSIONS_TO_CHECK["tabs"] = [
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

FILE_EXTENSIONS_TO_CHECK["whitespace"] = FILE_EXTENSIONS_TO_CHECK["tabs"] + [
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

# Other check parameters
TAB_SIZE = 4
FILE_ENCODING = "UTF-8"


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
) -> Dict[str, List[str]]:
    """
    Find all files to be checked in a given list of paths.

    @param paths List of paths to the files to check.
    @return Dictionary of checks and corresponding list of files to check.
            Example: {
                "formatting": list_of_files_to_check_formatting,
                ...,
            }
    """

    # Get list of files found in the given path
    files_found: List[str] = []

    for path in paths:
        abs_path = os.path.abspath(os.path.expanduser(path))

        if os.path.isfile(abs_path):
            files_found.append(path)

        elif os.path.isdir(abs_path):
            for dirpath, dirnames, filenames in os.walk(path, topdown=True):
                if not should_analyze_directory(dirpath):
                    # Remove directory and its subdirectories
                    dirnames[:] = []
                    continue

                files_found.extend([os.path.join(dirpath, f) for f in filenames])

        else:
            raise ValueError(f"{path} is not a valid file nor a directory")

    files_found.sort()

    # Check which files should be checked
    files_to_check: Dict[str, List[str]] = {c: [] for c in CHECKS}

    for f in files_found:
        for check in CHECKS:
            if should_analyze_file(f, FILES_TO_CHECK[check], FILE_EXTENSIONS_TO_CHECK[check]):
                files_to_check[check].append(f)

    return files_to_check


def find_clang_format_path() -> str:
    """
    Find the path to one of the supported versions of clang-format.
    If no supported version of clang-format is found, raise an exception.

    @return Path to clang-format.
    """

    # Find exact version, starting from the most recent one
    for version in range(CLANG_FORMAT_MAX_VERSION, CLANG_FORMAT_MIN_VERSION - 1, -1):
        clang_format_path = shutil.which(f"clang-format-{version}")

        if clang_format_path:
            return clang_format_path

    # Find default version and check if it is supported
    clang_format_path = shutil.which("clang-format")
    major_version = None

    if clang_format_path:
        process = subprocess.run(
            [clang_format_path, "--version"],
            capture_output=True,
            text=True,
            check=True,
        )

        clang_format_version = process.stdout.strip()
        version_regex = re.findall(r"\b(\d+)(\.\d+){0,2}\b", clang_format_version)

        if version_regex:
            major_version = int(version_regex[0][0])

            if CLANG_FORMAT_MIN_VERSION <= major_version <= CLANG_FORMAT_MAX_VERSION:
                return clang_format_path

    # No supported version of clang-format found
    raise RuntimeError(
        f"Could not find any supported version of clang-format installed on this system. "
        f"List of supported versions: [{CLANG_FORMAT_MAX_VERSION}-{CLANG_FORMAT_MIN_VERSION}]. "
        + (f"Found clang-format {major_version}." if major_version else "")
    )


###########################################################
# CHECK STYLE MAIN FUNCTIONS
###########################################################
def check_style_clang_format(
    paths: List[str],
    checks_enabled: Dict[str, bool],
    fix: bool,
    verbose: bool,
    n_jobs: int = 1,
) -> bool:
    """
    Check / fix the coding style of a list of files.

    @param paths List of paths to the files to check.
    @param checks_enabled Dictionary of checks indicating whether to enable each of them.
    @param fix Whether to fix (True) or just check (False) the file.
    @param verbose Show the lines that are not compliant with the style.
    @param n_jobs Number of parallel jobs.
    @return Whether all files are compliant with all enabled style checks.
    """

    files_to_check = find_files_to_check_style(paths)
    checks_successful = {c: True for c in CHECKS}

    style_check_strs = {
        "include_prefixes": '#include headers from the same module with the "ns3/" prefix',
        "include_quotes": 'ns-3 #include headers using angle brackets <> rather than quotes ""',
        "doxygen_tags": "Doxygen tags using \\ rather than @",
        "license": "GPL license text instead of SPDX license",
        "emacs": "emacs file style comments",
        "whitespace": "trailing whitespace",
        "tabs": "tabs",
        "formatting": "bad code formatting",
        "encoding": f"bad file encoding ({FILE_ENCODING})",
    }

    check_style_file_functions_kwargs = {
        "include_prefixes": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_include_prefixes_line,
            },
        },
        "include_quotes": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_include_quotes_line,
            },
        },
        "doxygen_tags": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_doxygen_tags_line,
            },
        },
        "license": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_licenses_line,
            },
        },
        "emacs": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_emacs_line,
            },
        },
        "whitespace": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": False,
                "check_style_line_function": check_whitespace_line,
            },
        },
        "tabs": {
            "function": check_manually_file,
            "kwargs": {
                "respect_clang_format_guards": True,
                "check_style_line_function": check_tabs_line,
            },
        },
        "formatting": {
            "function": check_formatting_file,
            "kwargs": {},  # The formatting keywords are added below
        },
        "encoding": {
            "function": check_encoding_file,
            "kwargs": {},
        },
    }

    if checks_enabled["formatting"]:
        check_style_file_functions_kwargs["formatting"]["kwargs"] = {
            "clang_format_path": find_clang_format_path(),
        }

    n_checks_enabled = sum(checks_enabled.values())
    n_check = 0

    for check in CHECKS:
        if checks_enabled[check]:
            checks_successful[check] = check_style_files(
                style_check_strs[check],
                check_style_file_functions_kwargs[check]["function"],
                files_to_check[check],
                fix,
                verbose,
                n_jobs,
                **check_style_file_functions_kwargs[check]["kwargs"],
            )

            n_check += 1

            if n_check < n_checks_enabled:
                print("")

    return all(checks_successful.values())


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
    @param fix Whether to fix (True) or just check (False) the style of the file.
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


def check_encoding_file(
    filename: str,
    fix: bool,
    verbose: bool,
) -> Tuple[str, bool, List[str]]:
    """
    Check / fix the encoding of a file.

    @param filename Name of the file to be checked.
    @param fix Whether to fix (True) or just check (False) the encoding of the file.
    @param verbose Show the lines that are not compliant with the style.
    @return Tuple [Filename,
                   Whether the file is compliant with the style (before the check),
                   Verbose information].
    """

    verbose_infos: List[str] = []
    is_file_compliant = True

    with open(filename, "rb") as f:
        file_data = f.read()
        file_lines = file_data.decode(FILE_ENCODING, errors="replace").splitlines(keepends=True)

        # Check if file has correct encoding
        try:
            file_data.decode(FILE_ENCODING)

        except UnicodeDecodeError as e:
            is_file_compliant = False

            if verbose:
                # Find line and column with bad encoding
                bad_char_start_index = e.start
                n_chars_file_read = 0

                for line_number, line in enumerate(file_lines):
                    n_chars_line = len(line)

                    if bad_char_start_index < n_chars_file_read + n_chars_line:
                        bad_char_column = bad_char_start_index - n_chars_file_read

                        verbose_infos.extend(
                            [
                                f"{filename}:{line_number + 1}:{bad_char_column + 1}: error: bad {FILE_ENCODING} encoding",
                                f"    {line.rstrip()}",
                                f"    {'':>{bad_char_column}}^",
                            ]
                        )

                        break

                    n_chars_file_read += n_chars_line

    # Fix file encoding
    if fix and not is_file_compliant:
        with open(filename, "w", encoding=FILE_ENCODING) as f:
            f.writelines(file_lines)

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
    @param fix Whether to fix (True) or just check (False) the style of the file.
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

    with open(filename, "r", encoding=FILE_ENCODING) as f:
        file_lines = f.readlines()

    for i, line in enumerate(file_lines):
        # Check clang-format guards
        if respect_clang_format_guards:
            line_stripped = line.strip()

            if line_stripped in FORMAT_GUARD_ON:
                clang_format_enabled = True
            elif line_stripped in FORMAT_GUARD_OFF:
                clang_format_enabled = False

            if not clang_format_enabled and line_stripped not in (
                FORMAT_GUARD_ON + FORMAT_GUARD_OFF
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
        with open(filename, "w", encoding=FILE_ENCODING) as f:
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
                    f"    {'':>{header_index}}^",
                ]
            )

    return (is_line_compliant, line_fixed, verbose_infos)


def check_include_quotes_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix ns-3 #include headers using angle brackets <> rather than quotes "" in a line.

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

    # Check if the line is an #include <ns3/...>
    header_file = re.findall(r"^#include <ns3/.*\.h>", line)

    if header_file:
        is_line_compliant = False
        line_fixed = line.replace("<", '"').replace(">", '"')

        header_index = len("#include ")

        verbose_infos = [
            f"{filename}:{line_number + 1}:{header_index + 1}: error: ns-3 #include headers with angle brackets detected",
            f"    {line}",
            f"    {'':{header_index}}^",
        ]

    return (is_line_compliant, line_fixed, verbose_infos)


def check_doxygen_tags_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix Doxygen tags using \\ rather than @ in a line.

    @param line The line to check.
    @param filename Name of the file to be checked.
    @param line_number The number of the line checked.
    @return Tuple [Whether the line is compliant with the style (before the check),
                   Fixed line,
                   Verbose information].
    """

    IGNORED_WORDS = [
        "\\dots",
        "\\langle",
        "\\quad",
    ]

    is_line_compliant = True
    line_fixed = line
    verbose_infos: List[str] = []

    # Match Doxygen tags at the start of the line (e.g., "* \param arg Description")
    line_stripped = line.rstrip()
    regex_findings = re.findall(r"^\s*(?:\*|\/\*\*|\/\/\/)\s*(\\\w{3,})(?=(?:\s|$))", line_stripped)

    if regex_findings:
        doxygen_tag = regex_findings[0]

        if doxygen_tag not in IGNORED_WORDS:
            is_line_compliant = False

            doxygen_tag_index = line_fixed.find(doxygen_tag)
            line_fixed = line.replace(doxygen_tag, f"@{doxygen_tag[1:]}")

            verbose_infos.extend(
                [
                    f"{filename}:{line_number + 1}:{doxygen_tag_index + 1}: error: detected Doxygen tags using \\ rather than @",
                    f"    {line_stripped}",
                    f"    {'':{doxygen_tag_index}}^",
                ]
            )

    return (is_line_compliant, line_fixed, verbose_infos)


def check_licenses_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix SPDX licenses rather than GPL text in a line.

    @param line The line to check.
    @param filename Name of the file to be checked.
    @param line_number The number of the line checked.
    @return Tuple [Whether the line is compliant with the style (before the check),
                   Fixed line,
                   Verbose information].
    """

    # fmt: off
    GPL_LICENSE_LINES = [
        "This program is free software; you can redistribute it and/or modify",
        "it under the terms of the GNU General Public License version 2 as",
        "published by the Free Software Foundation;",
        "This program is distributed in the hope that it will be useful,",
        "but WITHOUT ANY WARRANTY; without even the implied warranty of",
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the",
        "GNU General Public License for more details.",
        "You should have received a copy of the GNU General Public License",
        "along with this program; if not, write to the Free Software",
        "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA",
    ]
    # fmt: on

    SPDX_LICENSE = "SPDX-License-Identifier: GPL-2.0-only"

    is_line_compliant = True
    line_fixed = line
    verbose_infos: List[str] = []

    # Check if the line is a GPL license text
    line_stripped = line.strip()
    line_stripped_no_leading_comments = line_stripped.strip("*#/").strip()

    if line_stripped_no_leading_comments in GPL_LICENSE_LINES:
        is_line_compliant = False
        col_index = 0

        # Replace GPL text with SPDX license.
        # Replace the first line of the GPL text with SPDX.
        # Delete the remaining GPL text lines.
        if line_stripped_no_leading_comments == GPL_LICENSE_LINES[0]:
            line_fixed = line.replace(line_stripped_no_leading_comments, SPDX_LICENSE)
        else:
            line_fixed = ""

        verbose_infos.extend(
            [
                f"{filename}:{line_number + 1}:{col_index}: error: GPL license text detected instead of SPDX license",
                f"    {line_stripped}",
                f"    {'':>{col_index}}^",
            ]
        )

    return (is_line_compliant, line_fixed, verbose_infos)


def check_emacs_line(
    line: str,
    filename: str,
    line_number: int,
) -> Tuple[bool, str, List[str]]:
    """
    Check / fix emacs file style comment in a line.

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

    # Check if line is an emacs file style comment
    line_stripped = line.strip()
    # fmt: off
    emacs_line = re.search(r"c-file-style:|py-indent-offset:", line_stripped)
    # fmt: on

    if emacs_line:
        is_line_compliant = False
        line_fixed = ""
        col_index = emacs_line.start()

        verbose_infos = [
            f"{filename}:{line_number + 1}:{col_index}: error: emacs file style comment detected",
            f"    {line_stripped}",
            f"    {'':{col_index}}^",
        ]

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
            f"    {'':>{len(line_fixed_stripped_expanded)}}^",
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
            f"    {'':>{tab_index}}^",
        ]

    return (is_line_compliant, line_fixed, verbose_infos)


###########################################################
# MAIN
###########################################################
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Check and apply the ns-3 coding style recursively to all files in the given PATHs. "
        "The script checks the formatting of the files using clang-format and"
        " other coding style rules manually (see script arguments). "
        "All checks respect clang-format guards, except trailing whitespace and file encoding,"
        " which are always checked. "
        'When used in "check mode" (default), the script runs all checks in all files. '
        "If it detects non-formatted files, they will be printed and this process exits with a non-zero code. "
        'When used in "fix mode", this script automatically fixes the files and exits with 0 code.'
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
        help='Do not check / fix #include headers from the same module with the "ns3/" prefix (respects clang-format guards)',
    )

    parser.add_argument(
        "--no-include-quotes",
        action="store_true",
        help='Do not check / fix ns-3 #include headers using angle brackets <> rather than quotes "" (respects clang-format guards)',
    )

    parser.add_argument(
        "--no-doxygen-tags",
        action="store_true",
        help="Do not check / fix Doxygen tags using @ rather than \\ (respects clang-format guards)",
    )

    parser.add_argument(
        "--no-licenses",
        action="store_true",
        help="Do not check / fix SPDX licenses rather than GPL text (respects clang-format guards)",
    )

    parser.add_argument(
        "--no-emacs",
        action="store_true",
        help="Do not check / fix emacs file style comments (respects clang-format guards)",
    )

    parser.add_argument(
        "--no-whitespace",
        action="store_true",
        help="Do not check / fix trailing whitespace",
    )

    parser.add_argument(
        "--no-tabs",
        action="store_true",
        help="Do not check / fix tabs (respects clang-format guards)",
    )

    parser.add_argument(
        "--no-formatting",
        action="store_true",
        help="Do not check / fix code formatting (respects clang-format guards)",
    )

    parser.add_argument(
        "--no-encoding",
        action="store_true",
        help=f"Do not check / fix file encoding ({FILE_ENCODING})",
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
            checks_enabled={
                "include_prefixes": not args.no_include_prefixes,
                "include_quotes": not args.no_include_quotes,
                "doxygen_tags": not args.no_doxygen_tags,
                "license": not args.no_licenses,
                "emacs": not args.no_emacs,
                "whitespace": not args.no_whitespace,
                "tabs": not args.no_tabs,
                "formatting": not args.no_formatting,
                "encoding": not args.no_encoding,
            },
            fix=args.fix,
            verbose=args.verbose,
            n_jobs=args.jobs,
        )

    except Exception as ex:
        print("ERROR:", ex)
        sys.exit(1)

    if not all_checks_successful:
        if args.verbose:
            print(
                "",
                "Notes to fix the above formatting issues:",
                '  - To fix the formatting of specific files, run this script with the flag "--fix":',
                "      $ ./utils/check-style-clang-format.py --fix path [path ...]",
                "  - To fix the formatting of all files modified by this branch, run this script in the following way:",
                "      $ git diff --name-only master | xargs ./utils/check-style-clang-format.py --fix",
                sep="\n",
            )

        sys.exit(1)
