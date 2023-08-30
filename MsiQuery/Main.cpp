#include "MsiQuery.hpp"
#include "MsiUtil.hpp"
#include <fcntl.h>
#include <io.h>
#include <cctype>
#include <iostream>

#pragma comment(lib, "Msi.lib")


static std::wstring ToAbsolutePath(std::wstring path) {
    DWORD len = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    std::wstring buffer(len-1, L'\0'); // subtract null-termination
    len = GetFullPathNameW(path.c_str(), len, const_cast<wchar_t*>(buffer.data()), nullptr);
    return buffer;
}

static std::wstring ToString(INSTALLSTATE state) {
    switch (state) {
    case INSTALLSTATE_NOTUSED: return L"NOTUSED";
    case INSTALLSTATE_BADCONFIG: return L"BADCONFIG";
    case INSTALLSTATE_INCOMPLETE: return L"INCOMPLETE";
    case INSTALLSTATE_SOURCEABSENT: return L"SOURCEABSENT";
    case INSTALLSTATE_MOREDATA: return L"MOREDATA";
    case INSTALLSTATE_INVALIDARG: return L"INVALIDARG";
    case INSTALLSTATE_UNKNOWN: return L"UNKNOWN";
    case INSTALLSTATE_BROKEN: return L"BROKEN";
    case INSTALLSTATE_ADVERTISED: return L"ADVERTISED";
    //case INSTALLSTATE_REMOVED: return L"REMOVED";
    case INSTALLSTATE_ABSENT: return L"ABSENT";
    case INSTALLSTATE_LOCAL: return L"LOCAL"; // installed on local drive
    case INSTALLSTATE_SOURCE: return L"SOURCE";
    case INSTALLSTATE_DEFAULT: return L"DEFAULT";
    default:
        throw std::runtime_error("unknown INSTALLSTATE");
    }
}

void AnalyzeMsiFile(std::wstring msi_file, std::wstring * product_code) {
    MsiQuery query(msi_file);

    {
        std::wcout << L"Features:\n";
        std::vector<FeatureEntry> features = query.QueryFeature();
        for (const FeatureEntry& feature : features) {
            std::wstring install_state;
            if (product_code) {
                INSTALLSTATE state = MsiQueryFeatureStateW(product_code->c_str(), feature.Feature.c_str());
                install_state = L", INSTALLSTATE=" + ToString(state);
            }

            std::wcout << L"  " << feature.ToString() << install_state << L'\n';
        }
        std::wcout << L'\n';
    }

    FileTable files = query.QueryFile();

    {
        std::wcout << L"Custom actions: (might affect system state)\n";
        //REF: https://docs.microsoft.com/en-us/windows/win32/msi/changing-the-system-state-using-a-custom-action

        auto custom_actions = query.QueryCustomAction();
        bool has_custom_action = false;
        for (const CustomActionEntry& ca : custom_actions) {
            if (!ca.Type.NoImpersonate && !ca.Type.Deferred)
                continue; // discard custom actions that neither run as admin nor are deferred

            has_custom_action = true;
            FileTable::Entry file = files.Lookup(ca.Source, false); // might fail
            std::wcout << L"  " << ca.Action << L": "  << ca.Type.ToString() << L' ' << file.LongFileName() << L' ' << ca.Target << L'\n';
        }
        if (!has_custom_action)
            std::wcout << L"  <none>\n";

        std::wcout << L"\n";
    }

    ComponentTable components = query.QueryComponent();

    {
        std::wcout << L"Installed binaries: (skipping other file types)\n";

        // convert string to lowercase
        auto to_lowercase = [](std::wstring str) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](wchar_t c) { return std::tolower(c); });
            return str;
        };

        DirectoryTable directories = query.QueryDirectory();

        std::vector<std::wstring> exe_files, dll_files;
        for (const FileTable::Entry& file : files.Entries()) {
            ComponentTable::Entry component = components.Lookup(file.Component_);

            std::wstring path;
            if (product_code)
                path = GetComponentPath(*product_code, component.ComponentId); // get actually installed paths
            else
                path = directories.Lookup(component.Directory_) + L'\\' + file.LongFileName();

            if (to_lowercase(path).find(L".exe") != path.npos)
                exe_files.push_back(path);
            else if (to_lowercase(path).find(L".dll") != path.npos)
                dll_files.push_back(path);
            else if (to_lowercase(path).find(L".pyd") != path.npos)
                dll_files.push_back(path); // python extension (renamed DLL)
        }

        // write EXEs first, then DLLs
        for (auto file : exe_files)
            std::wcout << L"  " << file << L'\n';
        for (auto file : dll_files)
            std::wcout << L"  " << file << L'\n';

        std::wcout << L"\n";
    }

    {
        std::wcout << L"Registry entries:\n";

        auto reg_entries = query.QueryRegistry();
        for (const RegEntry& reg : reg_entries) {
            std::wstring path;
            if (product_code && false) // disabled for now since it always return "E:\"
                path = GetComponentPath(*product_code, components.Lookup(reg.Component_).ComponentId);
            else
                path = reg.RootStr() + L'\\' + reg.Key +  L'\\' + reg.Name + L'=' + reg.Value;

            std::wcout << L"  " << path << L'\n';
        }
        if (reg_entries.empty())
            std::wcout << L"  <none> (might still be created through custom actions)\n";

        std::wcout << L"\n";
    }
}

bool IsGUID (const std::wstring & str) {
    if (str.length() == 38) {
        if ((str[0] == L'{') && (str.back() == L'}'))
            return true;
    }
    return false;
}


