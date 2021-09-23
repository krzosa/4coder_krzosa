@echo off
pushd %~dp0
call ..\custom\bin\buildsuper_x64-win.bat kr_init.cpp %1
xcopy /Y /s theme-krzosa.4coder ..\themes\theme-krzosa.4coder
popd