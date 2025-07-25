#pragma once
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <Windows.h>
#include <msiquery.h>
#include <MsiDefs.h>


/** https://learn.microsoft.com/en-us/windows/win32/msi/feature-table */
struct FeatureEntry {
    std::wstring Feature;     ///< feature identifier [max 38 chars]
    //std::wstring Feature_Parent;
    std::wstring Title;       ///< short description
    std::wstring Description; ///< longer description [localizable]
    int Display = 0;          ///< UI order
    int Level = 0;            ///< 0=disables installation
    //std::wstring Directory_;
    int Attributes = 0;

    std::wstring ToString() const {
        return L"Title=" + Title + L", Description=" + Description + L", Feature=" + Feature;
    }
};


/** https://docs.microsoft.com/en-us/windows/win32/msi/customaction-table */
struct CustomActionEntry {
    /** CustomAction type parser.
    *   Based on msidbCustomActionType enum in <msidefs.h>
    *   REF: https://docs.microsoft.com/en-us/windows/win32/msi/summary-list-of-all-custom-action-types
    *
    *   Wix custom actions:
    *    Type 65   (0x041)  =                                                                Continue(0x40)                    + Dll(0x01) // ValidatePath, PrintEula
    *   Custom DLL:
    *    Type               =                                                                                                  + Dll(0x01) // Type 1/17 run DLL
    *   Custom EXE:
    *    Type 3106 (0x0C22) =                       NoImpersonate(0x800) + Deferred(0x400)                  + Directory(0x20)  + Exe(0x02) // Type 2/18/34/50 run executable
    *    Type 3170 (0x0C62) =                       NoImpersonate(0x800) + Deferred(0x400) + Continue(0x40) + Directory(0x20)  + Exe(0x02)
    *   Custom JScript:
    *    Type 7189 (0x1C15) = Script64Bit(0x1000) + NoImpersonate(0x800) + Deferred(0x400) +                  SourceFile(0x10) + Script(0x04) + Dll(0x01) // Type 5/21/37/53 JScript
    *    Type 7253 (0x1C55) = Script64Bit(0x1000) + NoImpersonate(0x800) + Deferred(0x400) + Continue(0x40) + SourceFile(0x10) + Script(0x04) + Dll(0x01)
    *   Custom VBScript:
    *    Type               =                                                                                                  + Script(0x04) + Exe(0x02) // Type 6/22/38/54 VBScript
    */
    struct Type {
        Type() {
            memset(this, 0, sizeof(Type)); // replace with default member initializers after upgrading to newer C++ version
        }

        /** Parse MSI CustomAction "Type" column. */
        Type(int val) {
            (int&)(*this) = val;
        }

        std::wstring ToString() const {
            std::wstring res = L"[";
            if (Dll) res += L"Dll,";
            if (Exe) res += L"Exe,";
            if (Script) res += L"Script,";
            if (SourceFile) res += L"SourceFile,";
            if (Directory) res += L"Directory,";
            if (Continue) res += L"Continue,";
            if (Async) res += L"Async,";
            if (Rollback) res += L"Rollback,";
            if (Commit) res += L"Commit,";
            if (Deferred) res += L"Deferred,";
            if (NoImpersonate) res += L"NoImpersonate,";
            if (Script64Bit) res += L"Script64Bit,";
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
        *   msidbCustomActionTypeTextData (0x03) = Dll | Exe
        *   msidbCustomActionTypeJScript (0x05) = 0x04 | Dll
        *   msidbCustomActionTypeVBScript (0x06) = 0x04 | Dll
        *   msidbCustomActionTypeInstall (0x07) = 0x04 | Exe | Dll
        *   msidbCustomActionTypeProperty (0x30) = Directory | File
        *   msidbCustomActionTypeClientRepeat (0x300) = FirstSequence + OncePerProcess */
        bool Dll : 1; ///< msidbCustomActionTypeDll (0x01)
        bool Exe : 1; ///< msidbCustomActionTypeExe (0x02)
        bool Script : 1; ///< script (used by msidbCustomActionTypeJScript (0x05) and msidbCustomActionTypeVBScript (0x06))
        bool _padding1 : 1;
        bool SourceFile : 1; ///< msidbCustomActionTypeSourceFile (0x10)
        bool Directory : 1; ///< msidbCustomActionTypeDirectory (0x20)
        bool Continue : 1; ///< msidbCustomActionTypeContinue (0x40)
        bool Async : 1; ///< msidbCustomActionTypeAsync (0x80)
        bool Rollback : 1; ///< msidbCustomActionTypeFirstSequence or msidbCustomActionTypeRollback (0x100)
        bool Commit : 1; ///< msidbCustomActionTypeOncePerProcess or msidbCustomActionTypeCommit (0x200)
        bool Deferred : 1; ///< msidbCustomActionTypeInScript (0x400) (deferred execution)
        bool NoImpersonate : 1; ///< msidbCustomActionTypeNoImpersonate (0x800) - run as ADMIN
        bool Script64Bit : 1; ///< msidbCustomActionType64BitScript (0x1000)
        bool HideTarget : 1; ///< msidbCustomActionTypeHideTarget (0x2000)
        bool TSAware : 1; ///< msidbCustomActionTypeTSAware (0x4000) (Terminal Server)
        bool PatchUninstall : 1; ///< msidbCustomActionTypePatchUninstall (0x8000)
        bool _padding2 : 8;
        bool _padding3 : 8;
    };
    static_assert(sizeof(Type) == sizeof(int), "CustomAction::Type size mismatch");


