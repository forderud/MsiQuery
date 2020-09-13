# MsiQuery
C++ tool for querying MSI files and installed Windows apps

Usage:`MsiQuery.exe [<filename.msi>|{ProductCode}|{UpgradeCode}]`

The tool will list the following:
* ProductCode
* UpgradeCode (if present)
* ProductName
* Manufacturer

In addition, the following is listed for installed apps:
* Custom actions that might affect system state
* Path to installed EXE files
* Registry entries


## Suggested SW
* **Orca** MSI viewer: Included with Windows 10 SDK. By default installed to `%ProgramFiles(x86)%\Windows Kits\10\bin\<version>\x86\Orca-x86_en-us.msi`

## References
* [Windows Installer](https://docs.microsoft.com/en-us/windows/win32/msi/windows-installer-portal)
