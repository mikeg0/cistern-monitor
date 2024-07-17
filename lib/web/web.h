
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

class CMWeb {
    private:
        static void _webSocketClientInit();
        static void _onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
        static void _handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

    public:
        static void initWebServer();
        static void initWebSocket();
        static void cleanupClients();
        static void notifyClients(const String serverEventType, const JSONVar serverEventValue);

};