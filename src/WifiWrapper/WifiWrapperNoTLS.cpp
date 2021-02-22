#include "WifiWrapper.hpp"
 
 WifiWrapperNoTLS::WifiWrapperNoTLS() : 
        WifiWrapper(), wifiClient() {
    client.setClient(wifiClient);
    createAPServer(apServer);
}

bool WifiWrapperNoTLS::begin() {
    return checkConn();
}