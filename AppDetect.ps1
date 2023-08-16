$apps = @()
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*" # 32bit apps
$apps += Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*" # 64bit apps

# Filter out apps without name and system components (hidden from control panel)
$apps = $apps | Where-Object {
    $_.DisplayName -and $_.SystemComponent -ne 1
}

# Write information about the first app
Write-Host $apps[0]
