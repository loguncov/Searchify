#include "Server.h"

// Конструктор сервера
Server::Server(pqxx::connection& bd, const int port) : bd(bd), port(port) {
    // Устанавливаем локаль для поддержки Unicode
    boost::locale::generator gen;
    std::locale loc = gen.generate("");
    std::locale::global(loc);
    std::wcout.imbue(loc);
}

// Разбор запроса клиента
std::pair<std::string, std::string> Server::Razbor_zaprosa(boost::asio::ip::tcp::socket& socket) {
    char buffer[8192]{};
    socket.read_some(boost::asio::buffer(buffer, 8192));
    std::string zapros = buffer;
    // Извлекаем GET или POST и строку поиска из запроса
    std::string get_post = zapros.substr(0, 5);
    std::smatch slovo_is_zaprosa;
    std::regex_search(zapros, slovo_is_zaprosa, std::regex("zapros=(.*)"));
    return make_pair(get_post, Decoder(slovo_is_zaprosa[1].str()));
}

// Разбор строки на слова
std::vector<std::string> Server::Razbor_stroki(std::string str) {
    // Удаляем знаки пунктуации и приводим к нижнему регистру
    str = std::regex_replace(str, std::regex("[[:punct:]]"), " ");
    str = boost::locale::to_lower(str);
    std::vector<std::string> out;
    std::string temp;
    str.push_back(' ');
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == ' ')
            continue;
        temp += str[i];
        if (str[i + 1] == ' ') {
            out.push_back(temp);
            temp = "";
        }
    }
    return out;
}

// Декодирование строки
std::string Server::Decoder(std::string input) {
    std::string out;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            // Декодируем символы из URL-кодировки
            int code = std::stoi(input.substr(i + 1, 2), nullptr, 16);
            out += static_cast<char>(code);
            i += 2;
        }
        else {
            out += input[i];
        }
    }
    return out;
}

// Формирование запроса к базе данных
std::string Server::Sborka_zaprosa_bd(std::vector<std::string> str) {
    std::string temp;
    for (size_t i = 0; i < str.size(); ++i) {
        if (i != str.size() - 1) {
            temp = temp + "slovo = '" + str[i] + "' or ";
        }
        else temp = temp + "slovo = '" + str[i] + "'";
    }
    return zapros + temp;
}

// Обработка запроса и поиск
std::string Server::Obrabotka_zaprosa(std::pair<std::string, std::string> GetPost_Poisk, pqxx::connection& bd) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    if (GetPost_Poisk.first.find("GET") == 0) {
        // Выводим стартовую страницу при GET запросе
        return converter.to_bytes(zagolovok_otvet + stranica_prefix + stranica_suffix);
    }
    if (GetPost_Poisk.first.find("POST") == 0) {
        if (GetPost_Poisk.second.empty())
            // Выводим стартовую страницу, если строка поиска пуста
            return converter.to_bytes(zagolovok_otvet + stranica_prefix + stranica_suffix);
        std::map<std::string, int>otvet_bd;
        pqxx::work tx{ bd };
        for (auto [host, path, size] : tx.query< std::string, std::string, std::string>(Sborka_zaprosa_bd(Razbor_stroki(GetPost_Poisk.second))))
            otvet_bd[host + path] += std::stoi(size);
        std::vector<std::pair<std::string, int >> sort_otvet;
        for (auto& [ref, size] : otvet_bd)
            sort_otvet.push_back(std::make_pair(ref, size));
        std::sort(sort_otvet.begin(), sort_otvet.end(), [](const std::pair<std::string, int>& pair1, const std::pair<std::string, int>& pair2) {
            return pair1.second > pair2.second;
            });
        int size = sort_otvet.size();
        if (size > 20)
            size = 20;
        for (size_t i = 0; i < size; ++i)
            str += _a + converter.from_bytes(sort_otvet[i].first) + _a_sred + converter.from_bytes(sort_otvet[i].first) + _a_end;
        if (str.empty())
            // Если ответ из базы пустой, выводим "Совпадений не найдено"
            return converter.to_bytes(zagolovok_otvet + stranica_prefix + L"Совпадений не найдено" + stranica_suffix);
    }
    return converter.to_bytes(zagolovok_otvet + stranica_prefix + str + stranica_suffix);
}

// Запуск сервера
void Server::Start_Server() {
    std::cout << "Server ok" << std::endl;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    while (true) {
        try {
            boost::asio::ip::tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::string z = Obrabotka_zaprosa(Razbor_zaprosa(socket), bd);
            boost::asio::write(socket, boost::asio::buffer(z));
            str = L"";
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
};
