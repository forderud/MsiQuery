## Windows Installer packages (MSI files)

### MsiQuery tool
Command-line tool for querying MSI files and installed Windows apps

Usage: `MsiQuery.exe [*|<filename.msi>|{ProductCode}|{UpgradeCode}]` where `*` will list all installed products.

The following is listed for each product:
* [ProductCode](https://docs.microsoft.com/en-us/windows/win32/msi/productcode): Unique identifier for a particular product release. Must be changed as part of a [major version upgrade](https://learn.microsoft.com/en-us/windows/win32/msi/major-upgrades) but can be kept unchanged for [small updates](https://learn.microsoft.com/en-us/windows/win32/msi/small-updates)
* [UpgradeCode](https://docs.microsoft.com/en-us/windows/win32/msi/using-an-upgradecode) (if present): Product identifier that remain unchanged across major version changes.
* [ProductName](https://docs.microsoft.com/en-us/windows/win32/msi/productname)
* [ProductVersion](https://docs.microsoft.com/en-us/windows/win32/msi/productversion)
* [Manufacturer](https://docs.microsoft.com/en-us/windows/win32/msi/manufacturer)
* Product [features](https://learn.microsoft.com/en-us/windows/win32/msi/windows-installer-features) and feature [installation status](https://learn.microsoft.com/en-us/windows/win32/msi/feature-table)

The following details are also listed:
* [Custom Actions](https://docs.microsoft.com/en-us/windows/win32/msi/custom-actions) that might affect [system state](https://docs.microsoft.com/en-us/windows/win32/msi/changing-the-system-state-using-a-custom-action)
* Path to installed EXE & DLL files (based on [File table](https://docs.microsoft.com/en-us/windows/win32/msi/file-table) query with [MsiGetComponentPath](https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msigetcomponentpathw) lookup) (only for installed apps)
* Added [registry entries](https://docs.microsoft.com/en-us/windows/win32/msi/registry-table) (can also be created through custom actions)

### AppDetect script
The [AppDetect.ps1](./AppDetect.ps1) script can be used to detect installed EXE and MSI applications through a [registry scan](https://learn.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key).

## ParseMSI script
The [ParseMSI.ps1](./ParseMSI.ps1) script can be used to detect installed MSI applications through the [WindowsInstaller](https://learn.microsoft.com/en-us/windows/win32/msi/installer-object) COM interfaces.

## Related tools
* [Orca](https://docs.microsoft.com/en-us/windows/win32/msi/orca-exe) MSI viewer: Included with Windows 10 SDK. By default installed to `%ProgramFiles(x86)%\Windows Kits\10\bin\<version>\x86\Orca-x86_en-us.msi`
* [msitools](https://gitlab.gnome.org/GNOME/msitools) for building & inspecing MSI files on Linux/Mac.

## References
* [Windows Installer](https://docs.microsoft.com/en-us/windows/win32/msi/windows-installer-portal)

## MicroSoft Update packages (MSU files)
MSU files is a separate format used for distribution of Windows OS updates. They are technically archives that contain cabinet (CAB) files with updates together with XML and TXT metadata.

[Description of the Windows Update Standalone Installer in Windows](https://support.microsoft.com/en-us/topic/description-of-the-windows-update-standalone-installer-in-windows-799ba3df-ec7e-b05e-ee13-1cdae8f23b19).
