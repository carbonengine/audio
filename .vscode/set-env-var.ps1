$branchPath = Read-Host 'Enter the CCP_EVE_PERFORCE_BRANCH_PATH value'
[Environment]::SetEnvironmentVariable('CCP_EVE_PERFORCE_BRANCH_PATH', $branchPath, 'User')
Write-Host "Set CCP_EVE_PERFORCE_BRANCH_PATH=$branchPath as user environment variable" -ForegroundColor Green
Write-Host 'You need to fully restart VS Code (close and reopen) for CMake to pick up the new value' -ForegroundColor Yellow
Write-Host 'Reload Window is not sufficient for user environment variables' -ForegroundColor Yellow
