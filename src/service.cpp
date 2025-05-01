//
// Created by aziz on 5/1/25.
//

#include "service.h"
#include <sdbus-c++/sdbus-c++.h>
#include <filesystem>
#include <iostream>
#include <fstream>


std::vector<std::unique_ptr<sdbus::IObject>> create_objects(std::unique_ptr<sdbus::IConnection>& connection)
{
    // вектор указателей на sdbus объекты, которые будут созданы и возвращены
    std::vector<std::unique_ptr<sdbus::IObject>>objects;
  // Определяем путь до папки с конфигурациями.
    // вместо ~ используем переменную окружения HOME
    const std::filesystem::path dir_path = std::getenv("HOME")
        + std::string("/com.system.configurationManager");
    // если директория не существует или путь указывает на файл -> throw exception
    if (!(std::filesystem::exists(dir_path)
        && std::filesystem::is_directory(dir_path)))
    {
        throw std::runtime_error("No such file or directory");
    }
    std::cout << "determine path: " << dir_path.string() << '\n';


    // Проходим по всем файлам в директории
    for (const auto& entry : std::filesystem::directory_iterator(dir_path))
    {
        if (entry.is_regular_file()) // проверяем что файл не является директорией (они считаются тоже файлами)
        {
            // берем путь к файлу и имя  файла
            std::string file_path = entry.path().string();
            std::string filename = entry.path().filename().string();
            std::cout << "examine file: " << filename << '\n';
            // в документации сказано,
            // что путь к объекту должен быть разделен знаком /, а не .
            sdbus::ObjectPath object_path
                {"/com/system/configurationManager/Application/" + filename};
            // создаем d-bus объект
            auto object = sdbus::createObject(
                *connection, std::move(object_path));

            // Cоздаем доп обычный указатель на объект.
            // Eго мы передадим в лямбда функцию, тк
            // unique pointer будет перемещен в вектор objects
            // (тк после выхода из for цикла все созданные объекты удалятся и мы должны их сохранить)
            // и обратится к объекту будет невозможно по нему.
            // тк все unqique pointers доживают до конца программы, утечек памяти не будет!
            auto raw_pointer_to_object = object.get();


            // создаем метод ChangeConfiguration
            auto ChangeConfiguration = [raw_pointer_to_object, file_path](
                std::string key, sdbus::Variant value)
            {

                // храним обновленные параметры конф файла, чтобы потом обновить его
                std::unordered_map<std::string, sdbus::Variant> parameters;
                std::ifstream config(file_path);
                if (!config.is_open())
                {
                    throw std::runtime_error("Could not open the file");
                }

                std::string conf_key, conf_value;
                // проверить, есть ли в конф файле введенные параметр
                bool isUpdated = false;
                // считываем файл
                while (config >> conf_key >> conf_value)
                {
                    // нашли параметр, который нужно изменить
                    if (conf_key == key)
                    {

                        parameters[conf_key] = value;
                        isUpdated = true;
                        continue;
                    }

                    // оборачиваем в тип Variant
                    parameters[conf_key] = sdbus::Variant(conf_value);
                }
                //  не нашли нужные ключ  -> throw sdbus exception
                if (!isUpdated)
                    throw sdbus::Error(sdbus::Error::Name{
                                           "org.freedesktop.DBus.Error.InvalidArgs"
                                       },
                                       "No such parameter in configuration");
                config.close();
                // перезаписываем файл
                std::ofstream upd_config(file_path);
                for (const auto& [key,value] : parameters)
                {
                    // assume that value is a string type)
                    upd_config << key << " " << value.get<std::string>() <<
                        "\n";
                }
                upd_config.close();

                // Посылаем сигнал configurationChanged
                raw_pointer_to_object->emitSignal("configurationChanged")
                      .onInterface(
                          "com.system.configurationManager.Application.Configuration")
                      .withArguments(parameters);
            };

            // Создаем функцию GetConfiguration
            auto GetConfiguration = [file_path
                ]()-> std::unordered_map<std::string, sdbus::Variant>
            {
                // мапа с содержимым конфиг файла
                std::unordered_map<std::string, sdbus::Variant> parameters;
                // начинаем считывать конфиг файл
                // данные в конфиге хранятся в виде key:value на строку
                std::ifstream config(file_path);
                std::string key, value;
                while (config >> key >> value)
                {
                    parameters[key] = sdbus::Variant(value);
                }
                config.close();
                return parameters;
            };
            // регистрируем d-bus методы и сигнал
            std::cout << "done registering object: " << object->getObjectPath()
                << '\n';
            object->addVTable(sdbus::registerMethod("ChangeConfiguration")
                              .implementedAs(std::move(ChangeConfiguration)),
                              sdbus::registerMethod("GetConfiguration").
                              implementedAs(std::move(GetConfiguration)),
                              sdbus::registerSignal("configurationChanged")
                              .withParameters<std::unordered_map<
                                  std::string, sdbus::Variant>>())
                  .forInterface(
                      "com.system.configurationManager.Application.Configuration");
            objects.push_back(std::move(object)); // сохраняем d-bus объект
        }
    }
    return objects;
}

void start_service()
{
    // Создаем сессионную шину
    sdbus::ServiceName service_name{"com.system.configurationManager"};
    auto connection = sdbus::createSessionBusConnection(service_name);

    std::cout << "Creating objects..." << "\n";
    // создаем d-bus объекты
    std::vector<std::unique_ptr<sdbus::IObject>> objects = create_objects(connection);

    std::cout << "Start listening connections...\n";
    // запускаем I/O цикл на шине
    connection->enterEventLoop();
}
