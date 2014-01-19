del *.dll
del __history\*.* /Q
rd __history /Q
move Win32\Release\*.dll
del Win32\Release\*.* /Q
rd Win32\Release /Q
rd Win32 /Q
upx -9 --lzma *.dll