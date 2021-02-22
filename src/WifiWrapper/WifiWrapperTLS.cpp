#include "WifiWrapper.hpp"

WifiWrapperTLS::WifiWrapperTLS() : 
        WifiWrapper(), wifiClient() {
    client.setClient(wifiClient);
    createAPServer(apServer);
}

bool WifiWrapperTLS::begin() {
    ConnectionInfo info = EepromManager::readConnectionInfo();
#if defined(ESP8266)
    caCertX509 = X509List(info.CERT);
    wifiClient.setTrustAnchors(&caCertX509);
    wifiClient.allowSelfSignedCerts();
    wifiClient.setFingerprint(info.FINGERPRINT);
#else
    wifiClient.setCACert(info.CERT);
#endif

    return checkConn();
}
