Standard metadata and installation practices involved in application & OS-updates on Windows computers. Other operating systems have similar but different metadata and practices.

## Application packages

Overview of categories of SW installers supported by Windows:
| Category | Installation process | Installer metadata | Detection and uninstallation |
|----------|----------------------|--------------------|------------------------------|
| Executable (or script) | App-specific (command-line arguments and return codes not standardized) | In general not parseable, since the installer is a black-box that Windows doesn't understand | Standard registry location |
| MSI      | Standardized (using `msiexec`) | Contains metadata that can be parsed. The installation steps can also be statically analyzed to asses security risk| MSI APIs or standard registry location |
| MSIX     | Standardized (using APIs) | Contains metadata that can be parsed. The installation steps can also be statically analyzed to asses security risk | `Get-AppxPackage` |

**Package identifier**: `ProductCode` (128bit unique GUID for MSI), `PackageFullName` string (for MSIX) or similar. All SW applications that show up in the Windows control panel for uninstallation does have a unique identifier.

The source of truth for installed EXE and MSI apps are the `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{ProductCode}` registry folders.

## Windows Installer packages (MSI files)
**Format**: MSI (self-described format with metadata). Primarily designed for application SW, but drivers and system configuration can also be MSI-packaged for unified installation.

Common operations:
| Operation | Description |
|-----------|-------------|
| Install | `msiexec.exe /i <appname>.msi /qn` (background installation) |
| Uninstall | Command in `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{ProductCode}\UninstallString` registry key (`msiexec /x "{ProductCode}" /qn` for MSI apps)|
| Upgrade | `msiexec.exe /i <appname>.msi /qn` (will automatically uninstall the old version before installing the new version and perform migration steps) |
| Downgrade | Achieved through _uninstall_ followed by _install_ of the old version. |
| Identify ProductCode | Check MsiQuery for code sample. |
| Check if installed | Check MsiQuery for code sample. |

### MsiQuery tool
Command-line tool for querying MSI files and installed Windows apps

Usage: `MsiQuery.exe [*|<filename.msi>|{ProductCode}|{UpgradeCode}]` where `*` will list all installed products.

