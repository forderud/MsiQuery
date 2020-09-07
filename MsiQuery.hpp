#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include <Windows.h>
#include <msiquery.h>


/** Query an installed MSI file. 
    Based on WiCompon.vbs sample (installed under C:\Program Files (x86)\Windows Kits\10\bin\<version>\x64) */
class MsiQuery {
public:
    /** Query MSI database. */
    MsiQuery (std::wstring msi_path) {
        // open MSI DB
        UINT ret = MsiOpenDatabase(msi_path.c_str(), MSIDBOPEN_READONLY, &m_db);
        if (ret != ERROR_SUCCESS)
            abort();
    }

    ~MsiQuery() {
    }

    /** Perform query that returns two strings per row. */
    std::vector<std::pair<std::wstring,std::wstring>> QuerySS (std::wstring sql_query) {
        PMSIHANDLE msi_view;
        Execute(sql_query, &msi_view);

        std::vector<std::pair<std::wstring,std::wstring>> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = GetRecordString(msi_record, 2);
            result.push_back({val1, val2});
        }

        return result;
    }

    /** Perform query that returns an int and string per row. */
    std::vector<std::pair<int,std::wstring>> QueryIS (const std::wstring& sql_query) {
        PMSIHANDLE msi_view;
        Execute(sql_query, &msi_view);

        std::vector<std::pair<int,std::wstring>> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordInt(msi_record, 1);
            auto val2 = GetRecordString(msi_record, 2);
            result.push_back({val1, val2});
        }

        return result;
    }

private:
    void Execute (const std::wstring& sql_query, MSIHANDLE* view) {
        UINT ret = MsiDatabaseOpenView(m_db, sql_query.c_str(), view);
        if (ret != ERROR_SUCCESS)
            abort();

        ret = MsiViewExecute(*view, NULL);
        if (ret != ERROR_SUCCESS)
            abort();
    }

    static std::wstring GetRecordString(MSIHANDLE record, unsigned int field) {
        std::wstring buffer(255, L'\0');
        DWORD buf_len = (DWORD)buffer.size();

        UINT ret = MsiRecordGetString(record, field, const_cast<wchar_t*>(buffer.data()), &buf_len);
        if (ret != ERROR_SUCCESS)
            return L"";

        buffer.resize(buf_len);
        return buffer;
    }

    static int GetRecordInt(MSIHANDLE record, unsigned int field) {
        int ret = MsiRecordGetInteger(record, field);
        if (ret == MSI_NULL_INTEGER)
            return 0;

        return ret;
    }

    PMSIHANDLE m_db; ///< RAII wrapper of MSIHANDLE
};


static std::wstring GetTargetPath (MSIHANDLE msi, const wchar_t* folder) {
    std::wstring buffer(MAX_PATH, L'\0'); 
    DWORD buf_len = (DWORD)buffer.size();
    UINT ret = MsiGetTargetPath(msi, folder, const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetTargetPath failed");

    return buffer;
}

/** CustomAction type parser.
    REF: https://docs.microsoft.com/en-us/windows/win32/msi/summary-list-of-all-custom-action-types */
struct CustomActionType {
    CustomActionType () = default;
    /** Parse MSI CustomAction "Type" column. */
    CustomActionType (int val) {
        Dll = val & 0x01;
        Exe = val & 0x02;
        Directory = val & 0x20;
        Continue = val & 0x40;
        InScript = val & 0x400;
        NoImpersonate = val & 0x800;
    }

    bool Dll           = false; ///< msidbCustomActionTypeDll (0x01)
    bool Exe           = false; ///< msidbCustomActionTypeExe (0x02)
    bool Directory     = false; ///< msidbCustomActionTypeDirectory (0x20)
    bool Continue      = false; ///< msidbCustomActionTypeContinue (0x40)
    bool InScript      = false; ///< msidbCustomActionTypeInScript (0x400)
    bool NoImpersonate = false; ///< msidbCustomActionTypeNoImpersonate (0x800) - run as ADMIN
    // TODO: Add missing types
};
