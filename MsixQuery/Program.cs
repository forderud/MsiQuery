using Windows.ApplicationModel;
using Windows.Management.Deployment;

void GetManagementPackages()
{
    var mgr = new PackageManager();
    var pkgs = mgr.FindPackagesForUser("");

    // filter out system packages
    pkgs = pkgs.Where(elm => elm.SignatureKind != PackageSignatureKind.System).ToList();

    foreach (Package pkg in pkgs)
    {
        Console.WriteLine("Package info:");
        Console.WriteLine("  DisplayName: "+pkg.DisplayName);
        Console.WriteLine("  Id: " + pkg.Id.FullName);
        PackageVersion ver = pkg.Id.Version;
        Console.WriteLine($"  Version: {ver.Major}.{ver.Minor}.{ver.Build}.{ver.Revision}");
        Console.WriteLine("  InstalledDate: " + pkg.InstalledDate);
    }
}

GetManagementPackages();
