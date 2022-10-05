#include "MsiQuery.hpp"
#include "MsiUtil.hpp"
#include <algorithm>
#include <fcntl.h>
#include <io.h>
#include <cctype>
#include <iostream>

#pragma comment(lib, "Msi.lib")


void AnalyzeMsiFile(std::wstring msi_file, std::wstring * product_code) {
    MsiQuery query(msi_file);

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

    std::wcout << L"Installed EXE files:\n";
    if (product_code) {
        // convert string to lowercase
        auto to_lowercase = [](std::wstring str) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](wchar_t c) { return std::tolower(c); });
            return str;
        };

        // component listing (sorted by "Component" field for faster lookup)
        std::vector<ComponentEntry> components = query.QueryComponent();
        std::sort(components.begin(), components.end());

        auto files = query.QueryFile();
        for (const FileEntry& file : files) {
            // search for matching component
            auto component = std::lower_bound(components.begin(), components.end(), CreateComponentEntry(file.Component_));
            if (component == components.end())
                throw std::runtime_error("Unable to find ComponentEntry");

            auto path = GetComponentPath(*product_code, component->ComponentId);
            if (to_lowercase(path).find(L".exe") == path.npos)
                continue; // filter out non-EXE files

            std::wcout << L"EXE: " << path << L'\n';
        }
        std::wcout << L"\n";
    } else {
        std::wcout << L"Not available since the app isn't installed.\n";
    }

    {
        std::wcout << L"Registry entries:\n";

        auto reg_entries = query.QueryRegistry();
        for (const RegEntry& reg : reg_entries) {
            std::wcout << L"Registry: " << reg.RootStr() << L", " << reg.Key << L", " << reg.Name << L", " << reg.Value << L'\n';
        }
        if (reg_entries.empty())
            std::wcout << L"<none> (might still be created through custom actions)\n";

        std::wcout << L"\n";
    }
}


std::wstring ParseMSIOrProductCode (std::wstring file_or_product) {
    PMSIHANDLE msi;
    if (file_or_product[0] == L'{') {
        // input is a ProductCode
        std::wcout << L"Attempting to open ProductCode " << file_or_product << L"...\n";
        UINT ret = MsiOpenProduct(file_or_product.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    } else {
        // input is a MSI filename
        std::wcout << L"Attempting to open file " << file_or_product << L"...\n";
        UINT ret = MsiOpenPackage(file_or_product.c_str(), &msi);
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


std::wstring ParseInstalledApp (std::wstring product_code) {
    std::wstring msi_cache_file;
    try {
        // installation details
        //std::wstring inst_prod_code = GetProductInfo(product_code, L"ProductCode");
        //assert(inst_prod_code == product_code);

        msi_cache_file = GetProductInfo(product_code, INSTALLPROPERTY_LOCALPACKAGE); // Local cached package
    } catch (const std::exception &) {
        return L""; // doesn't appear to be installed
    }

    {
        std::wstring inst_loc = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLLOCATION); // seem to be empty
        //std::wstring inst_folder = GetProductInfo(product_code, L"INSTALLFOLDER"); // not found
        std::wstring prod_id = GetProductInfo(product_code, INSTALLPROPERTY_PRODUCTID); // seem to be empty

        std::wstring inst_name = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLEDPRODUCTNAME); // seem identical to ProductName
        std::wstring publisher = GetProductInfo(product_code, INSTALLPROPERTY_PUBLISHER); // seem identical to Manufacturer
        std::wstring version = GetProductInfo(product_code, INSTALLPROPERTY_VERSIONSTRING); // seem identical to ProductVersion
        std::wstring inst_date = GetProductInfo(product_code, INSTALLPROPERTY_INSTALLDATE); // "YYYYMMDD" format

        std::wcout << L"Installed properties:\n";
        std::wcout << L"InstalledProductName: " << inst_name << L" (" << version << L")\n";
        std::wcout << L"Publisher: " << publisher << L"\n";
        std::wcout << L"InstallDate: " << inst_date << L"\n";
        std::wcout << L"MSI cache: " << msi_cache_file << L"\n";
        std::wcout << L"\n";
    }

    return msi_cache_file;
}


void EnumerateInstalledProducts() {
    std::wcout << L"List of installed products:\n";

    for (DWORD idx = 0;; ++idx) {
        std::wstring product_code(38, L'\0'); // fixed length
        UINT ret = MsiEnumProducts(idx, const_cast<wchar_t*>(product_code.data()));
        if (ret == ERROR_NO_MORE_ITEMS)
            break;
        assert(ret == ERROR_SUCCESS);

        std::wcout << idx << L": ProductCode: " << product_code << L'\n';
#ifndef EXTENDED_INFO
        std::wstring msi_cache_file = ParseInstalledApp(product_code);
#else
        try {
            ParseMSIOrProductCode(product_code); // slower, but also gives UpgradeCode
        } catch (const std::exception &) {/*discard errors*/}
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
            if (msi_cache_file.size() > 0) {
                AnalyzeMsiFile(msi_cache_file, &product_code);
            } else {
                std::wcout << L"ProductCode is NOT installed.\n";
                return 0;
            }
        }
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
