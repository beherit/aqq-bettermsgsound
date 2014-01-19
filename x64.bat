del *.x64
del __history\*.* /Q
rd __history /Q
rename Win64\Release\*.dll *.x64
move Win64\Release\*.x64
del Win64\Release\*.* /Q
rd Win64\Release /Q
rd Win64 /Q
upx -9 --lzma *.x64