    std::wstring Action;
    Type         Type;
    std::wstring Source;
    std::wstring Target;
    std::wstring ExtendedType;
};


/** https://docs.microsoft.com/en-us/windows/win32/msi/registry-table */
struct RegEntry {
    enum RootType : int {
        Dynamic      = -1,// HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE, depending on ALLUSERS
        ClassesRoot  = 0, // HKEY_CLASSES_ROOT
        CurrentUser  = 1, // HKEY_CURRENT_USER
        LocalMachine = 2, // HKEY_LOCAL_MACHINE
        Users        = 3, // HKEY_USERS
    };

    std::wstring RootStr () const {
        switch (Root) {
        case Dynamic: return L"Dynamic";
        case ClassesRoot: return L"ClassesRoot";
        case CurrentUser: return L"CurrentUser";
        case LocalMachine: return L"LocalMachine";
        case Users: return L"Users";
        }
        abort(); // should never be reached
    }

    std::wstring Registry;
    RootType     Root;
    std::wstring Key;
    std::wstring Name;
    std::wstring Value;
    std::wstring Component_;
};


class FileTable {
public:
    /** https://docs.microsoft.com/en-us/windows/win32/msi/file-table */
    struct Entry {
        std::wstring File;
        std::wstring Component_;
        std::wstring FileName; ///< stored in "short-name|long-name" format if longer than 8+3
        //std::wstring Filesize;
        //...

        std::wstring LongFileName() const {
            // Doc: https://learn.microsoft.com/en-us/windows/win32/msi/filename
            size_t idx = FileName.find(L'|');
            if (idx == std::wstring::npos)
                return FileName; // filename 8+3 or shorter

            return FileName.substr(idx + 1); // remove short-name prefix
        }

        bool operator < (const Entry& other) const {
            return File < other.File;
        }
    };

    FileTable(std::vector<Entry> files) : m_files(files) {
        // sort by "File" field
        std::sort(m_files.begin(), m_files.end());
    }

    Entry Lookup(std::wstring File, bool throw_on_failure) {
        // search for matching component
        const Entry val = CreateFileEntry(File);
        auto res = std::lower_bound(m_files.begin(), m_files.end(), val);
        if ((res == m_files.end()) || (val < *res)) {
            if (throw_on_failure)
                throw std::runtime_error("Unable to find FileTable entry");
            else
                return {};
        }

        return *res;
    }

    const std::vector<Entry>& Entries() {
        return m_files;
    }

private:
    static Entry CreateFileEntry(std::wstring File) {
        Entry entry;
        entry.File = File;
        return entry;
    }

    std::vector<Entry> m_files;
};


class DirectoryTable {
public:
    struct Entry {
        std::wstring Directory;
        std::wstring Directory_Parent;
        std::wstring DefaultDir; ///< stored in "short-name|long-name" format if long

        std::wstring LongDefaultDir() const {
            size_t idx = DefaultDir.find(L'|');
            if (idx == std::wstring::npos)
                return DefaultDir; // only short name

            return DefaultDir.substr(idx + 1); // remove short-name prefix
        }

        bool operator < (const Entry& other) const {
            return Directory < other.Directory;
        }
    };

