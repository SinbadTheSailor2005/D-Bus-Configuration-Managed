#!/bin/bash
# если нет директории - создаем
source_path=$(pwd)
if [ ! -d $HOME/com.system.configurationManager ]; then
mkdir $HOME/com.system.configurationManager
fi

cd $HOME/com.system.configurationManager 
echo -e "Timeout:1000\nTimeoutPhrase:hello from aziz" > confManagerApplication1
cd $source_path
cmake -S . -B build
cmake --build build
  
