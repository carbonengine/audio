@echo off
if "%~1"=="" (
    echo Usage: launch-vscode-with-branch.bat "path\to\branch"
    echo Example: launch-vscode-with-branch.bat "k:\p4\eve-frontier\branches\development\MAINLINE"
    exit /b 1
)

set "BRANCH_PATH=%~1"
echo Setting CCP_EVE_PERFORCE_BRANCH_PATH to: %BRANCH_PATH%

:: Set the environment variable for this session
set "CCP_EVE_PERFORCE_BRANCH_PATH=%BRANCH_PATH%"

:: Also set it permanently as user environment variable
powershell -Command "[Environment]::SetEnvironmentVariable('CCP_EVE_PERFORCE_BRANCH_PATH', '%BRANCH_PATH%', 'User')"

echo Environment variable set successfully
echo Launching VS Code with the new environment...

:: Launch VS Code with the current directory and environment
code .

echo VS Code launched with CCP_EVE_PERFORCE_BRANCH_PATH=%BRANCH_PATH%
