cmake_minimum_required(VERSION 3.8)
project(Diplom)

# Установка стандарта C++ на C++17
set(CMAKE_CXX_STANDARD 17)

# Копирование файла data.ini в бинарную директорию сборки
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data.ini ${CMAKE_CURRENT_BINARY_DIR}/data.ini COPYONLY)

# Поиск и подключение библиотеки Boost
find_package(Boost REQUIRED COMPONENTS system)

# Глобальные пути для поиска заголовочных файлов OpenSSL
include_directories($ENV{OPENSSL_ROOT_DIR}/include)

# Глобальные пути для поиска библиотек OpenSSL и Boost
link_directories($ENV{OPENSSL_ROOT_DIR}/lib)
link_directories($ENV{BOOST_ROOT}/stage/lib)

# Добавление поддиректории сборки библиотеки libpqxx
add_subdirectory(libpqxx libpqxx-build) 

# Создание исполняемого файла Diplom из исходных файлов
add_executable(Diplom main.cpp Pauk.h Pauk.cpp Parser.h syntax.h)

# Линковка исполняемого файла с необходимыми библиотеками
target_link_libraries(Diplom Boost::boost libcrypto_static libssl_static pqxx)
