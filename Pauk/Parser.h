#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <variant>
#include <iomanip>
#include <regex>
#include "syntax.h"

using Big_Map = std::map<string, std::map<string, std::variant<string, int, double>>>;
using Map = std::map<string, std::variant<string, int, double>>;

class ini_parser {
private:
	syntax stx;
	std::vector<string>str;
	Big_Map big_map;

	// Метод для загрузки данных из файла
	void load_data(string file) {
		std::ifstream fin(file);
		if (!fin.is_open())
			throw static_cast<string>("Файл отсутствует");

		string data;
		while (getline(fin, data))
			razbor_str(data);

		fin.close();

		// Проверка на наличие синтаксических ошибок
		if (stx.ex)
			throw static_cast < string>("Исправьте файл");

		// Упаковка данных в Big_Map
		upakovka_Big_Map();
	}

	// Метод для разбора строки из файла
	void razbor_str(string& data) {
		string perem = "";
		string znach = "";

		// Удаление пробелов из строки
		data = std::regex_replace(data, std::regex("\\s+"), "");

		// Удаление комментариев из строки
		data.erase(std::remove(data.begin(), data.end(), ';'), data.end());

		if (data != "") {
			// Проверка на синтаксические ошибки
			stx.syntax_errors(data);

			// Разбор строки на переменную и значение
			stx.sekciya_perem_znach(data, perem, znach, '=');

			if (perem[0] == '[')
				str.push_back(perem);
			else {
				str.push_back(perem);
				str.push_back(znach);
			}
		}
	}

	// Метод для упаковки данных в Big_Map
	void upakovka_Big_Map() {
		int temp = 1;

		for (int i = 0; i < str.size();) {
			if (str[i][0] != '[' && i == 0) {
				if (big_map.empty()) temp = 0;
				big_map["[]"][str[temp]] = get_variant(str[temp + 1]);
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
				else big_map[str[i]][str[i + temp]] = get_variant(str[i + temp + 1]);
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

	// Метод для определения типа значения и его преобразования
	std::variant<string, int, double> get_variant(string data) {
		if (data[0] < 48 || data[0]>57)
			return data;

		int schetchik = 0;
		for (auto& x : data) {
			if (x == '.')
				if (++schetchik > 1)
					return data;
		}

		if (data[0] >= 48 || data[0] <= 57) {
			double temp = 0;
			int p = 1;
			bool db = false;

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

	// Метод для проверки запроса
	void proverka_zaprosa(string sekciya, string perem) {
		bool sek = false;
		bool per = false;
		Big_Map::const_iterator big_ptr;

		for (big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr) {
			if (sekciya == big_ptr->first) {
				sek = true;
				break;
			}
		}

		if (!sek)
			throw spisok_sekciy();

		for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
			if (perem == ptr->first)
				per = true;
		}

		if (!per)
			throw spisok_perem(big_ptr);
	}

	// Метод для формирования сообщения об отсутствующей секции
	string spisok_sekciy() {
		string sek = "Секция отсутствует\nпосмотрите список существующих\n";

		for (Big_Map::const_iterator big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr) {
			sek = sek + big_ptr->first + "\n";
		}

		return sek;
	}

	// Метод для формирования сообщения об отсутствующей переменной в секции
	string spisok_perem(Big_Map::const_iterator big_ptr) {
		string perem = "В этой секции переменная отсутствует\nпосмотрите список существующих\n";

		for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
			if (ptr->first == "")
				return "В этой секции переменные отсутствует";

			perem = perem + ptr->first + "\n";
		}

		return perem;
	}

public:
	// Конструктор
	ini_parser(string file) {
		load_data(file);
	}

	// Метод для получения значения из конфигурационного файла
	template<class T>
	T get_value(string str) {
		string sekciya = "";
		string perem = "";
		stx.sekciya_perem_znach(str, sekciya, perem, '.');

		sekciya = "[" + sekciya + "]";
		proverka_zaprosa(sekciya, perem);

		string s = typeid(T).name();

		if (std::holds_alternative<int>(big_map.at(sekciya).at(perem))) {
			if (s != "int")
				throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";

			return  std::get<int>(big_map.at(sekciya).at(perem));
		}
		else if (std::holds_alternative<double>(big_map.at(sekciya).at(perem))) {
			if (s != "double")
				throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";

			return std::get<double>(big_map.at(sekciya).at(perem));
		}

		if (std::holds_alternative<string>(big_map.at(sekciya).at(perem)))
			if (std::get<string>(big_map.at(sekciya).at(perem)) == "")
				throw "В переменной \"" + perem + "\" значение отсутствует";

		throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"string\"";
	}

	// Перегрузка метода для получения значения string из конфигурационного файла
	template<>
	string get_value(string str) {
		string sekciya = "";
		string perem = "";
		stx.sekciya_perem_znach(str, sekciya, perem, '.');

		sekciya = "[" + sekciya + "]";
		proverka_zaprosa(sekciya, perem);

		if (std::holds_alternative<int>(big_map.at(sekciya).at(perem)))
			throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";

		if (std::holds_alternative<double>(big_map.at(sekciya).at(perem)))
			throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";

		if (std::holds_alternative<string>(big_map.at(sekciya).at(perem)))
			if (std::get<string>(big_map.at(sekciya).at(perem)) != "")
				return  std::get<string>(big_map.at(sekciya).at(perem));

		throw "В переменной \"" + perem + "\" значение отсутствует";
	}

	// Метод для печати содержимого Big_Map
	void print_am() {
		for (Big_Map::const_iterator big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr) {
			std::cout << big_ptr->first << std::endl;

			for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
				if (ptr->first != "") {
					std::cout << ptr->first << " = ";

					if (std::holds_alternative<string>(ptr->second))
						std::cout << std::get<string>(ptr->second) << std::endl;

					if (std::holds_alternative<int>(ptr->second))
						std::cout << std::get<int>(ptr->second) << std::endl;

					if (std::holds_alternative<double>(ptr->second))
						std::cout << std::fixed << std::setprecision(1) << std::get<double>(ptr->second) << std::endl;
				}
			}
		}
	}
};
