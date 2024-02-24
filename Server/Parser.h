#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <variant>
#include <iomanip>
#include <regex>
#include "syntax.h"

using Big_Map = std::map<std::string, std::map<std::string, std::variant<std::string, int, double>>>;
using Map = std::map<std::string, std::variant<std::string, int, double>>;

class ini_parser {
private:
    syntax stx; // Объект для проверки синтаксиса
    std::vector<std::string> str; // Вектор для хранения строк из файла
    Big_Map big_map; // Карта для хранения данных из ini-файла

    // Загрузка данных из файла
    void load_data(std::string file) {
        std::ifstream fin(file);
        if (!fin.is_open())
            throw std::string("Файл отсутствует");
        std::string data;
        while (getline(fin, data))
            razbor_str(data); // Разбор строки из файла
        fin.close();
        if (stx.ex)
            throw std::string("Исправьте файл");
        upakovka_Big_Map(); // Упаковка данных в карту
    }

    // Разбор строки из файла
    void razbor_str(std::string& data) {
        std::string perem = "", znach = "";
        data = std::regex_replace(data, std::regex("\\s+"), ""); // Удаление пробелов из строки
        for (int i = 0; i < data.length(); ++i) {
            if (data[i] == ';') // Удаление комментариев
                data.erase(i);
        }
        if (data != "") { // Если строка не пустая
            stx.syntax_errors(data); // Проверка синтаксиса строки
            stx.sekciya_perem_znach(data, perem, znach, '='); // Разбор секции, переменной и значения
            if (perem[0] == '[')
                str.push_back(perem); // Добавление секции в вектор
            else {
                str.push_back(perem); // Добавление переменной в вектор
                str.push_back(znach); // Добавление значения в вектор
            }
        }
    }

    // Упаковка данных из вектора в карту
    void upakovka_Big_Map() {
        int temp = 1; // Смещение для переменных и значений от секции
        for (int i = 0; i < str.size();) {
            if (str[i][0] != '[' && i == 0) {
                if (big_map.empty()) temp = 0;
                big_map["[]"][str[temp]] = get_variant(str[temp + 1]); // Добавление данных в карту
            }
            if (str[i][0] == '[') {
                if (i >= str.size() - 1) {
                    big_map[str[i]][""] = "";
                    return;
                }
                if (str[i + 1][0] == '[') {
                    big_map[str[i]][""] = "";
                    temp -= 2;
                }
                else big_map[str[i]][str[i + temp]] = get_variant(str[i + temp + 1]); // Добавление данных в карту
            }
            temp += 2;
            if ((i + temp) > str.size() - 1)
                return;
            if (str[i + temp][0] == '[') {
                i += temp;
                temp = 1;
            }
        }
    }

    // Преобразование строки в нужный тип (строка, целое число, вещественное число)
    std::variant<std::string, int, double> get_variant(std::string data) {
        if (data[0] < 48 || data[0]>57)
            return data;
        int schetchik = 0;
        for (auto& x : data) {
            if (x == '.')
                if (++schetchik > 1)
                    return data;
        }
        if (data[0] >= 48 || data[0] <= 57) {
            double temp = 0; int p = 1; bool db = false;
            for (auto& x : data) {
                if (x != '.') {
                    temp *= 10;
                    temp = (temp + x - 48);
                    if (db)
                        p *= 10;
                }
                else db = true;
            }
            if (db)
                return temp / p;
            else return static_cast<int>(temp);
        }
        return data;
    }

    // Проверка запроса на существование секции и переменной
    void proverka_zaprosa(std::string sekciya, std::string perem) {
        bool sek = false, per = false;
        Big_Map::const_iterator big_ptr;
        for (big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr)
            if (sekciya == big_ptr->first) {
                sek = true;
                break;
            }
        if (!sek)
            throw spisok_sekciy();
        for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr)
            if (perem == ptr->first)
                per = true;
        if (!per)
            throw spisok_perem(big_ptr);
    }

    // Вывод списка секций
    std::string spisok_sekciy() {
        std::string sek = "Секция отсутствует\nпосмотрите список существующих\n";
        for (Big_Map::const_iterator big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr)
            sek = sek + big_ptr->first + "\n";
        return sek;
    }

    // Вывод списка переменных в секции
    std::string spisok_perem(Big_Map::const_iterator big_ptr) {
        std::string perem = "В этой секции переменная отсутствует\nпосмотрите список существующих\n";
        for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
            if (ptr->first == "")
                return "В этой секции переменные отсутствует";
            perem = perem + ptr->first + "\n";
        }
        return perem;
    }

public:
    // Запрещаем создание объектов по умолчанию и операции копирования и перемещения
    ini_parser() = delete;
    ini_parser(const ini_parser&) = delete;
    ini_parser& operator=(const ini_parser&) = delete;
    ini_parser(ini_parser&&) = delete;
    ini_parser& operator=(ini_parser&&) = delete;

    // Конструктор с загрузкой данных из файла
    ini_parser(std::string file) {
        load_data(file);
    }

    // Получение значения переменной из секции
    template<class T>
    T get_value(std::string str) {
        std::string sekciya, perem;
        stx.sekciya_perem_znach(str, sekciya, perem, '.');
        sekciya = "[" + sekciya + "]";
        proverka_zaprosa(sekciya, perem);
        std::string s = typeid(T).name();
        if (std::holds_alternative<int>(big_map.at(sekciya).at(perem))) {
            if (s != "int")
                throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";
            return std::get<int>(big_map.at(sekciya).at(perem));
        } else if (std::holds_alternative<double>(big_map.at(sekciya).at(perem))) {
            if (s != "double")
                throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";
            return std::get<double>(big_map.at(sekciya).at(perem));
        }
        if (std::holds_alternative<std::string>(big_map.at(sekciya).at(perem))) {
            if (std::get<std::string>(big_map.at(sekciya).at(perem)).empty())
                throw "В переменной \"" + perem + "\" значение отсутствует";
        }
        throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"string\"";
    }

    // Получение значения переменной типа string из секции
    std::string get_value(const std::string& str) {
        std::string sekciya, perem;
        stx.sekciya_perem_znach(str, sekciya, perem, '.');
        sekciya = "[" + sekciya + "]";
        proverka_zaprosa(sekciya, perem);
        if (std::holds_alternative<int>(big_map.at(sekciya).at(perem)))
            throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";
        if (std::holds_alternative<double>(big_map.at(sekciya).at(perem)))
            throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";
        if (std::holds_alternative<std::string>(big_map.at(sekciya).at(perem))) {
            if (!std::get<std::string>(big_map.at(sekciya).at(perem)).empty())
                return std::get<std::string>(big_map.at(sekciya).at(perem));
        }
        throw "В переменной \"" + perem + "\" значение отсутствует";
    }

    // Вывод содержимого карты
    void print_am() {
        for (const auto& [key, value] : big_map) {
            std::cout << key << std::endl;
            for (const auto& [inner_key, inner_value] : value) {
                if (!inner_key.empty()) {
                    std::cout << inner_key << " = ";
                    if (std::holds_alternative<std::string>(inner_value))
                        std::cout << std::get<std::string>(inner_value) << std::endl;
                    if (std::holds_alternative<int>(inner_value))
                        std::cout << std::get<int>(inner_value) << std::endl;
                    if (std::holds_alternative<double>(inner_value))
                        std::cout << std::fixed << std::setprecision(1) << std::get<double>(inner_value) << std::endl;
                }
            }
        }
    }
};
