//
// Created by aziz on 5/1/25.
//

#include "application.h"
#include <iostream>
#include "sdbus-c++/sdbus-c++.h"
void signal_handler(const std::unordered_map<std::string, sdbus::Variant>& parameters)
{

}

void start_application()
{
    // createing proxy object
    sdbus::ServiceName destination {"com.system.configurationManager"};
    sdbus::ObjectPath object_path {"/com/system/configurationManager/Application/confManagerApplication1"};
    auto proxy = sdbus::createProxy(std::move(destination), std::move(object_path));
    std:: cout << "Proxy created\n";
    // subscribe to the signal
    sdbus::InterfaceName interface {"com.system.configurationManager.Application.Configuration"};
    proxy->uponSignal("configurationChanged").onInterface(interface)
    .call([](const std::unordered_map<std::string, sdbus::Variant>&parameters)
    {
        signal_handler(parameters);
    });

    std::unordered_map<std::string, sdbus::Variant> config_params;

    proxy->callMethod("GetConfiguration")
    .onInterface("com.system.configurationManager.Application.Configuration")
    .storeResultsTo(config_params);

    std::cout<<"iam here!\n";
    while (true)
    {
        std::cout << "i am here!";
        // auto timeout = config_params["Timeout"].get<std::string>();
        auto phrase = config_params["TimeoutPhrase"].get<std::string>();
        sleep(1);
        std::cout << phrase << "\n";
    }

}
