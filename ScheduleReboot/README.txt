Sample installer for testing of reboot handling. The installer always returns ERROR_SUCCESS_REBOOT_REQUIRED (3010) to indicate that a reboot is required to complete the install.

Doc: https://learn.microsoft.com/en-us/windows/win32/msi/error-codes


### How check msiexec codes

Run the following commands from a batch script:
```
start /wait msiexec.exe /i ScheduleReboot
echo msiexec exit code: %ERRORLEVEL%
```
