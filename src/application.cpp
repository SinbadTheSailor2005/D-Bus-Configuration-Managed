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

  std::string timeout;
  std::string phrase;
  // подписываемся на сигнал
  proxy->uponSignal("configurationChanged")
      .onInterface(interface)
      .call([&proxy, &config_params, &timeout,
             &phrase](const std::unordered_map<std::string, sdbus::Variant>
                          &parameters) {
        // очищаем предыдущий сохраненный конфиг
        config_params.clear();

        // сохраняем новый конфиг в config_params
        proxy->callMethod("GetConfiguration")
            .onInterface(
                "com.system.configurationManager.Application.Configuration")
            .storeResultsTo(config_params);

        // обновляем в обработчике
        // тк во время sleep программа может
        // не обновить phrase и вывести старое значение
        timeout = config_params["Timeout"].get<std::string>();
        phrase = config_params["TimeoutPhrase"].get<std::string>();
        // вызываем обработчик сигнала
        signal_handler(parameters);
      });

  // получаем текущую конфигурацию приложения
  proxy->callMethod("GetConfiguration")
      .onInterface("com.system.configurationManager.Application.Configuration")
      .storeResultsTo(config_params);
  while (true) {
    try {
      timeout = config_params["Timeout"].get<std::string>();
      phrase = config_params["TimeoutPhrase"].get<std::string>();
      std::this_thread::sleep_for(
          std::chrono::milliseconds(std::stoi(timeout)));
      std::cout << phrase << "\n";
    } catch (std::invalid_argument &e) {

      // если параметр неправильного типа
      // например, Timeout:abc5000
      // выводим ошибку и ждем 5 секунд
      std::cout << "ERROR during showing configuration file: " << e.what()
                << "\n";
      std::cout << "waiting 5 seconds for repeat the process...\n";
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
}