    DirectoryTable(std::vector<Entry> directories) : m_directories(directories) {
        // sort by "Directory" field
        std::sort(m_directories.begin(), m_directories.end());
    }

    std::wstring Lookup(std::wstring Directory) const {
        if (Directory.empty())
            return L"";

        const Entry val = CreateDirectoryEntry(Directory);
        auto res = std::lower_bound(m_directories.begin(), m_directories.end(), val);
        if (res == m_directories.end())
            throw std::runtime_error("Unable to find DirectoryTable entry");
        if (val < *res)
            throw std::runtime_error("Unable to find DirectoryTable entry");

        // recursive lookup
        return Lookup(res->Directory_Parent) + L'\\' + res->LongDefaultDir();
    }

private:
    static Entry CreateDirectoryEntry(std::wstring Directory) {
        Entry entry;
        entry.Directory = Directory;
        return entry;
    }

    std::vector<Entry> m_directories;
};



class ComponentTable {
public:
    /** https://docs.microsoft.com/en-us/windows/win32/msi/component-table */
    struct Entry {
        std::wstring Component;
        std::wstring ComponentId;
        std::wstring Directory_;
        int          Attributes; ///< 0x100=64bit, 0x004=RegistryKeyPath
        //std::wstring Condition;
        //std::wstring KeyPath;

        bool operator < (const Entry& other) const {
            return Component < other.Component;
        }
    };

    ComponentTable(std::vector<Entry> components) : m_components(components) {
        // sort by "Component" field
        std::sort(m_components.begin(), m_components.end());
    }

    Entry Lookup(std::wstring Component) const {
        // search for matching component
        const Entry val = CreateComponentEntry(Component);
        auto res = std::lower_bound(m_components.begin(), m_components.end(), val);
        if (res == m_components.end())
            throw std::runtime_error("Unable to find ComponentTable entry");
        if (val < *res)
            throw std::runtime_error("Unable to find ComponentTable entry");

        return *res;
    }

private:
    static Entry CreateComponentEntry(std::wstring Component) {
        Entry entry;
        entry.Component = Component;
        return entry;
    }

    std::vector<Entry> m_components;
};


/** Query an MSI file. It doesn't need to be installed first.
    Based on WiCompon.vbs sample (installed under C:\Program Files (x86)\Windows Kits\10\bin\<version>\x64) */
class MsiQuery {
public:
    /** Query MSI database. */
    MsiQuery (std::wstring msi_path) {
        // open MSI DB
        UINT ret = MsiOpenDatabaseW(msi_path.c_str(), MSIDBOPEN_READONLY, &m_db);
        if (ret != ERROR_SUCCESS)
            abort();
    }

    ~MsiQuery() {
    }

    std::vector<FeatureEntry> QueryFeature() {
        PMSIHANDLE msi_view;
        Execute(L"SELECT `Feature`,`Title`,`Description`,`Display`,`Level`,`Attributes` FROM `Feature`", &msi_view);

        std::vector<FeatureEntry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            FeatureEntry entry;
            entry.Feature = GetRecordString(msi_record, 1);
            entry.Title = GetRecordString(msi_record, 2);
            entry.Description = GetRecordString(msi_record, 3);
            entry.Display = GetRecordInt(msi_record, 4);
            entry.Level = GetRecordInt(msi_record, 5);
            entry.Attributes = GetRecordInt(msi_record, 6);
            result.push_back(entry);
        }

        return result;
    }

    /** Query Component table. */
    ComponentTable QueryComponent () {
        PMSIHANDLE msi_view;
        Execute(L"SELECT `Component`,`ComponentId`,`Directory_`,`Attributes` FROM `Component`", &msi_view);

        std::vector<ComponentTable::Entry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = GetRecordString(msi_record, 2);
            auto val3 = GetRecordString(msi_record, 3);
            auto val4 = GetRecordInt(msi_record, 4);
            result.push_back({val1, val2, val3, val4});
        }

