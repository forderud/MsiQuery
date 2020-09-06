#include "MsiQuery.hpp"
#include "MsiUtil.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

#pragma comment(lib, "Msi.lib")


/** Convert string to lowercase. */
static std::wstring ToLower (std::wstring str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](wchar_t c){ return std::tolower(c); });
    return str;
}


std::wstring ParseMSIorProductCode (const std::wstring& file_or_product) {
    PMSIHANDLE msi;
    if (file_or_product[0] == L'{') {
        // input is a ProductCode
        UINT ret = MsiOpenProduct(file_or_product.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    } else {
        // input is a MSI filename
        UINT ret = MsiOpenPackage(file_or_product.c_str(), &msi);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiOpenPackage failed");
    }

    std::wstring product_code;
    {
        // high-level metadata
        product_code = GetProductProperty(msi, L"ProductCode");
        std::wstring upgrade_code = GetProductProperty(msi, L"UpgradeCode");
        std::wstring product_name = GetProductProperty(msi, L"ProductName");
        std::wcout << L"MSI metadata for " << file_or_product << L":\n";
        std::wcout << L"ProductCode: " << product_code << L"\n";
        std::wcout << L"UpgradeCode: " << upgrade_code << L"\n";
        std::wcout << L"ProductName: " << product_name << L"\n";
    }

    {
        // installation details
        //std::wstring inst_prod_code = GetProductInfo(product_code.c_str(), L"ProductCode");
        //assert(inst_prod_code == product_code);

        std::wstring msi_cache_file = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_LOCALPACKAGE); // Local cached package
        std::wstring inst_loc = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_INSTALLLOCATION); // seem to be empty
        //std::wstring inst_folder = GetProductInfo(product_code.c_str(), L"INSTALLFOLDER"); // not found
        std::wstring prod_id = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_PRODUCTID); // seem to be empty
    }

    return product_code;
}


void ParseInstalledApp (std::wstring product_code) {
    std::wstring msi_cache_file = GetProductInfo(product_code.c_str(), INSTALLPROPERTY_LOCALPACKAGE); // Local cached package

    MsiQuery query(msi_cache_file);

    {
        // custom action query
        // https://docs.microsoft.com/en-us/windows/win32/msi/summary-list-of-all-custom-action-types
        // Wix custom actions:
        // Type 65   (0x41)  =                                                                                      msidbCustomActionTypeContinue (0x40)                                         + msidbCustomActionTypeDll (0x01)
        // Custom EXE register:
        // Type 3106 (0xC22) = msidbCustomActionTypeNoImpersonate (0x800) + msidbCustomActionTypeInScript (0x400)                                        + msidbCustomActionTypeDirectory (0x20) + msidbCustomActionTypeExe (0x02) // Type 34 run executable
        // Type 3170 (0xC62) = msidbCustomActionTypeNoImpersonate (0x800) + msidbCustomActionTypeInScript (0x400) + msidbCustomActionTypeContinue (0x40) + msidbCustomActionTypeDirectory (0x20) + msidbCustomActionTypeExe (0x02)
        auto custom_actions = query.QueryIS(L"SELECT `Type`,`Target` FROM `CustomAction` WHERE `Type` <> 65"); // filter out WiX entries 
        for (auto& ca : custom_actions) {
            std::wcout << L"CustomAction: " << ca.first << L", " << ca.second << L'\n';
        }
    }{
        // component query
        auto components = query.QuerySS(L"SELECT `Component`,`ComponentId` FROM `Component`");

        for (const auto& cmp : components) {
            auto path = GetComponentPath(product_code, cmp.second);
            if (ToLower(path).find(L".exe") != path.npos)
                std::wcout << L"installed EXE: " << path << L'\n';
        }
    }{
        // file query
        auto files = query.QuerySS(L"SELECT `File`,`FileName` FROM `File`");
        for (auto& file : files) {
            std::wcout << L"File: " << file.first << L", " << file.second << L'\n';
        }
    }
}


int wmain (int argc, wchar_t *argv[ ]) {
    if (argc < 2) {
        std::wcout << L"Usage: MsiQuery.exe [<filename.msi>|{product-code}]\n";
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    try {
        // hide MSI installer UI
        MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);

        auto product_code = ParseMSIorProductCode(argv[1]);
        ParseInstalledApp(product_code);
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
