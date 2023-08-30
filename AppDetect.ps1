$apps = @()
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*" # 32bit apps
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*" # 64bit apps

# Filter out apps without name and system components (hidden from control panel)
$apps = $apps | Where-Object {
    $_.DisplayName -and ($_.SystemComponent -ne 1)
}

# List MSI- and EXE-installed apps
foreach($app in $apps)
{
    # check if app has UpgradeCode
    $query = "SELECT Value FROM Win32_Property WHERE Property='UpgradeCode' AND ProductCode='"+$app.PSChildName+"'"
    $UpgradeCode = Get-WmiObject -Query $query
    
    Write-Host ""
    Write-Host ID: $app.PSChildName
    Write-Host UpgradeCode: $UpgradeCode.Value
    Write-Host DisplayName: $app.DisplayName
    Write-Host DisplayVersion: $app.DisplayVersion
    Write-Host Publisher: $app.Publisher
    Write-Host InstallDate: $app.InstallDate
}
