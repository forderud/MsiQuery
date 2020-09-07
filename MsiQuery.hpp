#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include <Windows.h>
#include <msiquery.h>


/** CustomAction type parser.
REF: https://docs.microsoft.com/en-us/windows/win32/msi/summary-list-of-all-custom-action-types */
struct CustomActionType {
    CustomActionType () {
        memset(this, 0, sizeof(CustomActionType));
    }

    /** Parse MSI CustomAction "Type" column. */
    CustomActionType (int val) {
        (int&)(*this) = val;
    }

    bool HasFields (const CustomActionType other) const {
        // Wix custom actions:
        // Type 65   (0x41)  =                                            Continue (0x40)                    + Dll (0x01)

        // Custom EXE register:
        // Type 3106 (0xC22) = NoImpersonate (0x800) + InScript (0x400)                   + Directory (0x20) + Exe (0x02) // Type 34 run executable
        // Type 3170 (0xC62) = NoImpersonate (0x800) + InScript (0x400) + Continue (0x40) + Directory (0x20) + Exe (0x02)

        return (int)(*this) & (int)other;
    }

    std::wstring ToString() const {
        std::wstring res = L"[";
        if (Dll) res += L"Dll,";
        if (Exe) res += L"Exe,";
        if (JScript) res += L"JScript,";
        if (SourceFile) res += L"SourceFile,";
        if (Directory) res += L"Directory,";
        if (Continue) res += L"Continue,";
        if (Async) res += L"Async,";
        if (Rollback) res += L"Rollback,";
        if (Commit) res += L"Commit,";
        if (InScript) res += L"InScript,";
        if (NoImpersonate) res += L"NoImpersonate,";
        if (HideTarget) res += L"HideTarget,";
        if (TSAware) res += L"TSAware,";
        if (PatchUninstall) res += L"PatchUninstall,";
        return res.substr(0, res.size() - 1) + L"]";
    }

    operator int& () {
        return *reinterpret_cast<int*>(this);
    }
    operator const int& () const {
        return *reinterpret_cast<const int*>(this);
    }

    /** Special combinations:
     * ClientRepeat (0x300) = FirstSequence + OncePerProcess */
    bool Dll           : 1; ///< msidbCustomActionTypeDll or msidbCustomActionTypeBinaryData (0x01)
    bool Exe           : 1; ///< msidbCustomActionTypeExe (0x02)
    bool JScript       : 2; ///< msidbCustomActionTypeJScript (0x04)
    bool padding1      : 1;
    bool SourceFile    : 1; ///< msidbCustomActionTypeSourceFile (0x10)
    bool Directory     : 1; ///< msidbCustomActionTypeDirectory (0x20)
    bool Continue      : 1; ///< msidbCustomActionTypeContinue (0x40)
    bool Async         : 1; ///< msidbCustomActionTypeAsync (0x80)
    bool Rollback      : 1; ///< msidbCustomActionTypeFirstSequence or msidbCustomActionTypeRollback (0x100)
    bool Commit        : 1; ///< msidbCustomActionTypeOncePerProcess or msidbCustomActionTypeCommit (0x200)
    bool InScript      : 1; ///< msidbCustomActionTypeInScript (0x400)
    bool NoImpersonate : 1; ///< msidbCustomActionTypeNoImpersonate (0x800) - run as ADMIN
    bool padding2      : 1;
    bool HideTarget    : 1; ///< msidbCustomActionTypeHideTarget (0x2000)
    bool TSAware       : 1; ///< msidbCustomActionTypeTSAware (0x4000) (Terminal Server)
    bool PatchUninstall: 1; ///< msidbCustomActionTypePatchUninstall (0x8000)
    bool padding3      : 7;
    bool padding4      : 8;
};
static_assert(sizeof(CustomActionType) == sizeof(int), "CustomActionType size mismatch");


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
    std::vector<std::pair<CustomActionType,std::wstring>> QueryIS (const std::wstring& sql_query) {
        PMSIHANDLE msi_view;
        Execute(sql_query, &msi_view);

        std::vector<std::pair<CustomActionType,std::wstring>> result;
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
