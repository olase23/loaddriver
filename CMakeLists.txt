
add_rc_deps(Skript1.rc)
add_executable(loaddriver loaddriver.c misc.c sapi.c Skript1.rc)
set_module_type(loaddriver win32gui UNICODE)
add_importlibs(loaddriver advapi32 user32 shell32 gdi32 comctl32 comdlg32 msvcrt kernel32)
add_cd_file(TARGET loaddriver DESTINATION reactos/system32 FOR all)
