#pragma once
#include <cassert>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <msi.h>


/** Converts ASCII string to unicode */
inline std::wstring ToUnicode(const std::string& s_str) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // function or variable may be unsafe
#endif
    std::wstring w_str(s_str.size(), L'\0');
    mbstowcs(const_cast<wchar_t*>(w_str.data()), s_str.c_str(), w_str.size());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    return w_str;
}

/** Get info about a MSI product that is not neccesarily installed. */
static std::wstring GetProductProperty (MSIHANDLE msi, const wchar_t* property, bool throw_on_failure = true) {
    DWORD buf_len = 0;
    UINT ret = MsiGetProductPropertyW(msi, property, const_cast<wchar_t*>(L""), &buf_len);
    if (ret != ERROR_MORE_DATA) {
        if (throw_on_failure)
            throw std::runtime_error("MsiGetProductProperty failed");
        else
            return L"";
    }

    std::wstring buffer(buf_len++, L'\0');
    ret = MsiGetProductPropertyW(msi, property, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS) {
        if (throw_on_failure)
            throw std::runtime_error("MsiGetProductProperty failed");
        else
            return L"";
    }
    return buffer;
}

/** Get info for published & installed products. */
static std::wstring GetProductInfo (const std::wstring& product_code, const wchar_t* attribute) {
    wchar_t EMPTY_STRING[] = L""; // cannot const_cast(L"") since MsiGetProductInfo will write to empty string

    DWORD buf_len = 0;
    UINT ret = MsiGetProductInfoW(product_code.c_str(), attribute, EMPTY_STRING, &buf_len);
    if (ret == ERROR_SUCCESS)
        return EMPTY_STRING;
    else if (ret == ERROR_UNKNOWN_PRODUCT)
        return L""; // not installed
    else if (ret != ERROR_MORE_DATA)
        throw std::runtime_error("MsiGetProductInfo failed");

    if (buf_len == 32)
        buf_len = 38; // increase buffer to fit a GUID string (prevents additional ERROR_MORE_DATA)

    std::wstring buffer(buf_len++, L'\0');
    ret = MsiGetProductInfoW(product_code.c_str(), attribute, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetProductInfo failed");
    return buffer;
}


static std::wstring GetComponentPath (const std::wstring& product, const std::wstring& component) {
    DWORD buf_len = 0;
    INSTALLSTATE ret = MsiGetComponentPathW(product.c_str(), component.c_str(), nullptr, &buf_len);
    if (ret == INSTALLSTATE_ABSENT)
        throw std::runtime_error("MsiGetComponentPath returned INSTALLSTATE_ABSENT");
    if (ret == INSTALLSTATE_UNKNOWN) // uninstalled feature
        return L"";
    assert(ret == INSTALLSTATE_LOCAL);

    std::wstring buffer(buf_len++, L'\0'); 
    ret = MsiGetComponentPathW(product.c_str(), component.c_str(), const_cast<wchar_t*>(buffer.data()), &buf_len);
    assert(ret == INSTALLSTATE_LOCAL);
    return buffer;
}

/** Returns the first ProductCode associated with a given UpgradeCode, or "" if unknown. */
static std::wstring GetFirstProductCode (const std::wstring& upgrade_code) {
    std::wstring buffer(38, L'\0'); // fixed length
    DWORD idx = 0;
    UINT ret = MsiEnumRelatedProductsW(upgrade_code.c_str(), NULL, idx, const_cast<wchar_t*>(buffer.data()));
    if (ret != ERROR_SUCCESS)
        return L""; // none found

    return buffer;
}
