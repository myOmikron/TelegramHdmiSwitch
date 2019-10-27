#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "CREDENTIALS.c"

const char* ssid = SSID;
const char* pass = PASS;

const char* BotToken = BOTTOKEN;

const char* host = "api.telegram.org";
const int httpsPort = 443;

const long* allowedChatIds = ALLOWEDCHATIDS;
const int allowedChatIdsLength = sizeof(allowedChatIds)/sizeof(allowedChatIds[0]);

int updateId = 0;

IRsend sender(D2);
enum operations {power, first, second, third, fourth};

void setup() {
  Serial.begin(115200);
  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("\n\nConnecting to WiFi: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi");

  sender.begin();
  Serial.println("IR Emitter started\n");

}

void loop() {

  String payload = "getUpdates?limit=1";
  
  if(updateId > 0) {
    payload += "&offset=";
    payload += String(updateId);
  }
  
  Serial.println("Payload: " + payload);
  char* getRequest = const_cast<char*>(payload.c_str());
  
  String updates = sendGet(getRequest);
  int len = updates.length();

  if(len == 0) {
    Serial.println("No Response");
    delay(800);
    return;
  }
  
  Serial.println(updates);

  DynamicJsonBuffer jb(sizeof(updates));
  JsonObject& firstObject = jb.parseObject(updates);

  if(!firstObject.success()) {
    Serial.println("Parsing failed");
    delay(800);
    return;
  }

  if(!firstObject["ok"]) {
    Serial.println("JSON \"ok\" is false");
    delay(800);
    return;
  }

  JsonArray& results = firstObject["result"];
  for(JsonObject& result : results) {
    updateId = result["update_id"];
    updateId++;

    JsonObject& msg = result["message"];
    JsonObject& from = msg["from"];

    Serial.printf("AllowedChatIdsLendth: %i\n", allowedChatIdsLength);
    
    for(int i = 0; i < allowedChatIdsLength; i++) {
      long chatId = from["id"];
      if(chatId == allowedChatIds[i]) {
        Serial.println("Found accepted Chat-ID");

        const char* command = msg["text"];
        Serial.printf("Code received: %s\n", command);

        if(strcmp("/1", command) == 0) {
          Serial.println("Execute /1");
          sendCode(first);
        } else if(strcmp("/2", command) == 0) {
          Serial.println("Execute /2");
          sendCode(second);
        } else if(strcmp("/3", command) == 0) {
          Serial.println("Execute /3");
          sendCode(third);
        } else if(strcmp("/4", command) == 0) {
          Serial.println("Execute /4");
          sendCode(fourth);
        } else if(strcmp("/power", command) == 0) {
          Serial.println("Execute /power");
          sendCode(power);
        }
      }
    }
  }
  
  delay(1000);
}



String sendGet(char* query) {
  
  boolean finishedHeaders = false;
  boolean currentLineIsBlank = true;
  boolean gotResponse = false;
  String body = "";
  
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  if(httpsClient.connect(host, 443)) {
    Serial.println("Connection established");

    char URL[500];
    strcpy(URL, "/bot");
    strcat(URL, BotToken);
    strcat(URL, "/");
    strcat(URL, query);
    Serial.println(URL);
    
    httpsClient.print(String("GET ") + URL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

    while(httpsClient.available()) {
      char c = httpsClient.read();
      if(finishedHeaders) {
        body += c;
      } else {
        if(currentLineIsBlank && c=='\n') {
          finishedHeaders = true;
        }
      }

      if(c == '\n') {
        currentLineIsBlank = true;
      } else if(c != '\r') {
        currentLineIsBlank = false;
      }
      gotResponse = true;
    }

    if (gotResponse) {
      return body;
    }
  }
}
  
void sendCode(operations op) {
  switch (op) {
    case power:
        sender.sendNEC(0x1FE48B7, 32);    
      break;
    case first:
      for (int i = 0; i < 3; i++) {
        sender.sendNEC(0x1FEA05F, 32);
      }
      break;
    case second:
      for (int i = 0; i < 3; i++) {
        sender.sendNEC(0x1FEE01F, 32);
      }
      break;
    case third:
      for (int i = 0; i < 3; i++) {
        sender.sendNEC(0x1FE906F, 32);
      }
      break;
    case fourth:
      for (int i = 0; i < 3; i++) {
        sender.sendNEC(0x1FED827, 32);
      }
      break;
  }
}
