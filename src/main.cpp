#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <cstdint>
#include <stdexcept>
#include <codecvt>
#include <locale>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
using Money = std::int64_t;
constexpr Money LIMIT = 1000000000000LL;
struct Entry { std::string date, type, description; Money value; };
const char* color(bool terminal, const char* code) { return terminal ? code : ""; }
constexpr const char* GREEN = "\033[1;32m";
constexpr const char* RED = "\033[1;31m";
constexpr const char* RESET = "\033[0m";
std::string decimalMoney(Money value) {
    bool negative = value < 0; if (negative) value = -value;
    std::ostringstream s;
    s << (negative ? "-" : "") << value / 100 << ',' << std::setw(2) << std::setfill('0') << value % 100;
    return s.str();
}
std::string currency(Money value) {
    bool negative = value < 0; if (negative) value = -value;
    std::ostringstream s; s << (negative ? "-R$ " : "R$ ") << value / 100 << ',' << std::setw(2) << std::setfill('0') << value % 100;
    return s.str();
}
bool parseMoney(const std::string& text, Money& value) {
    if (text.empty()) return false;
    bool decimal = false; int digits = 0; Money whole = 0, fraction = 0;
    for (char c : text) {
        if (c == ',' || c == '.') { if (decimal) return false; decimal = true; continue; }
        if (c < '0' || c > '9') return false;
        if (decimal) { if (++digits > 2) return false; fraction = fraction * 10 + c - '0'; }
        else { whole = whole * 10 + c - '0'; if (whole > LIMIT / 100) return false; }
    }
    if (decimal && digits == 0) return false;
    if (digits == 1) fraction *= 10;
    value = whole * 100 + fraction;
    return value > 0 && value <= LIMIT;
}
std::string today() {
    std::time_t t = std::time(nullptr); std::tm tm = *std::localtime(&t);
    std::ostringstream s; s << std::put_time(&tm, "%Y-%m-%d %H:%M:%S"); return s.str();
}
std::string input(const std::string& prompt) {
    std::cout << prompt; std::string s; if (!std::getline(std::cin, s)) throw std::runtime_error("Entrada encerrada.");
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}
void save(const std::filesystem::path& file, const std::vector<Entry>& entries) {
    auto temporary = file; temporary += ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) throw std::runtime_error("Nao foi possivel salvar os dados.");
    for (const auto& e : entries) out << std::quoted(e.date) << ' ' << std::quoted(e.type) << ' ' << e.value << ' ' << std::quoted(e.description) << '\n';
    out.flush(); if (!out) throw std::runtime_error("Falha ao gravar os dados."); out.close();
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Falha ao substituir arquivo de dados.");
#else
    std::filesystem::rename(temporary, file);
