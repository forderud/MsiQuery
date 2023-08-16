using Windows.ApplicationModel;
using Windows.Management.Deployment;

void GetManagementPackages()
{
    var mgr = new PackageManager();
    var pkgs = mgr.FindPackagesForUser(""); // current user
    //var pkgs = mgr.FindPackages(); // require admin privileges

    // filter out system packages (not shown in "Apps & features")
    pkgs = pkgs.Where(elm => elm.SignatureKind != PackageSignatureKind.System).ToList();
    // filter out frameworks (not shown in "Apps & features")
    pkgs = pkgs.Where(elm => elm.IsFramework == false).ToList();

    Console.WriteLine($"Found {pkgs.Count()} packages.");

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
