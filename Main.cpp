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
        if (!product_code.empty())
            file_or_code = product_code;

        // input is a ProductCode
        UINT ret = MsiOpenProduct(file_or_code.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    } else {
        // input is a MSI filename
        UINT ret = MsiOpenPackage(file_or_code.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    }

    std::wstring product_code;
    {
        // high-level metadata
        product_code = GetProductProperty(msi, L"ProductCode");
        std::wstring upgrade_code = GetProductProperty(msi, L"UpgradeCode");
        std::wstring product_name = GetProductProperty(msi, L"ProductName");
        std::wcout << L"MSI metadata for " << file_or_code << L":\n";
        std::wcout << L"ProductCode: " << product_code << L"\n";
        std::wcout << L"UpgradeCode: " << upgrade_code << L"\n";
        std::wcout << L"ProductName: " << product_name << L"\n";
    }

    return product_code;
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
    }


    MsiQuery query(msi_cache_file);

    {
        // custom action query
        CustomActionType reg_type;
        reg_type.InScript = true;
        reg_type.NoImpersonate = true;

        auto custom_actions = query.QueryIS(L"SELECT `Type`,`Target` FROM `CustomAction`");
        for (auto& ca : custom_actions) {
            if (!ca.first.HasFields(reg_type))
                continue; // discard custom actions that are neither scripts nor run as admin

            std::wcout << L"CustomAction: " << ca.first.ToString() << L", " << ca.second << L'\n';
        }
    }{
        // component query
        auto components = query.QuerySS(L"SELECT `Component`,`ComponentId` FROM `Component`");

        // Convert string to lowercase
        auto to_lowercase = [](std::wstring str) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](wchar_t c){ return std::tolower(c); });
            return str;
        };

        for (const auto& cmp : components) {
            auto path = GetComponentPath(product_code, cmp.second);
            if (to_lowercase(path).find(L".exe") != path.npos)
                std::wcout << L"installed EXE: " << path << L'\n';
        }
    }{
        // file query
        auto files = query.QuerySS(L"SELECT `File`,`FileName` FROM `File`");
        for (auto& file : files) {
            std::wcout << L"File: " << file.first << L", " << file.second << L'\n';
        }
    }

    return true;
}


int wmain (int argc, wchar_t *argv[ ]) {
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
