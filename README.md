# MsiQuery
C++ tool for querying MSI files and installed Windows apps

Usage: `MsiQuery.exe [*|<filename.msi>|{ProductCode}|{UpgradeCode}]` where `*` will list all installed products.

The tool will list the following:
* [ProductCode](https://docs.microsoft.com/en-us/windows/win32/msi/productcode): Unique identifier for a particular product release. Must vary for different versions and languages.
* [UpgradeCode](https://docs.microsoft.com/en-us/windows/win32/msi/using-an-upgradecode) (if present): Product identifier that remain unchanged across  version changes.
* [ProductName](https://docs.microsoft.com/en-us/windows/win32/msi/productname)
* [ProductVersion](https://docs.microsoft.com/en-us/windows/win32/msi/productversion)
* [Manufacturer](https://docs.microsoft.com/en-us/windows/win32/msi/manufacturer)

In addition, the following is listed for installed apps:
* [Custom Actions](https://docs.microsoft.com/en-us/windows/win32/msi/custom-actions) that might affect [system state](https://docs.microsoft.com/en-us/windows/win32/msi/changing-the-system-state-using-a-custom-action)
* Path to installed EXE files (based on [File table](https://docs.microsoft.com/en-us/windows/win32/msi/file-table) query with [MsiGetComponentPath](https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msigetcomponentpathw) lookup)
* Added [registry entries](https://docs.microsoft.com/en-us/windows/win32/msi/registry-table)


## Suggested SW
* **[Orca](https://docs.microsoft.com/en-us/windows/win32/msi/orca-exe)** MSI viewer: Included with Windows 10 SDK. By default installed to `%ProgramFiles(x86)%\Windows Kits\10\bin\<version>\x86\Orca-x86_en-us.msi`

## References
* [Windows Installer](https://docs.microsoft.com/en-us/windows/win32/msi/windows-installer-portal)
