#include "core/scheduled_clean.hpp"

#include <windows.h>

#include <sddl.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <system_error>
#include <vector>

#include "platform/paths.hpp"

namespace stc::core::scheduled {
namespace {

namespace fs_std = std::filesystem;

std::wstring xml_escape(std::wstring_view in)
{
    std::wstring out;
    out.reserve(in.size());
    for (wchar_t ch : in)
    {
        switch (ch)
        {
            case L'&':  out.append(L"&amp;");  break;
            case L'<':  out.append(L"&lt;");   break;
            case L'>':  out.append(L"&gt;");   break;
            case L'"':  out.append(L"&quot;"); break;
            case L'\'': out.append(L"&apos;"); break;
            default:    out.push_back(ch);     break;
        }
    }
    return out;
}

std::wstring current_user_sid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return {};

    DWORD len = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &len);
    std::vector<BYTE> buf(len);
    auto* user_t = reinterpret_cast<PTOKEN_USER>(buf.data());
    if (!GetTokenInformation(token, TokenUser, user_t, len, &len))
    {
        CloseHandle(token);
        return {};
    }

    LPWSTR sid_str = nullptr;
    std::wstring m_sid;
    if (ConvertSidToStringSidW(user_t->User.Sid, &sid_str))
    {
        m_sid = sid_str;
        LocalFree(sid_str);
    }
    CloseHandle(token);
    return m_sid;
}

std::wstring make_task_xml(const fs_std::path& exe_path)
{
    std::wstring sid = current_user_sid();
    std::wstring exe = xml_escape(exe_path.wstring());

    // Hand-built string instead of wstringstream for this one. The stream version pulled in
    // <sstream> which felt heavy for what is basically a printf job, and the resulting code
    // walked off the right side of the editor anyway. Plain += is fine here.
    std::wstring out;
    out.reserve(1024);
    out += L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>";
    out += L"<Task version=\"1.4\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">";
    out += L"<RegistrationInfo>";
    out += L"<Description>Runs Steam Tracer Cleaner Quick Clean at user logon.</Description>";
    out += L"</RegistrationInfo>";
    out += L"<Triggers><LogonTrigger><Enabled>true</Enabled>";
    if (!sid.empty())
    {
        out += L"<UserId>";
        out += sid;
        out += L"</UserId>";
    }
    out += L"</LogonTrigger></Triggers>";
    out += L"<Principals><Principal id=\"Author\">";
    if (!sid.empty())
    {
        out += L"<UserId>";
        out += sid;
        out += L"</UserId>";
    }
    out += L"<LogonType>InteractiveToken</LogonType>"
           L"<RunLevel>LeastPrivilege</RunLevel>"
           L"</Principal></Principals>"
           L"<Settings>"
           L"<MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>"
           L"<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"
           L"<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"
           L"<AllowHardTerminate>true</AllowHardTerminate>"
           L"<StartWhenAvailable>true</StartWhenAvailable>"
           L"<RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>"
           L"<IdleSettings><StopOnIdleEnd>false</StopOnIdleEnd>"
           L"<RestartOnIdle>false</RestartOnIdle></IdleSettings>"
           L"<AllowStartOnDemand>true</AllowStartOnDemand>"
           L"<Enabled>true</Enabled>"
           L"<Hidden>false</Hidden>"
           L"<RunOnlyIfIdle>false</RunOnlyIfIdle>"
           L"<WakeToRun>false</WakeToRun>"
           L"<ExecutionTimeLimit>PT15M</ExecutionTimeLimit>"
           L"<Priority>7</Priority>"
           L"</Settings>"
           L"<Actions Context=\"Author\">"
           L"<Exec><Command>";
    out += exe;
    out += L"</Command><Arguments>--scheduled</Arguments></Exec></Actions></Task>";
    return out;
}

bool run_schtasks(const std::wstring& cmdline) {
    std::wstring mut = cmdline;
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, mut.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        spdlog::warn("CreateProcessW(schtasks) failed: {}", GetLastError());
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

bool write_utf16_with_bom(const fs_std::path& path, std::wstring_view content) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    constexpr unsigned char bom[2] = {0xFF, 0xFE};
    f.write(reinterpret_cast<const char*>(bom), 2);
    f.write(reinterpret_cast<const char*>(content.data()),
            static_cast<std::streamsize>(content.size() * sizeof(wchar_t)));
    return f.good();
}

}  // namespace

TaskInfo query() {
    TaskInfo info;
    std::wstring cmd = std::wstring{L"schtasks /Query /TN \""} + kTaskName + L"\" /FO LIST";
    info.registered = run_schtasks(cmd);
    return info;
}

bool register_logon_task(const fs_std::path& exe_path) {
    auto xml = make_task_xml(exe_path);
    auto temp = stc::platform::temp_dir() / L"steam-tracer-cleaner-task.xml";
    if (!write_utf16_with_bom(temp, xml)) {
        return false;
    }
    std::wstring cmd = std::wstring{L"schtasks /Create /TN \""} + kTaskName +
                       L"\" /XML \"" + temp.wstring() + L"\" /F";
    bool ok = run_schtasks(cmd);
    std::error_code ec;
    fs_std::remove(temp, ec);
    return ok;
}

bool unregister() {
    std::wstring cmd = std::wstring{L"schtasks /Delete /TN \""} + kTaskName + L"\" /F";
    return run_schtasks(cmd);
}

}  // namespace stc::core::scheduled
