#pragma once
#include <iostream>
#include <string>
#include <exception>

using std::string;
using std::cout;
using std::endl;
using std::exception;

class syntax {
    friend class ini_parser;
    bool ex = false;

    // Проверка синтаксиса строки
    void syntax_errors(const string& str) {
        string temp_perem = "";
        string temp_znach = "";
        try {
            if (skobki(str))
                return;
            ravno(str);
            sekciya_perem_znach(str, temp_perem, temp_znach,'=');
            sintax_perem(temp_perem, str);
            sintax_znach(temp_znach, str);
        }
        catch (string iskl) {
            cout << iskl << endl;
            ex = true;
        }        
    }

    //Проверка алфавита
    void abc(const string& str) {
		for (const size_t& n : str)
			if (n > 127) throw "Cинтаксическая ошибка в строке \"" + str + "\" неверный символ (рус)";
	}

    // Проверка наличия квадратных скобок для секции
    bool skobki(const string& str) {
        for (const auto& x : str) {
            if (x == '[' || x == ']') {
                if (str[0] == '[' && str[str.size() - 1] == ']') {
                    abc(str); // Проверка алфавита
                    return true;
                }
                else throw "Cинтаксическая ошибка в строке \"" + str + "\" отсутствует скобка ";
            }
        }
        return false;
    }

    // Проверка наличия знака равенства
    void ravno(const string& str) {
        int chetchik = 0;
        for (const auto& s : str) {
            if (s == '=')
                ++chetchik;
        }
        if (chetchik == 0)
            throw "Cинтаксическая ошибка в строке \"" + str + "\" отсутствует знак '='";            
    }

    // Разделение строки на переменную и значение
    void sekciya_perem_znach(const string& str, string& perem, string& znach, char simvol) {
        bool flag = true;
        for (const auto& s : str) {
            if (s != simvol && flag)
                perem += s;
            else {
                if (flag) {
                    flag = false;
                    continue;
                }
                znach += s;
            }
        }
    }

    // Проверка синтаксиса переменной
    void sintax_perem(const string& temp_perem, const string& str) {
        abc(temp_perem); // Проверка алфавита
        if (temp_perem[0] < 65 || (temp_perem[0] > 90 && temp_perem[0] < 95) || temp_perem[0] == 96 || temp_perem[0] >122)
            if (temp_perem == "")
                throw "Cинтаксическая ошибка в строке \"" + str + "\" переменная не содержит имени";
            else throw "Cинтаксическая ошибка в строке \"" + str + "\" переменная \"" + temp_perem + "\" не может начинаться с '" + temp_perem[0] + "'";
        for (const auto& s : temp_perem)
            if (s < 46 || (s > 46 && s < 48) || (s > 57 && s < 65) || (s > 90 && s < 95) || s == 96 || s > 122)
                throw "Cинтаксическая ошибка в строке \"" + str + "\" переменная \"" + temp_perem + "\" не может содержать '" + s + "'";
    }

    // Проверка синтаксиса значения
    void sintax_znach(const string& temp_znach, const string& str) {
        if (temp_znach[0] == 46 || (temp_znach[0] > 47 && temp_znach[0] < 58))
            for (const auto& s : temp_znach) {
                if ( s < 46 || (s > 46 && s < 48) || s > 57)
                    throw "Cинтаксическая ошибка в строке \"" + str + "\" значение \"" + temp_znach + "\" должно быть числовым";
            }
    }
};
