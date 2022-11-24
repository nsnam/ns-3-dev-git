set(3RD_PARTY_FIND_PROGRAM_HINTS
    # find_program HINTS for Doxygen
    # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/FindDoxygen.cmake
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\doxygen_is1;Inno Setup: App Path]/bin"
    /Applications/Doxygen.app/Contents/Resources
    /Applications/Doxygen.app/Contents/MacOS
    /Applications/Utilities/Doxygen.app/Contents/Resources
    /Applications/Utilities/Doxygen.app/Contents/MacOS
    # find_program HINTS for Dia
    # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/FindDoxygen.cmake
    "$ENV{ProgramFiles}/Dia"
    "$ENV{ProgramFiles\(x86\)}/Dia"
    # find_program HINTS for Graphviz
    # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/FindDoxygen.cmake
    "$ENV{ProgramFiles}/ATT/Graphviz/bin"
    "C:/Program Files/ATT/Graphviz/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ATT\\Graphviz;InstallPath]/bin"
    /Applications/Graphviz.app/Contents/MacOS
    /Applications/Utilities/Graphviz.app/Contents/MacOS
)
