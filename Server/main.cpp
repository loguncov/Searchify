#include "Server.h" // Подключаем заголовочный файл для сервера
#include "Parser.h" // Подключаем заголовочный файл для парсера

int main() {
    SetConsoleOutputCP(1251); // Устанавливаем кодировку консоли для корректного отображения русских символов
    try {
        ini_parser par("data.ini"); // Создаем объект парсера и загружаем данные из файла "data.ini"
        
        // Получаем параметры для подключения к базе данных из конфигурационного файла
        pqxx::connection bd("host=" + par.get_value<std::string>("bd.host_bd")
            + " port=" + std::to_string(par.get_value<int>("bd.port_bd"))
            + " dbname=" + par.get_value<std::string>("bd.name_bd")
            + " user=" + par.get_value<std::string>("bd.user_bd")
            + " password=" + par.get_value<std::string>("bd.password_bd"));
        
        int server_port = par.get_value<int>("server.port"); // Получаем порт сервера из конфигурационного файла
        
        // Создаем объект сервера и передаем ему параметры для подключения к базе данных и порт
        Server server(bd, server_port); 
        server.Start_Server(); // Запускаем сервер
    }
    catch (const std::string e) { // Обрабатываем исключения типа std::string
        std::cout << e << std::endl; // Выводим сообщение об ошибке на консоль
    }
    catch (const std::exception& e) { // Обрабатываем исключения типа std::exception
        std::cout << e.what() << std::endl; // Выводим сообщение об ошибке на консоль
    }
    return 0; // Возвращаем 0, чтобы показать успешное завершение программы
}
