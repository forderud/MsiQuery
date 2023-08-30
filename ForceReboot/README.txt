Sample installer for testing of reboot handling. The installer always returns ERROR_INSTALL_SUSPEND (1604) to indicate that the install is incomplete.

Doc: https://learn.microsoft.com/en-us/windows/win32/msi/error-codes


### How check msiexec codes

Run the following commands from a batch script:
```
start /wait msiexec.exe /i ForceReboot.msi
echo msiexec exit code: %ERRORLEVEL%
```
