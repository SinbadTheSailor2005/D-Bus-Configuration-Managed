//
// Created by aziz on 5/1/25.
//

#include "application.h"

#include <iostream>

#include "sdbus-c++/sdbus-c++.h"

void signal_handler(
    const std::unordered_map<std::string, sdbus::Variant> &parameters) {
  std::cout << "<------------------------------------------------------->\n\n";
  std::cout << "got the configureChanged signal\nNew updated config:\n";
  for (const auto &[key, value] : parameters) {
    // выводим обновленный конфиг
    std::cout << key << ":" << value.get<std::string>() << "\n";
  }
  std::cout << "\n\n";
  std::cout << "<------------------------------------------------------->\n\n";
}

void start_application() {
  sdbus::ServiceName destination{"com.system.configurationManager"};
  sdbus::ObjectPath object_path{
      "/com/system/configurationManager/Application/confManagerApplication1"};

  // создаем прокси на выше объявленный объект
  auto proxy =
      sdbus::createProxy(std::move(destination), std::move(object_path));
  std::cout << "Proxy created\n";

  const sdbus::InterfaceName interface{
      "com.system.configurationManager.Application.Configuration"};

  // словарь с нашей текущей конфигурацией
  std::unordered_map<std::string, sdbus::Variant> config_params;

  // подписываемся на сигнал
  proxy->uponSignal("configurationChanged")
      .onInterface(interface)
      .call([&proxy, &config_params](
                const std::unordered_map<std::string, sdbus::Variant>
                    &parameters) {
        // вызываем обработчик сигнала
        signal_handler(parameters);

        // очищаем предыдущий сохраненный конфиг
        config_params.clear();

        // сохраняем новый конфиг в config_params
        proxy->callMethod("GetConfiguration")
            .onInterface(
                "com.system.configurationManager.Application.Configuration")
            .storeResultsTo(config_params);
      });

  // получаем текущую конфигурацию приложения
  proxy->callMethod("GetConfiguration")
      .onInterface("com.system.configurationManager.Application.Configuration")
      .storeResultsTo(config_params);
  while (true) {
    auto timeout = config_params["Timeout"].get<std::string>();
    auto phrase = config_params["TimeoutPhrase"].get<std::string>();
    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(timeout)));
    std::cout << phrase << "\n";
  }
}
