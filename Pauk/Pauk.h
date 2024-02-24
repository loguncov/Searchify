#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url/parse.hpp>
#include <boost/locale.hpp>
#include <regex>
#include <queue>
#include <unordered_set>
#include <map>
#include <pqxx/pqxx>

// Использование пространств имен для удобства, но внимание к потенциальным конфликтам имен
using std::string;
using namespace boost::beast;
using namespace boost::asio;

// Структура для хранения начальных настроек
struct INI {    
    string start_sayt; // Начальный сайт для парсинга
    string path;       // Путь
    string port;       // Порт
    int recursiya;     // Глубина рекурсии    
};

// Класс Pauk для парсинга сайтов
class Pauk {
private:
    std::chrono::steady_clock::time_point start_; // Время старта
    std::chrono::steady_clock::time_point end;    // Время окончания

    // Пул потоков для загрузки HTML
    int recursiya;
    std::vector<std::thread> Pool_HTML; // Пул потоков для HTML
    std::queue<std::function<void()>> Tasks_HTML; // Задачи на скачивание
    std::unordered_set<string> ref_HTML; // Проверка на уникальность URL
    int prev_ref_size_HTML = 0; // Предыдущий размер ref_HTML
    std::mutex m_HTML; // Мьютекс для HTML
    bool Stop_Pool_HTML = true; // Флаг остановки пула HTML

    void Task_Load_HTML(string host, const string path, const string port, int recursiya); // Загрузка HTML
    void Task_Load_BD(string html, string host, string path); // Работа с базой данных
    string Load_HTML(const string host, const string path, const string port); // Загрузка страницы
    std::pair<string, string> Razbor_Url_HTML(const string& url, string& Host); // Разбор URL
    void Thread_Pool_Load_HTML(); // Пул потоков для загрузки HTML

    // Пул потоков для работы с базой данных
    std::vector<std::thread> Pool_BD; // Пул потоков для базы данных
    std::queue<std::function<void()>> Tasks_BD; // Задачи для работы с базой
    std::mutex m_BD; // Мьютекс для базы данных
    bool Stop_Pool_BD = true; // Флаг остановки пула базы данных
    pqxx::connection& bd; // Соединение с базой данных

    std::map<string, int> Html_v_Slova_v_Map(string& html, int min_slovo, int max_slovo); // Очистка HTML от ненужных данных
    void Thread_Pool_Load_BD(); // Пул потоков для работы с базой данных

public:
    // Конструкторы и операторы копирования/перемещения запрещены для предотвращения их неправомерного использования
    Pauk() = delete;
    Pauk(const Pauk&) = delete;
    Pauk(const Pauk&&) = delete;
    Pauk& operator=(const Pauk& other) = delete;
    Pauk& operator=(const Pauk&& other) = delete;

    Pauk(pqxx::connection& bd, const INI ini); // Конструктор с параметрами
    ~Pauk(); // Деструктор
};
