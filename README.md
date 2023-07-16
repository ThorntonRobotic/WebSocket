# WebSocket

submodules:
 # https://github.com/abseil/abseil-cpp - think this is needed for jetson webrtc library
https://github.com/chriskohlhoff/asio/
https://github.com/zaphoyd/websocketpp/

git submodule add https://github.com/zaphoyd/websocketpp/
git submodule add https://github.com/chriskohlhoff/asio/
git submodule add https://github.com/open-source-parsers/jsoncpp

-- To populate submodules

git submodule init
git submodule update