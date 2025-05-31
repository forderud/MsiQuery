using System.Management;

if (!Environment.IsPrivilegedProcess)
{
    Console.WriteLine("ERROR: Admin privileges required.");
    Environment.Exit(-1);
}

// Doc: https://learn.microsoft.com/en-us/windows/win32/wmisdk/win32-installedwin32program
ManagementObjectSearcher mos = new ManagementObjectSearcher("SELECT * FROM Win32_InstalledWin32Program");
foreach (ManagementObject mo in mos.Get())
{
    Console.WriteLine("Program: " + mo["Name"] + ":");
    Console.WriteLine("  Vendor: " + mo["Vendor"]);
    Console.WriteLine("  Version: " + mo["Version"]);
    Console.WriteLine("  Language: " + mo["Language"]);
    Console.WriteLine("  ProgramId: " + mo["ProgramId"]);
    Console.WriteLine("  MsiPackageCode: " + mo["MsiPackageCode"]);
    Console.WriteLine("  MsiProductCode: " + mo["MsiProductCode"]);
}
