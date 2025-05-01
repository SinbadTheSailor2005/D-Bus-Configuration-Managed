//
// Created by aziz on 5/1/25.
//

#include "service.h"
#include <sdbus-c++/sdbus-c++.h>
#include <filesystem>
#include <iostream>
#include <fstream>
// Initialize dbus objects, register methods, interface and signals
void create_objects(const std::unique_ptr<sdbus::IConnection>& connection)
{
    // determine directory with configs
    const std::filesystem::path dir_path = std::getenv("HOME")
        + std::string("/com.system.configurationManager");
    // no such directory or it is a file -> throw exception
    if (!(std::filesystem::exists(dir_path)
        && std::filesystem::is_directory(dir_path)))
    {
        throw std::runtime_error("No such file or directory");
    }
    else
    {
        // iterate through files in directory
        for (const auto& entry : std::filesystem::directory_iterator(dir_path))
        {
            if (entry.is_regular_file())
            {
                // obtaining file's metadata
                std::string file_path = entry.path().string();
                std::string filename = entry.path().filename().string();
                // according to documentation,
                // object path should be separated with /, not with .
                sdbus::ObjectPath object_path
                    {"/com/system/configurationManager/Application/" + filename};
                // create dbus object
                auto object = sdbus::createObject(
                    *connection, std::move(object_path));
                // declaring ChangeConfigation method
                auto ChangeConfiguration = [& object, file_path](
                    std::string key, sdbus::Variant value)
                {
                    std::unordered_map<std::string, sdbus::Variant> parameters;
                    std::ifstream config(file_path);
                    if (!config.is_open())
                    {
                        throw std::runtime_error("Could not open the file");
                    }
                    // storing updated config to later overwrite
                    // corresponding config
                    // std::vector<std::string>updated_config;
                    std::string conf_key, conf_value;
                    bool isUpdated = false;
                    // reading config file
                    while (config >> conf_key >> conf_value)
                    {
                        // found needed parameter and update it
                        if (conf_key == key)
                        {
                            // TODO: а если не string?
                            // conf_value = value.get<std::string>();
                            parameters[conf_value] = value;
                            isUpdated = true;
                            continue;
                        }
                        // std::string parameters = conf_key+ " ";
                        // parameters.append(conf_value);
                        // updated_config.push_back(parameters);
                        // wrap string into Variant type
                        parameters[conf_key] = sdbus::Variant(conf_value);
                    }
                    // no such parameter in config  -> throw sdbus exception
                    if (!isUpdated)
                        throw sdbus::Error(sdbus::Error::Name{
                                               "org.freedesktop.DBus.Error.InvalidArgs"
                                           },
                                           "No such parameter in configuration");
                    config.close();
                    // overwrite config
                    std::ofstream upd_config(file_path);
                    for (const auto& [key,value] : parameters)
                    {
                        // assume that value is a string type)
                        upd_config << key << " " << value.get<std::string>() <<
                            "\n";
                    }
                    upd_config.close();

                    // declaring signal
                    object->emitSignal("configurationChanged")
                          .onInterface(
                              "com.system.configurationManager.Application.Configuration")
                          .withArguments(parameters);
                };

                // retrieving configuration
                auto GetConfiguration = [&object, file_path]()->std::unordered_map<std::string, sdbus::Variant>
                {
                    std::unordered_map<std::string, sdbus::Variant> parameters;
                    std::ifstream config(file_path);
                    std::string key, value;
                    while (config >> key >> value)
                    {
                        parameters[key] = sdbus::Variant(value);
                    }
                    return parameters;
                };
                // register dbus methods
                object->addVTable(sdbus::registerMethod("ChangeConfiguration")
                    .implementedAs(std::move(ChangeConfiguration)),
                    sdbus::registerMethod("GetConfiguration").implementedAs(std::move(GetConfiguration)),
                    sdbus::registerSignal("configurationChanged")
                    .withParameters<std::unordered_map<std::string, sdbus::Variant>>())
                .forInterface("com.system.configurationManager.Application.Configuration");
            }
        }
    }
}

void start_service()
{
    sdbus::ServiceName service_name{"com.system.configurationManager"};
    auto connection = sdbus::createSessionBusConnection(service_name);

    create_objects(connection);
    connection->enterEventLoop();
}