#endif
}
std::string xmlEscape(const std::string& text) {
    std::string result;
    for (unsigned char c : text) {
        if (c == '&') result += "&amp;";
        else if (c == '<') result += "&lt;";
        else if (c == '>') result += "&gt;";
        else if (c == '"') result += "&quot;";
        else if (c >= 32 || c == '\t' || c == '\n' || c == '\r') result += static_cast<char>(c);
    }
    return result;
}
void exportSpreadsheet(const std::filesystem::path& file, const std::vector<Entry>& entries) {
    std::ofstream out(file, std::ios::binary);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<?mso-application progid=\"Excel.Sheet\"?>\n"
           "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">"
           "<Styles><Style ss:ID=\"Header\"><Font ss:Bold=\"1\"/><Interior ss:Color=\"#FFE0B2\" ss:Pattern=\"Solid\"/></Style>"
           "<Style ss:ID=\"Text\"><Alignment ss:Vertical=\"Top\" ss:WrapText=\"1\"/></Style>"
           "<Style ss:ID=\"Money\"><NumberFormat ss:Format=\"0.00\"/></Style>"
           "<Style ss:ID=\"Incoming\" ss:Parent=\"Text\"><Font ss:Color=\"#008000\"/></Style>"
           "<Style ss:ID=\"Outgoing\" ss:Parent=\"Text\"><Font ss:Color=\"#C00000\"/></Style>"
           "<Style ss:ID=\"IncomingMoney\" ss:Parent=\"Money\"><Font ss:Color=\"#008000\"/></Style>"
           "<Style ss:ID=\"OutgoingMoney\" ss:Parent=\"Money\"><Font ss:Color=\"#C00000\"/></Style></Styles>"
           "<Worksheet ss:Name=\"Extrato\"><Table>"
           "<Column ss:Width=\"125\"/><Column ss:Width=\"75\"/><Column ss:Width=\"300\"/>"
           "<Column ss:Width=\"110\"/><Column ss:Width=\"110\"/><Row ss:StyleID=\"Header\">";
    for (const auto& title : {"Data", "Tipo", "Descrição", "Valor (R$)", "Saldo (R$)"})
        out << "<Cell><Data ss:Type=\"String\">" << title << "</Data></Cell>";
    out << "</Row>";
    Money balance = 0;
    for (const auto& e : entries) {
        balance += e.type == "ENTRADA" ? e.value : -e.value;
        bool incoming = e.type == "ENTRADA";
        out << "<Row ss:AutoFitHeight=\"1\">";
        for (const auto& value : {e.date, e.type, e.description})
            out << "<Cell ss:StyleID=\"" << (incoming ? "Incoming" : "Outgoing") << "\"><Data ss:Type=\"String\">" << xmlEscape(value) << "</Data></Cell>";
        const Money values[] = {e.value, balance};
        for (int column = 0; column < 2; ++column) {
            Money value = values[column];
            auto number = decimalMoney(value); number[number.find(',')] = '.';
            const char* style = column == 0 ? (incoming ? "IncomingMoney" : "OutgoingMoney") : "Money";
            out << "<Cell ss:StyleID=\"" << style << "\"><Data ss:Type=\"Number\">" << number << "</Data></Cell>";
        }
        out << "</Row>";
    }
    out << "</Table></Worksheet></Workbook>";
    out.flush(); if (!out) throw std::runtime_error("Falha ao exportar planilha XML.");
}
void deleteHistory(const std::filesystem::path& file, std::vector<Entry>& entries, bool terminal) {
    if (terminal) std::cout << "\033[1;31m";
    std::cout << "\n  Você tem certeza que deseja deletar? Esse ato não pode ser revertido. (s/n): ";
    if (terminal) std::cout << "\033[0m";
    if (input("") != "s") {
        std::cout << "  Exclusao cancelada.\n";
        return;
    }
    if (input("  Senha: ") != "1234") {
        std::cout << "  Senha incorreta. Exclusao cancelada.\n";
        return;
    }
    save(file, {});
    entries.clear();
    for (const auto& name : {"extrato.csv", "extrato.xml"}) {
        std::error_code error;
        std::filesystem::remove(file.parent_path() / name, error);
        if (error) std::cout << "  Historico apagado, mas nao foi possivel excluir " << name
                             << ". Feche o arquivo no Excel e repita a exclusao.\n";
    }
    std::cout << "  Todo o historico foi deletado. Saldo zerado.\n";
}
int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);
    bool terminal = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
    DWORD mode; if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode)) SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    bool terminal = isatty(STDOUT_FILENO);
