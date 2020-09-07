#pragma once
#include <cassert>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <msi.h>


/** Get info about a MSI product that is not neccesarily installed. */
static std::wstring GetProductProperty (MSIHANDLE msi, const wchar_t* property) {
    DWORD buf_len = 0;
    UINT ret = MsiGetProductProperty(msi, property, const_cast<wchar_t*>(L""), &buf_len);
    if (ret != ERROR_MORE_DATA)
        throw std::runtime_error("MsiGetProductProperty failed");

    std::wstring buffer(buf_len++, L'\0');
    ret = MsiGetProductProperty(msi, property, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetProductProperty failed");
    return buffer;
}

/** Get info for published & installed products. */
static std::wstring GetProductInfo (const std::wstring& product_code, const wchar_t* attribute) {
    wchar_t EMPTY_STRING[] = L""; // cannot const_cast(L"") since MsiGetProductInfo will write to empty string

    DWORD buf_len = 0;
    UINT ret = MsiGetProductInfo(product_code.c_str(), attribute, EMPTY_STRING, &buf_len);
    if (ret == ERROR_SUCCESS)
        return EMPTY_STRING;
    else if (ret != ERROR_MORE_DATA)
        throw std::runtime_error("MsiGetProductInfo failed");

    std::wstring buffer(buf_len++, L'\0');
    ret = MsiGetProductInfo(product_code.c_str(), attribute, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetProductInfo failed");
    return buffer;
}


static std::wstring GetComponentPath (const std::wstring& product, const std::wstring& component) {
    DWORD buf_len = 0;
    UINT ret = MsiGetComponentPath(product.c_str(), component.c_str(), nullptr, &buf_len);
    assert(ret == INSTALLSTATE_LOCAL);

    std::wstring buffer(buf_len++, L'\0'); 
    ret = MsiGetComponentPath(product.c_str(), component.c_str(), const_cast<wchar_t*>(buffer.data()), &buf_len);
    assert(ret == INSTALLSTATE_LOCAL);
    return buffer;
}

