#include "Pauk.h"

// Конструктор класса Pauk
Pauk::Pauk(pqxx::connection& bd, const INI ini) : bd(bd), recursiya(ini.recursiya) {
    // Записываем время старта
    start_ = chrono::steady_clock::now();

    // Создаем таблицы в базе данных, если они не существуют
    pqxx::work w{ bd };
    w.exec("create table if not exists ref(id serial PRIMARY KEY not null,host varchar ,path varchar )");
    w.exec("create table if not exists data(id serial not null, nomer_ref int  references ref (id)  not null, slovo varchar ,size int )");
    w.commit();

    // Настраиваем библиотеку для перевода символов (в том числе русских) в нижний регистр
    boost::locale::generator gen;
    std::locale loc = gen.generate("");
    std::locale::global(loc);
    std::wcout.imbue(loc);

    // Грузим первую задачу на скачивание сайта
    Tasks_HTML.push([ini, this]() { Task_Load_HTML(ini.start_sayt, ini.path, ini.port, recursiya); });

    // Запускаем оба пула
    for (int i = 0; i < std::thread::hardware_concurrency() / 2; ++i) {
        Pool_HTML.emplace_back(&Pauk::Thread_Pool_Load_HTML, this);
        Pool_BD.emplace_back(&Pauk::Thread_Pool_Load_BD, this);
    }
}

// Задача для загрузки страниц и задач по работе с базой данных
void Pauk::Task_Load_HTML(string host, string path, const string port, int recursiya) {
    // Загружаем HTML страницу
    const string html = Load_HTML(host, path, port);
    if (html.empty())
        return;

    // Помещаем задачу в пул для работы с базой данных
    m_HTML.lock();
    Tasks_BD.push([html, host, path, this]() { Pauk::Task_Load_BD(html, host, path); });
    static int x = 0;
    std::cout << "Количество_сайтов: " << ++x << ", ID потока: " << std::this_thread::get_id() << ", Задачи: " << Tasks_HTML.size() << std::endl;
    m_HTML.unlock();

    // Если есть еще доступные рекурсии, ищем ссылки на другие сайты в HTML странице
    if (--recursiya == 0)
        return;
    std::smatch match;
    std::string::const_iterator Html_Start(html.cbegin());
    while (std::regex_search(Html_Start, html.cend(), match, std::regex("<a href=\"(.*?)\""))) {
        auto [Host, Path] = Razbor_Url_HTML(match[1], host);
        Html_Start = match.suffix().first;
        m_HTML.lock();
        ref_HTML.insert(Host + Path);
        int size = ref_HTML.size();
        int prev = prev_ref_size_HTML;
        m_HTML.unlock();
        if (prev == size)
            continue;
        m_HTML.lock();
        prev_ref_size_HTML = size;
        Tasks_HTML.push([Host, Path, port, recursiya, this]() { Pauk::Task_Load_HTML(Host, Path, port, recursiya); });
        m_HTML.unlock();
    }
}

// Загрузка HTML страницы по указанному URL
string Pauk::Load_HTML(const string host, const string path, const string port) {
    string html;
    try {
        // Устанавливаем соединение с сервером
        io_service servis;
        ssl::context context(ssl::context::sslv23_client);
        context.set_default_verify_paths();
        ssl::stream<ip::tcp::socket> ssocket = { servis, context };
        ssocket.set_verify_mode(ssl::verify_none);
        ip::tcp::resolver resolver(servis);
        auto it = resolver.resolve(host, port);
        connect(ssocket.lowest_layer(), it);
        ssocket.handshake(ssl::stream_base::handshake_type::client);

        // Формируем HTTP запрос и отправляем его
        http::request<http::string_body> req{ http::verb::get, path, 11 };
        req.set(http::field::host, host);
        m_HTML.lock();
        http::write(ssocket, req);
        m_HTML.unlock();

        // Получаем ответ от сервера и считываем HTML
        http::response<http::string_body> res;
        flat_buffer buffer;
        http::read(ssocket, buffer, res);
        html = res.body().data();
    }
    catch (const std::exception& e) {
        // Обработка ошибок
    }
    return html;
}