#endif
    try {
        auto folder = std::filesystem::absolute(argv[0]).parent_path();
        if (argc == 3 && std::string(argv[1]) == "--dados") folder = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(folder);
        auto file = folder / "movimentacoes.dat";
        std::vector<Entry> entries;
        if (std::filesystem::exists(file)) {
            std::ifstream in(file); if (!in) throw std::runtime_error("Nao foi possivel abrir os dados.");
            std::string line;
            while (std::getline(in, line)) {
                Entry e; std::istringstream row(line);
                if (!(row >> std::quoted(e.date) >> std::quoted(e.type) >> e.value >> std::quoted(e.description)) || (e.type != "ENTRADA" && e.type != "SAIDA") || e.value <= 0 || e.value > LIMIT) throw std::runtime_error("Arquivo de dados invalido. Preserve o arquivo para recuperacao.");
                row >> std::ws; if (!row.eof()) throw std::runtime_error("Arquivo de dados invalido.");
                entries.push_back(e);
            }
        }
        for (;;) {
            Money incoming = 0, outgoing = 0;
            for (const auto& e : entries) { auto& total = e.type == "ENTRADA" ? incoming : outgoing; if (total > LIMIT - e.value) throw std::runtime_error("Limite financeiro excedido."); total += e.value; }
            if (terminal) std::cout << "\033[2J\033[H\033[38;5;214m";
            std::cout << "\n  ==============================================\n"
                         "             Doc'ê Hamburgueria\n"
                         "                GESTAO FINANCEIRA\n"
                         "  ==============================================\n";
            if (terminal) std::cout << "\033[0m";
            std::cout << '\n' << color(terminal, GREEN) << "  Entradas:     " << currency(incoming) << color(terminal, RESET)
                      << '\n' << color(terminal, RED) << "  Saidas:       " << currency(outgoing) << color(terminal, RESET)
                      << '\n' << color(terminal, incoming - outgoing < 0 ? RED : GREEN)
                      << "  Saldo atual:  " << currency(incoming - outgoing) << color(terminal, RESET)
                      << "\n\n" << color(terminal, GREEN) << "  [1] Registrar entrada / saldo inicial" << color(terminal, RESET)
                      << '\n' << color(terminal, RED) << "  [2] Registrar saida" << color(terminal, RESET)
                      << "\n  [3] Ver extrato\n  [4] Exportar CSV / planilha Excel\n  [5] Deletar todo o historico\n  [0] Sair\n\n";
            auto choice = input("  Escolha: ");
            if (choice == "0") break;
            if (choice == "1" || choice == "2") {
                auto description = input("  Descricao (ou vazio para cancelar): ");
                if (description.empty()) continue;
                if (description.size() > 200 || description.find_first_of("\r\n\t") != std::string::npos) { std::cout << "  Descricao invalida (maximo 200 bytes).\n"; input("  Enter para continuar..."); continue; }
                Money value = 0;
                if (!parseMoney(input("  Valor em R$: "), value)) { std::cout << "  Valor invalido. Use um numero positivo com ate 2 casas decimais.\n"; input("  Enter para continuar..."); continue; }
                if ((choice == "1" ? incoming : outgoing) > LIMIT - value) { std::cout << "  Limite financeiro excedido.\n"; input("  Enter para continuar..."); continue; }
                if (choice == "2" && value > incoming - outgoing) {
                    if (input("  Esta saida deixara saldo negativo. Confirmar? (s/n): ") != "s") continue;
                }
                std::cout << color(terminal, choice == "1" ? GREEN : RED);
                auto confirmation = input("  Confirmar " + currency(value) + "? (s/n): ");
                std::cout << color(terminal, RESET);
                if (confirmation != "s") continue;
                auto updated = entries; updated.push_back({today(), choice == "1" ? "ENTRADA" : "SAIDA", description, value});
                save(file, updated); entries.swap(updated); std::cout << "  Movimentacao salva!\n";
            } else if (choice == "3") {
                Money balance = 0;
                std::cout << "\n  EXTRATO COMPLETO\n";
                if (entries.empty()) std::cout << "  Nenhuma movimentacao registrada.\n";
                for (const auto& e : entries) {
                    bool incomingEntry = e.type == "ENTRADA";
                    balance += incomingEntry ? e.value : -e.value;
                    std::cout << color(terminal, incomingEntry ? GREEN : RED)
                              << "  " << e.date << " | " << e.type << " | " << currency(e.value) << " | " << e.description
                              << color(terminal, RESET) << " | Saldo: " << currency(balance) << '\n';
                }
            } else if (choice == "4") {
                auto csv = folder / "extrato.csv";
                std::ostringstream out;
                out << "sep=;\r\nData;Tipo;Descrição;Valor (R$);Saldo (R$)\r\n";
                Money balance = 0;
                for (const auto& e : entries) {
                    balance += e.type == "ENTRADA" ? e.value : -e.value;
                    std::string escaped; for (char c : e.description) { escaped += c; if (c == '"') escaped += '"'; }
                    // Prevent spreadsheet formulas in user-entered descriptions.
                    if (!escaped.empty() && std::string("=+-@").find(escaped[0]) != std::string::npos) escaped.insert(0, "'");
                    out << e.date << ';' << e.type << ";\"" << escaped << "\";" << decimalMoney(e.value) << ';' << decimalMoney(balance) << "\r\n";
                }
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
                auto encoded = converter.from_bytes(out.str());
                std::ofstream csvOut(csv, std::ios::binary);
                csvOut.write("\xFF\xFE", 2);
                for (char16_t c : encoded) { csvOut.put(static_cast<char>(c & 0xff)); csvOut.put(static_cast<char>(c >> 8)); }
                csvOut.flush(); if (!csvOut) throw std::runtime_error("Falha ao exportar CSV. Feche o arquivo no Excel e tente novamente.");
                auto spreadsheet = folder / "extrato.xml";
                exportSpreadsheet(spreadsheet, entries);
                std::cout << "  Exportado: " << csv.string() << '\n';
                std::cout << "  Planilha com colunas ajustadas: " << spreadsheet.string() << '\n';
            } else if (choice == "5") {
                deleteHistory(file, entries, terminal);
            } else std::cout << "  Opcao invalida.\n";
            input("  Enter para continuar...");
        }
        std::cout << "\n  Dados salvos. Ate logo!\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "\n  " << e.what() << '\n'; return 1; }
}
