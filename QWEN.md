This project is hosted on the remote MorphOS computer at /Work/Development/git/netshell.

The MorphOS SDK is located locally at /home/sblo/xtra/morphos/Development/ but its executables cannot be run locally.

When testing the remote connection, use: "echo ls | nc 192.168.1.136 2323"

Note: When building on MorphOS, the compiler flags are different than in cross-compilation.
- Instead of "-mc68020 -m68881", MorphOS GCC uses "-noixemul" for native applications
- MorphOS SDK path is typically "SDK:" rather than Unix-style paths
- MorphOS GCC doesn't recognize the "-mc68020" and "-m68881" flags

MUI Development Notes:
- Use basic MUI macros like ApplicationObject, WindowObject, VGroup, HGroup
- Use SimpleButton() for basic buttons
- Use StringObject for string input
- Use TextObject for text display
- Use ScrollgroupObject for scrollable content
- Include basic MUI headers like <libraries/mui.h>
- Don't use advanced MUI object constants that might not be available

New Requirements:
- Enhanced netshell protocol with magic string to enable binary extensions
- File transfer capabilities
- Support for ncurses applications
- New netshell-client with CLI features
- Prioritize MUI application over gui_version (which is broken)
- Target is ppc-morphos with -noixemul flag (not ixemul layer)