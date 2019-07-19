setlocal enabledelayedexpansion
for /r %%i in (.\src\*.frag, .\src\*.vert) do (
	set b=%%~xi
	set b=!b:.=!
	%VULKAN_SDK%/Bin/glslangValidator.exe -V %%~ni_!b!.spv %%i
)
pause
