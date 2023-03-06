@echo off

if "%MSBUILD_TOOLS_PATH%"=="" (
    echo "Environment variable MSBUILD_TOOLS_PATH must contain a path to a directory that contains MSBuild.exe."
    exit
)

if NOT EXIST "%MSBUILD_TOOLS_PATH%\MSBuild.exe" (
    echo "Environment variable MSBUILD_TOOLS_PATH [%MSBUILD_TOOLS_PATH%] directory does not contain MSBuild.exe."
    exit
)

"%MSBUILD_TOOLS_PATH%\MSBuild.exe" %*