std::wstring ParseMSIOrProductCode (std::wstring file_or_product) {
    PMSIHANDLE msi;
    if (IsGUID(file_or_product)) {
        // input is a ProductCode
        //std::wcout << L"Attempting to open ProductCode " << file_or_product << L"...\n";
        UINT ret = MsiOpenProductW(file_or_product.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    } else {
        // input is a MSI filename
        file_or_product = ToAbsolutePath(file_or_product);

        std::wcout << L"Attempting to open file " << file_or_product << L"...\n";
        UINT ret = MsiOpenPackageW(file_or_product.c_str(), &msi);
        if (ret == ERROR_FILE_NOT_FOUND)
            throw std::runtime_error("MsiOpenPackage file not found");
        else if (ret == ERROR_INSTALL_PACKAGE_OPEN_FAILED)
            throw std::runtime_error("MsiOpenPackage unable to open file (might be locked)");
        else if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage unspecified error");
    }

    std::wstring product_code;
    {
        // read properties
        product_code = GetProductProperty(msi, L"ProductCode"); // REQUIRED
        std::wstring upgrade_code = GetProductProperty(msi, L"UpgradeCode", false); // optional
        std::wstring product_name = GetProductProperty(msi, L"ProductName"); // REQUIRED
        std::wstring product_ver = GetProductProperty(msi, L"ProductVersion"); // REQUIRED
        std::wstring manufacturer = GetProductProperty(msi, L"Manufacturer"); // REQUIRED
        std::wcout << L"MSI properties:\n";
        std::wcout << L"  ProductCode: " << product_code << L"\n";
        std::wcout << L"  UpgradeCode: " << upgrade_code << L"\n";
        std::wcout << L"  ProductName: " << product_name << L"\n";
        std::wcout << L"  ProductVersion: " << product_ver << L"\n";
        std::wcout << L"  Manufacturer: " << manufacturer << L"\n";
    }

    return product_code;
}


std::wstring ParseInstalledApp (std::wstring product_code) {
    // check if app is installed
    std::wstring msi_cache_file = GetProductInfo(product_code, INSTALLPROPERTY_LOCALPACKAGE); // Local cached package
    if (msi_cache_file.empty())
        return L""; // early return if not installed

    {
        std::wstring inst_loc = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLLOCATION); // seem to be empty
        //std::wstring inst_folder = GetProductInfo(product_code, L"INSTALLFOLDER"); // not found
        std::wstring prod_id = GetProductInfo(product_code, INSTALLPROPERTY_PRODUCTID); // seem to be empty

        std::wstring inst_name = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLEDPRODUCTNAME); // seem identical to ProductName
        std::wstring publisher = GetProductInfo(product_code, INSTALLPROPERTY_PUBLISHER); // seem identical to Manufacturer
        std::wstring version = GetProductInfo(product_code, INSTALLPROPERTY_VERSIONSTRING); // seem identical to ProductVersion
        std::wstring inst_date = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLDATE); // "YYYYMMDD" format

        std::wcout << L"Installed properties:\n";
        std::wcout << L"  InstalledProductName: " << inst_name << L"\n";
        std::wcout << L"  Version: " << version << L"\n";
        std::wcout << L"  Publisher: " << publisher << L"\n";
        std::wcout << L"  InstallDate: " << inst_date << L"\n";
        std::wcout << L"  MSI cache: " << msi_cache_file << L"\n";
    }

    return msi_cache_file;
}


void EnumerateInstalledProducts() {
    std::wcout << L"List of installed products:\n";

    for (DWORD idx = 0;; ++idx) {
        std::wstring product_code(38, L'\0'); // fixed length
        UINT ret = MsiEnumProductsW(idx, const_cast<wchar_t*>(product_code.data()));
        if (ret == ERROR_NO_MORE_ITEMS)
            break;
        assert(ret == ERROR_SUCCESS);

        std::wcout << L"\n";
        std::wcout << idx << L": ProductCode: " << product_code << L'\n';
#ifndef EXTENDED_INFO
        std::wstring msi_cache_file = ParseInstalledApp(product_code);
#else
        try {
            ParseMSIOrProductCode(product_code); // slower, but also gives UpgradeCode
            ParseInstalledApp(product_code);
        } catch (const std::exception & err) {
            std::wcout << L"  ERROR: " << ToUnicode(err.what()) << L'\n';
        }
#endif
    }
}


int wmain (int argc, wchar_t *argv[]) {
    // enable unicode characters in console output
    _setmode(_fileno(stdout), _O_U16TEXT);

    if (argc < 2) {
        std::wcout << L"Usage: " << argv[0] << L" [*|<filename.msi>|{ProductCode}|{UpgradeCode}]\n";
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // hide MSI installer UI
    MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);

    try {
        std::wstring argument = argv[1];
        if (argument == L"*") {
            EnumerateInstalledProducts();
        } else {
            // check if input is UpgradeCode
            auto product_code = GetFirstProductCode(argument);
            if (!product_code.empty()) {
                std::wcout << L"UpgradeCode " << argument << L" is associated with ProductCode " << product_code << L"\n";
                argument = product_code;
            }

            product_code = ParseMSIOrProductCode(argument);
            std::wstring msi_cache_file = ParseInstalledApp(product_code);
            std::wcout << L"\n";
            if (msi_cache_file.size() > 0) {
                std::wcout << L"Application is already installed. Will also analyze installed files.\n\n";

                // parse installed MSI
                AnalyzeMsiFile(msi_cache_file, &product_code);
            } else {
                std::wcout << L"Application is NOT installed. Will perform offline analysis.\n\n";

                // parse non-installed MSI
                AnalyzeMsiFile(argument, nullptr); // assume argument is MSI file
            }
        }
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
