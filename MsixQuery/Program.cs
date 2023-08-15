using Windows.ApplicationModel;
using Windows.Management.Deployment;

void GetManagementPackages()
{
    var mgr = new PackageManager();
    var pkgs = mgr.FindPackagesForUser("");

    foreach (Package pkg in pkgs)
    {
        Console.WriteLine("Package info:");
        Console.WriteLine("  DisplayName: "+pkg.DisplayName);
        Console.WriteLine("  Id: " + pkg.Id.FullName);
        var ver = pkg.Id.Version;
        Console.WriteLine("  Version: " + ver.Major+"."+ver.Minor+"."+ver.Revision+"."+ver.Build);
    }
}

GetManagementPackages();
