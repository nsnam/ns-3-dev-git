#!/usr/bin/env bash

# Diagnose the developer environment

# How to use the tool
# ```
# $ ./utils/diag-macos.sh
# ```

# How to extend the tool
# Append more useful commands to `COMMANDS` array

COMMANDS=(
    "sw_vers"
    "uname -a"
    "arch"
    "which clang"
    "which clang++"
    "clang --version"
    "clang++ --version"
    "collect_libcpp_version"
    "which cmake"
    "cmake --version"
    "xcodebuild -version"
    "xcode-select -version"
    "python3 --version"
    "brew --version"
    "brew leaves | xargs"
    "system_profiler -detailLevel mini SPSoftwareDataType SPHardwareDataType"
    "git --no-pager log --abbrev-commit --date=short  -n1 --pretty=format:'%h %s %an %cd'; echo "
    "which brew && brew list --versions"
    "which pip3 && pip3 list --version"
)

function collect_libcpp_version() {
    grep " _LIBCPP_VERSION " $(xcrun --show-sdk-path)/usr/include/c++/v1/__config | grep -v "LLVM" | cut -f5 -d' '
}

function collect() {
    printf "\n"
    printf -- '-%0.s' {1..72}

    for ((i = 0; i < ${#COMMANDS[@]}; i++)); do
        echo ""
        echo "## ${COMMANDS[$i]}"
        eval "${COMMANDS[$i]}"
    done

    printf "\n"
    printf -- '-%0.s' {1..72}
}

collect
