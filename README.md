# MsiQuery
C++ tool for querying MSI files and installed Windows apps

Usage: `MsiQuery.exe [<filename.msi>|{ProductCode}|{UpgradeCode}]`

The tool will list the following:
* [ProductCode](https://docs.microsoft.com/en-us/windows/win32/msi/productcode)
* [UpgradeCode](https://docs.microsoft.com/en-us/windows/win32/msi/upgradecode) (if present)
* [ProductName](https://docs.microsoft.com/en-us/windows/win32/msi/productname)
* [ProductVersion](https://docs.microsoft.com/en-us/windows/win32/msi/productversion)
* [Manufacturer](https://docs.microsoft.com/en-us/windows/win32/msi/manufacturer)

In addition, the following is listed for installed apps:
* [Custom Actions](https://docs.microsoft.com/en-us/windows/win32/msi/custom-actions) that might affect system state
* Path to installed EXE files
* Registry entries


## Suggested SW
* **Orca** MSI viewer: Included with Windows 10 SDK. By default installed to `%ProgramFiles(x86)%\Windows Kits\10\bin\<version>\x86\Orca-x86_en-us.msi`

## References
* [Windows Installer](https://docs.microsoft.com/en-us/windows/win32/msi/windows-installer-portal)
