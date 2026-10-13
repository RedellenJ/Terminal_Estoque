#pragma once
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <sys/wait.h>
#endif
class Supabase {
    FILE *writer = nullptr, *reader = nullptr;
#ifdef _WIN32
    HANDLE process = nullptr;
#else
    pid_t process = -1;
#endif
    std::string line() {
        std::string result;
        int c;
        while ((c = std::fgetc(reader)) != EOF && c != '\n') {
            if (c != '\r') result += static_cast<char>(c);
        }
        if (c == EOF) throw std::runtime_error("A conexao foi encerrada. Tente abrir o programa novamente.");
        return result;
    }
public:
    explicit Supabase(const std::filesystem::path& folder) {
        auto script = folder / "supabase.ps1";
        if (!std::filesystem::exists(script)) throw std::runtime_error("Um arquivo necessario esta ausente. Reinstale o programa.");
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
        HANDLE childRead, childWrite, parentRead, parentWrite;
        if (!CreatePipe(&childRead, &parentWrite, &sa, 0)) throw std::runtime_error("Nao foi possivel iniciar a conexao.");
        if (!CreatePipe(&parentRead, &childWrite, &sa, 0)) {
            CloseHandle(childRead); CloseHandle(parentWrite);
            throw std::runtime_error("Nao foi possivel iniciar a conexao.");
        }
        SetHandleInformation(parentRead, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(parentWrite, HANDLE_FLAG_INHERIT, 0);
        wchar_t system[MAX_PATH]; GetSystemDirectoryW(system, MAX_PATH);
        std::wstring shell = std::wstring(system) + L"\\WindowsPowerShell\\v1.0\\powershell.exe";
        std::wstring command = L"\"" + shell + L"\" -NoLogo -NoProfile -ExecutionPolicy Bypass -File \"" + script.wstring() + L"\"";
        STARTUPINFOW si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = childRead; si.hStdOutput = childWrite; si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        PROCESS_INFORMATION pi{};
        BOOL started = CreateProcessW(shell.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        CloseHandle(childRead); CloseHandle(childWrite);
        if (!started) { CloseHandle(parentRead); CloseHandle(parentWrite); throw std::runtime_error("Falha ao iniciar PowerShell."); }
        process = pi.hProcess; CloseHandle(pi.hThread);
        writer = _fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(parentWrite), _O_BINARY), "wb");
        reader = _fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(parentRead), _O_BINARY), "rb");
#else
        int toChild[2], fromChild[2];
        if (pipe(toChild)) throw std::runtime_error("Nao foi possivel iniciar a conexao.");
        if (pipe(fromChild)) { close(toChild[0]); close(toChild[1]); throw std::runtime_error("Nao foi possivel iniciar a conexao."); }
        process = fork();
        if (process == 0) {
            dup2(toChild[0], STDIN_FILENO); dup2(fromChild[1], STDOUT_FILENO);
            close(toChild[0]); close(toChild[1]); close(fromChild[0]); close(fromChild[1]);
            execlp("pwsh", "pwsh", "-NoLogo", "-NoProfile", "-File", script.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        close(toChild[0]); close(fromChild[1]);
        if (process < 0) { close(toChild[1]); close(fromChild[0]); throw std::runtime_error("Falha ao iniciar auxiliar."); }
        writer = fdopen(toChild[1], "w"); reader = fdopen(fromChild[0], "r");
#endif
        if (!writer || !reader) throw std::runtime_error("Nao foi possivel abrir a conexao.");
    }
    Supabase(const Supabase&) = delete;
    ~Supabase() {
        if (writer) { std::fputs("QUIT\n", writer); std::fflush(writer); std::fclose(writer); }
        if (reader) std::fclose(reader);
#ifdef _WIN32
        if (process) { WaitForSingleObject(process, 35000); CloseHandle(process); }
#else
        if (process > 0) waitpid(process, nullptr, 0);
#endif
    }
    static std::string json(const std::string& value) {
        std::string result = "\"";
        for (unsigned char c : value) {
            if (c == '"' || c == '\\') { result += '\\'; result += c; }
            else if (c < 32) {
                const char* hex = "0123456789abcdef";
                result += "\\u00"; result += hex[c >> 4]; result += hex[c & 15];
            } else result += c;
        }
        return result + "\"";
    }
    std::vector<Entry> request(const std::string& command) {
        if (std::fputs((command + "\n").c_str(), writer) < 0 || std::fflush(writer)) throw std::runtime_error("Nao foi possivel enviar os dados. Tente novamente.");
        auto status = line();
        if (status.rfind("ERROR ", 0) == 0) throw std::runtime_error(status.substr(6));
        if (status.rfind("OK ", 0) != 0) throw std::runtime_error("Nao foi possivel ler a resposta do servidor.");
        auto count = std::stoull(status.substr(3));
        std::vector<Entry> result;
        for (std::size_t i = 0; i < count; ++i) {
            std::istringstream row(line()); Entry e;
            if (!(row >> std::quoted(e.date) >> std::quoted(e.type) >> e.value >> std::quoted(e.description)) ||
                (e.type != "ENTRADA" && e.type != "SAIDA") || e.value <= 0 || e.value > LIMIT)
                throw std::runtime_error("Movimentacao invalida recebida do banco.");
            result.push_back(e);
        }
        return result;
    }
    std::vector<Entry> load() { return request("LOAD"); }
    void add(const Entry& e, bool allowNegative) {
        request("ADD {\"p_type\":" + json(e.type) + ",\"p_description\":" + json(e.description) +
            ",\"p_value\":" + std::to_string(e.value) + ",\"p_allow_negative\":" + (allowNegative ? "true" : "false") + "}");
    }
    void clear() { request("DELETE"); }
};
