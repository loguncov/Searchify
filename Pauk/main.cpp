#include "Pauk.h"   // Подключение заголовочного файла Pauk.h
#include "Parser.h" // Подключение заголовочного файла Parser.h

int main() {
    SetConsoleOutputCP(1251); // Установка кодовой страницы консоли для поддержки русского языка
    try {
        ini_parser par("data.ini"); // Создание объекта парсера INI-файлов и загрузка файла "data.ini"
        INI ini; // Создание объекта для хранения начальных настроек
        // Создание соединения с базой данных, используя параметры из INI-файла
        pqxx::connection bd("host=" + par.get_value<std::string>("bd.host_bd") +
                            " port=" + std::to_string(par.get_value<int>("bd.port_bd")) +
                            " dbname=" + par.get_value<std::string>("bd.name_bd") +
                            " user=" + par.get_value<std::string>("bd.user_bd") +
                            " password=" + par.get_value<std::string>("bd.password_bd"));
        // Загрузка начальных настроек из INI-файла
        ini.start_sayt = par.get_value<std::string>("pauk.start_sayt");
        ini.path = par.get_value<std::string>("pauk.path");
        ini.port = std::to_string(par.get_value<int>("pauk.port"));
        ini.recursiya = par.get_value<int>("pauk.recursiya");
        // Создание объекта для парсинга и обработки сайтов
        Pauk pauk(bd, ini);
    }
    catch (const std::string& e) { // Обработка исключений типа std::string
        std::cout << e << std::endl; // Вывод сообщения об ошибке в консоль
    }
    catch (const std::exception& e) { // Обработка исключений типа std::exception
        std::cout << e.what() << std::endl; // Вывод сообщения об ошибке в консоль
    }
    return 0; // Возвращаем 0, чтобы обозначить успешное завершение программы
}
