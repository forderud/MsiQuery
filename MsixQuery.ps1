
function PrintPackageMetadata {
    param (
        $package
    )

    $manifest= Get-AppxPackageManifest -Package $package
    $pkg = $manifest.Package
    #Write-Host $pkg.OuterXml

    Write-Host PackageFullName: $app.PackageFullName
    Write-Host Identity: $pkg.Identity.Name
    Write-Host DisplayName: $pkg.Properties.DisplayName
    Write-Host Publisher: $pkg.Properties.PublisherDisplayName
    Write-Host Version: $pkg.Identity.Version
}


# Query installed MSIX apps
$apps = Get-AppXPackage -AllUsers

# Get app metadata
PrintPackageMetadata $apps[0]