// Разбор ссылки на Host и Path
std::pair<string, string> Pauk::Razbor_Url_HTML(const string& url, string& Host) {
    string host, path;
    try {
        boost::urls::url_view URL(url);
        host = URL.host(); path = URL.path();
        if (host.empty())
            host = Host;
        else Host = host;
        if (path.empty())
            path = "/";
    }
    catch (const std::exception& e) {
        // Обработка ошибок
    }
    return std::make_pair(host, path);
}

// Задача для очистки строки HTML и загрузки в базу данных
void Pauk::Task_Load_BD(string html, string host, string path) {
    static int count = 0;
    try {
        // Очищаем строку HTML и загружаем данные в базу данных
        auto slova = Html_v_Slova_v_Map(html, 3, 15);
        m_BD.lock();
        ++count;
        std::cout << "Количество_загрузок: " << count << ", ID потока: " << std::this_thread::get_id() << ", Задачи_BD: " << Tasks_BD.size() << std::endl;
        pqxx::work w{ bd };
        w.exec("insert into ref(host, path) values ('" + host + "', '" + path + "')");
        for (auto [key, size] : slova) {
            w.exec("insert into data(nomer_ref, slovo, size) values (" + std::to_string(count) + ", '" + key + "', " + std::to_string(size) + ")");
        }
        w.commit();
        m_BD.unlock();
    }
    catch (const std::exception& e) {
        // Обработка ошибок
        m_BD.unlock();
        --count;
    }
}

// Очистка строки HTML и разбиение ее на слова
std::map<string, int> Pauk::Html_v_Slova_v_Map(string& html, int min_slovo, int max_slovo) {
    // Убираем теги HTML
    bool trigger = false;
    for (int i = 0; i < html.size(); ++i) {
        if (html[i] == '<')
            trigger = true;
        if (html[i] == '>')
            trigger = false;
        if (trigger) {
            html.erase(i, 1); --i;
        }
    }

    // Убираем все, кроме букв
    for (int i = 0; i < html.size(); ++i) {
        unsigned char z = html[i];
        if (z < 32 || (z > 32 && z < 65) || (z > 90 && z < 97) || (z > 122 && z < 128)) {
            html[i] = ' ';
        }
    }

    // Убираем лишние пробелы
    for (int i = 0; i < html.size() - 1; ++i) {
        if (html[i] == ' ' && html[i + 1] == ' ') {
            html.erase(i, 1);
            --i;
        }
    }

    // Убираем первый пробел
    if (html[0] == ' ')
        html.erase(0, 1);

    // Переводим в нижний регистр
    html = boost::locale::to_lower(html);

    // Разбиваем строку на слова и сохраняем в map
    std::map<string, int> map;
    string temp;
    for (auto& x : html) {
        int temp_min, temp_max;
        if (x != ' ') {
            temp += x;
        }
        else {
            if (unsigned(temp[0]) >= 208) {
                temp_min = min_slovo * 2;  temp_max = max_slovo * 2;
            }
            else {
                temp_min = min_slovo; temp_max = max_slovo;
            }
            if (temp.size() <= temp_min) {
                temp = ""; continue;
            }
            if (temp.size() > temp_max) {
                temp = ""; continue;
            }
            map[temp]++;
            temp = "";
        }
    };
    return map;
}

// Пул потоков для загрузки HTML страниц
void Pauk::Thread_Pool_Load_HTML() {
    while (Stop_Pool_HTML || !Tasks_HTML.empty()) {
        int x = Tasks_HTML.size();
        if (x < 10)
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5 * 100));
        m_HTML.lock();
        if (!Tasks_HTML.empty()) {
            auto task = Tasks_HTML.front();
            Tasks_HTML.pop();
            m_HTML.unlock();
            task();
        }
        else { m_HTML.unlock(); }
    }
}

// Пул потоков для загрузки данных в базу
void Pauk::Thread_Pool_Load_BD() {
    while (Stop_Pool_BD || !Tasks_BD.empty()) {
        int x = Tasks_BD.size();
        if (x < 20)
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5 * 100));
        m_BD.lock();
        if (!Tasks_BD.empty()) {
            auto task = Tasks_BD.front();
            Tasks_BD.pop();
            m_BD.unlock();
            task();
        }
        else { m_BD.unlock(); }
    }
}
