@echo off
cd ../Data/Shaders/SPIRV
del *.spv
cd ./src

for /r %%i in (.) do (
	pushd "%%i"
	call "%%i/compile.bat"
	echo "%%i Complete"
	popd
)

pause