#include "MsiQuery.hpp"
#include "MsiUtil.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

#pragma comment(lib, "Msi.lib")


/** Returns the first ProductCode associated with a given UpgradeCode. */
static std::wstring GetProductCode (const std::wstring& upgrade_code) {
    std::wstring buffer(38, L'\0'); // fixed length
    DWORD idx = 0;
    UINT ret = MsiEnumRelatedProducts(upgrade_code.c_str(), NULL, idx, const_cast<wchar_t*>(buffer.data()));
    if (ret != ERROR_SUCCESS)
        return L""; // none found

    return buffer;
}


std::wstring ParseMSIOrProductCodeOrUpgradeCode (std::wstring file_or_code) {
    PMSIHANDLE msi;
    if (file_or_code[0] == L'{') {
        // check if input is UpgradeCode
        auto product_code = GetProductCode(file_or_code);
        if (!product_code.empty()) {
            std::wcout << L"UpgradeCode " << file_or_code << L" is associated with ProductCode " << product_code << L"\n";
            file_or_code = product_code;
        }

        // input is a ProductCode
        std::wcout << L"Attempting to open ProductCode " << file_or_code << L"...\n";
        UINT ret = MsiOpenProduct(file_or_code.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    } else {
        // input is a MSI filename
        std::wcout << L"Attempting to open file " << file_or_code << L"...\n";
        UINT ret = MsiOpenPackage(file_or_code.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
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
        std::wcout << L"ProductCode: " << product_code << L"\n";
        std::wcout << L"UpgradeCode: " << upgrade_code << L"\n";
        std::wcout << L"ProductName: " << product_name << L" (" << product_ver << L")\n";
        std::wcout << L"Manufacturer: " << manufacturer << L"\n";
        std::wcout << L"\n";
    }

    return product_code;
}

static ComponentEntry FindComponent(const std::vector<ComponentEntry>& components, const std::wstring& Component_) {
    for (const ComponentEntry& entry : components) {
        if (Component_ == entry.Component)
            return entry;
    }

    throw std::runtime_error("Unable to find ComponentEntry");
}


bool ParseInstalledApp (std::wstring product_code) {
    std::wstring msi_cache_file;
    try {
        // installation details
        //std::wstring inst_prod_code = GetProductInfo(product_code.c_str(), L"ProductCode");
        //assert(inst_prod_code == product_code);

        msi_cache_file = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_LOCALPACKAGE); // Local cached package
    } catch (const std::exception & e) {
        e;
        return false; // doesn't appear to be installed
    }

    {
        std::wstring inst_loc = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_INSTALLLOCATION); // seem to be empty
        //std::wstring inst_folder = GetProductInfo(product_code.c_str(), L"INSTALLFOLDER"); // not found
        std::wstring prod_id = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_PRODUCTID); // seem to be empty

        std::wstring inst_name = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_INSTALLEDPRODUCTNAME); // seem identical to ProductName
        std::wstring publisher = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_PUBLISHER); // seem identical to Manufacturer
        std::wstring version = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_VERSIONSTRING); // seem identical to ProductVersion
        std::wstring inst_date = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_INSTALLDATE); // "YYYYMMDD" format

        std::wcout << L"Installed properties:\n";
        std::wcout << L"InstalledProductName: " << inst_name << L" (" << version << L")\n";
        std::wcout << L"Publisher: " << publisher << L"\n";
        std::wcout << L"InstallDate: " << inst_date << L"\n";
        std::wcout << L"\n";
    }

    MsiQuery query(msi_cache_file);

    {
        std::wcout << L"Custom actions that might affect system state:\n";
        //REF: https://docs.microsoft.com/en-us/windows/win32/msi/changing-the-system-state-using-a-custom-action

        auto custom_actions = query.QueryCustomAction();
        for (const CustomActionEntry& ca : custom_actions) {
            if (!ca.Type.NoImpersonate && !ca.Type.Deferred)
                continue; // discard custom actions that neither run as admin nor are deferred

            std::wcout << L"CustomAction: " << ca.Type.ToString() << L", " << ca.Target << L'\n';
        }
        std::wcout << L"\n";
    }

    {
        std::wcout << L"Installed EXE files:\n";

        // convert string to lowercase
        auto to_lowercase = [](std::wstring str) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](wchar_t c){ return std::tolower(c); });
            return str;
        };

        std::vector<ComponentEntry> components = query.QueryComponent();

        auto files = query.QueryFile();
        for (const FileEntry& file : files) {
            ComponentEntry cmp = FindComponent(components, file.Component_);

            auto path = GetComponentPath(product_code, cmp.ComponentId);
            if (to_lowercase(path).find(L".exe") == path.npos)
                continue; // filter out non-EXE files

            std::wcout << L"EXE: " << path << L'\n';
        }
        std::wcout << L"\n";
    }

    {
        std::wcout << L"Registry entries:\n";

        auto reg_entries = query.QueryRegistry();
        for (const RegEntry& reg : reg_entries) {
            std::wcout << L"Registry: " << reg.RootStr() << L", " << reg.Key << L", " << reg.Name << L", " << reg.Value << L'\n';
        }
        std::wcout << L"\n";
    }

    return true;
}


int wmain (int argc, wchar_t *argv[]) {
    if (argc < 2) {
        std::wcout << L"Usage: " << argv[0] << L" [<filename.msi>|{ProductCode}|{UpgradeCode}]\n";
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // hide MSI installer UI
    MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);

    try {
        auto product_code = ParseMSIOrProductCodeOrUpgradeCode(argv[1]);
        if(!ParseInstalledApp(product_code)) {
            std::wcout << L"ProductCode is NOT installed.\n";
            return 0;
        }
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
