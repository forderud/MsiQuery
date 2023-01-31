# stop script on first error
$ErrorActionPreference = "Stop"

$filename = $args[0] # absolute MSI path (not relative)

# DOC: https://learn.microsoft.com/en-us/windows/win32/msi/installer-object
$windowsInstaller = New-Object -ComObject WindowsInstaller.Installer

Write-Output "Opening $filename"
$db = $windowsInstaller.OpenDatabase($filename, 0) # 0=read-only

$view = $db.OpenView("select * from File")
$view.Execute()

write-host "File table content:" -ForegroundColor Green
for () {
    $record = $view.Fetch()
    if ($record -eq $null) {
        break
    }

    write-host "Row:" -ForegroundColor Green
    for($i = 1; $i -le $record.FieldCount(); $i++){
        Write-Output $record.StringData($i)
    }
}

$view.Close()
