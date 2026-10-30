@echo off
setlocal

set "ROOT=%~dp0"
set "DEVENV=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.exe"

if not exist "%DEVENV%" (
    echo Could not find Visual Studio Community at:
    echo %DEVENV%
    echo.
    echo Open this solution manually instead:
    echo %ROOT%Dx12Cpp26Pipeline.sln
    pause
    exit /b 1
)

start "" "%DEVENV%" "%ROOT%Dx12Cpp26Pipeline.sln"