The following is listed for each product:
* [**PackageCode**](https://learn.microsoft.com/en-us/windows/win32/msi/package-codes): Unique identifier for a MSI installer file that _might_ contain multiple products.
* [**ProductCode**](https://docs.microsoft.com/en-us/windows/win32/msi/productcode): Unique identifier for a particular product release. Must be changed as part of a [major version upgrade](https://learn.microsoft.com/en-us/windows/win32/msi/major-upgrades) but can be kept unchanged for [small updates](https://learn.microsoft.com/en-us/windows/win32/msi/small-updates)
* [**UpgradeCode**](https://docs.microsoft.com/en-us/windows/win32/msi/using-an-upgradecode) (if present): Product identifier that remain unchanged across major version changes.
* [ProductName](https://docs.microsoft.com/en-us/windows/win32/msi/productname)
* [ProductVersion](https://docs.microsoft.com/en-us/windows/win32/msi/productversion)
* [Manufacturer](https://docs.microsoft.com/en-us/windows/win32/msi/manufacturer)
* Product [features](https://learn.microsoft.com/en-us/windows/win32/msi/windows-installer-features) and feature [installation status](https://learn.microsoft.com/en-us/windows/win32/msi/feature-table)

The following details are also listed:
* [Custom Actions](https://docs.microsoft.com/en-us/windows/win32/msi/custom-actions) that might affect [system state](https://docs.microsoft.com/en-us/windows/win32/msi/changing-the-system-state-using-a-custom-action)
* Path to installed EXE & DLL files (based on [File table](https://docs.microsoft.com/en-us/windows/win32/msi/file-table) query with [MsiGetComponentPath](https://docs.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msigetcomponentpathw) lookup) (only for installed apps)
* Added [registry entries](https://docs.microsoft.com/en-us/windows/win32/msi/registry-table) (can also be created through custom actions)

### Inventory scan
The [DetectInstalledApps.ps1](./DetectInstalledApps.ps1) script can be used to detect installed EXE and MSI applications through a [registry scan](https://learn.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key).

It's possible to use the `Get-WmiObject Win32_Product` command to list all MSI-installed apps with ProductCode on the system. However, this listing will not include EXE-installed apps without a ProductCode, like 7-zip and Notepad++. It's therefore insufficient if support for non-MSI installers is also required.

### ParseMSI script
The [ParseMSI.ps1](./ParseMSI.ps1) script can be used to detect installed MSI applications through the [WindowsInstaller](https://learn.microsoft.com/en-us/windows/win32/msi/installer-object) COM interfaces.

### ScheduleReboot Installer
Sample installer for testing of reboot handling. The installer always returns ERROR_SUCCESS_REBOOT_REQUIRED (3010) to indicate that a reboot is required to complete the install.

## MSIX packages
Microsoft is recommending to migrate to the newer [MSIX](https://learn.microsoft.com/en-us/windows/msix/overview) installer format. However, it's more restrictive with limitations on inter-app communication. Also, tooling support is lagging behind - at least for Qt (see [How to package a Win32 desktop app in MSIX?](https://bugreports.qt.io/browse/QTBUG-97088)). Adoption can therefore be challenging.

### MsixQuery tool
The MsixQuery tool can be used to detect installed MSIX apps.


## Microsoft Update packages (MSU files)
MSU files is a separate format used for distribution of Windows OS updates. They are technically archives that contain cabinet (CAB) files with updates together with XML and TXT metadata.

[Description of the Windows Update Standalone Installer in Windows](https://support.microsoft.com/en-us/topic/description-of-the-windows-update-standalone-installer-in-windows-799ba3df-ec7e-b05e-ee13-1cdae8f23b19).

Windows OS updates and can be downloaded from https://www.catalog.update.microsoft.com/

Common operations:
| Operation | Description |
|-----------|-------------|
| Install/upgrade | `wusa.exe <filename>.msu /quiet` |
| Uninstall/downgrade | `wusa.exe /uninstall /kb:<KB number> /quiet` |
| Identify KB number | Unpack MSU archive and parse XML or TXT files inside (TODO: find code sample) |
| Check if installed | see link below |

DOC: https://learn.microsoft.com/en-us/mem/intune/apps/apps-win32-deploy-update-package


### Sample scripts for OS version
Get OS version:
```
PS > Get-WmiObject Win32_OperatingSystem

SystemDirectory : C:\Windows\system32
Organization    : <org-name>
BuildNumber     : 19044
RegisteredUser  : <user>
SerialNumber    : <serial>
Version         : 10.0.19044
```

Get list of installed hotfixes (KB's):
```
PS > Get-HotFix

Source        Description      HotFixID      InstalledBy          InstalledOn
------        -----------      --------      -----------          -----------
MYMACHINE     Update           KB5027122     NT AUTHORITY\SYSTEM  2023-06-23 12:00:00 AM
MYMACHINE     Update           KB5003791                          2021-10-06 12:00:00 AM
MYMACHINE     Security Update  KB5012170     NT AUTHORITY\SYSTEM  2022-12-14 12:00:00 AM
MYMACHINE     Security Update  KB5028166     NT AUTHORITY\SYSTEM  2023-07-28 12:00:00 AM
MYMACHINE     Security Update  KB5014032     NT AUTHORITY\SYSTEM  2022-11-11 12:00:00 AM
MYMACHINE     Update           KB5016705     NT AUTHORITY\SYSTEM  2022-11-11 12:00:00 AM
MYMACHINE     Update           KB5018506     NT AUTHORITY\SYSTEM  2022-11-21 12:00:00 AM
MYMACHINE     Update           KB5020372     NT AUTHORITY\SYSTEM  2022-12-21 12:00:00 AM
MYMACHINE     Update           KB5022924     NT AUTHORITY\SYSTEM  2023-03-23 12:00:00 AM
MYMACHINE     Update           KB5023794     NT AUTHORITY\SYSTEM  2023-04-21 12:00:00 AM
MYMACHINE     Update           KB5025315     NT AUTHORITY\SYSTEM  2023-05-18 12:00:00 AM
MYMACHINE     Update           KB5026879     NT AUTHORITY\SYSTEM  2023-06-23 12:00:00 AM
MYMACHINE     Update           KB5028318     NT AUTHORITY\SYSTEM  2023-07-20 12:00:00 AM
MYMACHINE     Security Update  KB5005699                          2021-10-06 12:00:00 AM
```


## Return codes
Installers are using the following return-codes to signal installation result:

| Code | Interpretation |
|------|----------------|
| 0    | Success (installation completed successfully) |
| 1707 | Success (installation completed successfully) |
| 3010 | Soft reboot (restart is required to complete the install) |
| 1641 | Hard reboot (installer have initiated a restart) |
| 1618 | Retry (another installation is already in progress) |
|      | Other values are treated as failure |

The above codes are used by InTune and probably other MDM solutions to detect installation success/failure and retry or restart if needed afterwards.

The codes are documented on [MsiExec.exe and InstMsi.exe error messages](https://learn.microsoft.com/en-us/windows/win32/msi/error-codes).


## References
Related tools:
* [Orca](https://docs.microsoft.com/en-us/windows/win32/msi/orca-exe) MSI viewer: Included with Windows 10 SDK. By default installed to `%ProgramFiles(x86)%\Windows Kits\10\bin\<version>\x86\Orca-x86_en-us.msi`
* [msitools](https://gitlab.gnome.org/GNOME/msitools) for building & inspecing MSI files on Linux/Mac.

Documentation:
* [Windows Installer](https://docs.microsoft.com/en-us/windows/win32/msi/windows-installer-portal)
