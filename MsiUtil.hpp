#pragma once
#include <cassert>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <msi.h>


/** Get info about a MSI product that is not neccesarily installed. */
static std::wstring GetProductProperty (MSIHANDLE msi, const wchar_t* property) {
    std::wstring buffer(255, L'\0');
    DWORD buf_len = (DWORD)buffer.size();
    UINT ret = MsiGetProductProperty(msi, property, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret == ERROR_MORE_DATA)
        throw std::runtime_error("Insufficient MsiGetProductProperty buffer size");
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetProductProperty failed");
    buffer.resize(buf_len);
    return buffer;
}

/** Get info for published & installed products. */
static std::wstring GetProductInfo (const std::wstring& product_code, const wchar_t* attribute) {
    std::wstring buffer(255, L'\0');
    DWORD buf_len = (DWORD)buffer.size();
    UINT ret = MsiGetProductInfo(product_code.c_str(), attribute, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret == ERROR_MORE_DATA) {
        throw std::runtime_error("Insufficient MsiGetProductInfo buffer size");
    }
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetProductProperty failed");
    buffer.resize(buf_len);
    return buffer;
}


static std::wstring GetComponentPath (const std::wstring& product, const std::wstring& component) {
    std::wstring buffer(MAX_PATH, L'\0'); 
    DWORD buf_len = (DWORD)buffer.size();

    UINT ret = MsiGetComponentPath(product.c_str(), component.c_str(), const_cast<wchar_t*>(buffer.data()), &buf_len);
    assert(ret == INSTALLSTATE_LOCAL);
    buffer.resize(buf_len);
    return buffer;
}

