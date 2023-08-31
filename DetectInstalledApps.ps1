# Detect MSI- and EXE-based installed apps
# Will return approx. the same results as what's listed in "Apps & features" in Windows Settings.
# However, MSIX-based apps are not detected.
$apps = @()
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*" # 32bit apps
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*" # 64bit apps

# Filter out apps without name and system components (hidden from "Apps & features")
$apps = $apps | Where-Object {
    $_.DisplayName -and ($_.SystemComponent -ne 1)
}

# List installed apps
foreach($app in $apps) {
    # check if app has UpgradeCode (slow call that can be optimized)
    $UpgradeCode = Get-WmiObject -Query "SELECT Value FROM Win32_Property WHERE Property='UpgradeCode' AND ProductCode='$($app.PSChildName)'"
    
    Write-Host ProductCode: $app.PSChildName   # REQUIRED primary identifier - guid or string (might vary between versions)
    Write-Host UpgradeCode: $UpgradeCode.Value # OPTIONAL stable identifier that does _not_ change between versions
    Write-Host Name: $app.DisplayName          # REQUIRED
    Write-Host Version: $app.DisplayVersion    # REQUIRED
    Write-Host Publisher: $app.Publisher       # OPTIONAL
    Write-Host InstallDate: $app.InstallDate   # OPTIONAL
    Write-Host ""
}