        return ComponentTable(result);
    }

    /** Query File table. */
    FileTable QueryFile () {
        PMSIHANDLE msi_view;
        Execute(L"SELECT `File`,`Component_`,`FileName` FROM `File`", &msi_view);

        std::vector<FileTable::Entry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = GetRecordString(msi_record, 2);
            auto val3 = GetRecordString(msi_record, 3);
            result.push_back({val1, val2, val3});
        }

        return FileTable(result);
    }

    /** Query Directory table. */
    DirectoryTable QueryDirectory() {
        PMSIHANDLE msi_view;
        Execute(L"SELECT `Directory`,`Directory_Parent`,`DefaultDir` FROM `Directory`", &msi_view);

        std::vector<DirectoryTable::Entry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = GetRecordString(msi_record, 2);
            auto val3 = GetRecordString(msi_record, 3);
            result.push_back({ val1, val2, val3 });
        }

        return DirectoryTable(result);
    }

    /** Query Registry table. */
    std::vector<RegEntry> QueryRegistry () {
        PMSIHANDLE msi_view;
        if (!Execute(L"SELECT `Registry`,`Root`,`Key`,`Name`,`Value`,`Component_` FROM `Registry`", &msi_view))
            return {}; // table not found

        std::vector<RegEntry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = static_cast<RegEntry::RootType>(GetRecordInt(msi_record, 2));
            auto val3 = GetRecordString(msi_record, 3);
            auto val4 = GetRecordString(msi_record, 4);
            auto val5 = GetRecordString(msi_record, 5);
            auto val6 = GetRecordString(msi_record, 6);
            result.push_back({val1, val2, val3, val4, val5, val6});
        }

        return result;
    }

    /** Query CustomAction table. */
    std::vector<CustomActionEntry> QueryCustomAction () {
        PMSIHANDLE msi_view;
        bool ok = Execute(L"SELECT `Action`,`Type`,`Source`,`Target`,`ExtendedType` FROM `CustomAction`", &msi_view);
        if (!ok)
            return {};

        std::vector<CustomActionEntry> result;
        while (true) {
            PMSIHANDLE msi_record;
            UINT ret = MsiViewFetch(msi_view, &msi_record);
            if (ret == ERROR_NO_MORE_ITEMS)
                break;
            if (ret != ERROR_SUCCESS)
                abort();

            auto val1 = GetRecordString(msi_record, 1);
            auto val2 = GetRecordInt(msi_record, 2);
            auto val3 = GetRecordString(msi_record, 3);
            auto val4 = GetRecordString(msi_record, 4);
            auto val5 = GetRecordString(msi_record, 5);
            result.push_back({val1, val2, val3, val4, val5});
        }

        return result;
    }

private:
    bool Execute (const std::wstring& sql_query, MSIHANDLE* view) {
        UINT ret = MsiDatabaseOpenViewW(m_db, sql_query.c_str(), view);
        if (ret == ERROR_BAD_QUERY_SYNTAX)
            return false; // table not found
        if (ret != ERROR_SUCCESS)
            abort();

        ret = MsiViewExecute(*view, NULL);
        if (ret != ERROR_SUCCESS)
            abort();

        return true;
    }

    static std::wstring GetRecordString(MSIHANDLE record, unsigned int field) {
        DWORD buf_len = 0;
        UINT ret = MsiRecordGetStringW(record, field, const_cast<wchar_t*>(L""), &buf_len);
        if (ret != ERROR_MORE_DATA)
            return L"";

        std::wstring buffer(buf_len++, L'\0');
        ret = MsiRecordGetStringW(record, field, const_cast<wchar_t*>(buffer.data()), &buf_len);
        if (ret != ERROR_SUCCESS)
            throw std::runtime_error("MsiRecordGetString failed");

        return buffer;
    }

    static int GetRecordInt(MSIHANDLE record, unsigned int field) {
        int ret = MsiRecordGetInteger(record, field);
        if (ret == MSI_NULL_INTEGER)
            throw std::runtime_error("MsiRecordGetInteger failed");

        return ret;
    }

    PMSIHANDLE m_db; ///< RAII wrapper of MSIHANDLE
};


static std::wstring GetTargetPath (MSIHANDLE msi, const std::wstring& folder) {
    DWORD buf_len = 0;
    UINT ret = MsiGetTargetPathW(msi, folder.c_str(), const_cast<wchar_t*>(L""), &buf_len);
    if (ret != ERROR_MORE_DATA)
        throw std::runtime_error("MsiGetTargetPath failed");

    std::wstring buffer(buf_len++, L'\0'); 
    ret = MsiGetTargetPathW(msi, folder.c_str(), const_cast<wchar_t*>(buffer.data()), &buf_len);
    if (ret != ERROR_SUCCESS)
        throw std::runtime_error("MsiGetTargetPath failed");

    return buffer;
}
