//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 01 ESP32 Cam QR Code Scanner
/*
 * Reference :
 * - ESP32-CAM QR Code Reader (off-line) : https://www.youtube.com/watch?v=ULZL37YqJc8
 * - ESP32QRCodeReader_Page : https://github.com/fustyles/Arduino/tree/master/ESP32-CAM_QRCode_Recognition/ESP32QRCodeReader_Page
 * 
 * The source of the "quirc" library I shared on this project: https://github.com/fustyles/Arduino/tree/master/ESP32-CAM_QRCode_Recognition/ESP32QRCodeReader_Page
 */

/* ======================================== Including the libraries. */
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "quirc.h"
#include <ArduinoJson.h>
/* ======================================== */

// creating a task handle
TaskHandle_t QRCodeReader_Task; 

/* ======================================== Select camera model */
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM
#define CAMERA_MODEL_AI_THINKER

#define LED_PIN 2
#define LED_PIN_DENIED 4
/* ======================================== */

/* ======================================== GPIO of camera models */
#if defined(CAMERA_MODEL_WROVER_KIT)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_ESP_EYE)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    4
  #define SIOD_GPIO_NUM    18
  #define SIOC_GPIO_NUM    23
  
  #define Y9_GPIO_NUM      36
  #define Y8_GPIO_NUM      37
  #define Y7_GPIO_NUM      38
  #define Y6_GPIO_NUM      39
  #define Y5_GPIO_NUM      35
  #define Y4_GPIO_NUM      14
  #define Y3_GPIO_NUM      13
  #define Y2_GPIO_NUM      34
  #define VSYNC_GPIO_NUM   5
  #define HREF_GPIO_NUM    27
  #define PCLK_GPIO_NUM    25

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_M5STACK_WITHOUT_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif
/* ======================================== */

/* ======================================== Variables declaration */
struct QRCodeData
{
  bool valid;
  int dataType;
  uint8_t payload[1024];
  int payloadLen;
};

struct quirc *q = NULL;
uint8_t *image = NULL;  
camera_fb_t * fb = NULL;
struct quirc_code code;
struct quirc_data data;
quirc_decode_error_t err;
struct QRCodeData qrCodeData;  
String QRCodeResult = "";
/* ======================================== */

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Informatica";
const char* password = "informatica453";

const char* serverName = "http://10.20.16.225:8000/codigo/validar";
const char* ledURL = "http://10.20.16.225:8000/qrcode";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

/* ________________________________________________________________________________ VOID SETTUP() */
void setup() {
  // put your setup code here, to run once:

  
  // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  /* ---------------------------------------- Init serial communication speed (baud rate). */
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();

  
  pinMode(LED_PIN, OUTPUT);  // Configura o GPIO 2 como saída
  digitalWrite(LED_PIN, LOW);
  
  pinMode(LED_PIN_DENIED, OUTPUT);  // Configura o GPIO 2 como saída
  digitalWrite(LED_PIN_DENIED, LOW);

  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  /* ---------------------------------------- */

  /* ---------------------------------------- Camera configuration. */
  Serial.println("Start configuring and initializing the camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;
  
  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- create "QRCodeReader_Task" using the xTaskCreatePinnedToCore() function */
  xTaskCreatePinnedToCore(
             QRCodeReader,          /* Task function. */
             "QRCodeReader_Task",   /* name of task. */
             10000,                 /* Stack size of task */
             NULL,                  /* parameter of the task */
             1,                     /* priority of the task */
             &QRCodeReader_Task,    /* Task handle to keep track of created task */
             0);                    /* pin task to core 0 */
  /* ---------------------------------------- */

  

}
/* ________________________________________________________________________________ */
void loop() {
  
  delay(100);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ The function to be executed by "QRCodeReader_Task" */
// This function is to instruct the camera to take or capture a QR Code image, then it is processed and translated into text.
void QRCodeReader( void * pvParameters ){
  /* ---------------------------------------- */
  Serial.println("QRCodeReader is ready.");
  Serial.print("QRCodeReader running on core ");
  Serial.println(xPortGetCoreID());
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Loop to read QR Code in real time. */
  while(1){
      q = quirc_new();
      if (q == NULL){
        Serial.print("can't create quirc object\r\n");  
        continue;
      }
    
      fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.println("Camera capture failed");
        continue;
      }   
      
      quirc_resize(q, fb->width, fb->height);
      image = quirc_begin(q, NULL, NULL);
      memcpy(image, fb->buf, fb->len);
      quirc_end(q);
      
      int count = quirc_count(q);
      if (count > 0) {
        quirc_extract(q, 0, &code);
        err = quirc_decode(&code, &data);
    
        if (err){
          Serial.println("Decoding FAILED");
          QRCodeResult = "Decoding FAILED";
        } else {
          Serial.printf("Decoding successful:\n");
          dumpData(&data);
        } 
        Serial.println();
      } 
      
      esp_camera_fb_return(fb);
      fb = NULL;
      image = NULL;  
      quirc_destroy(q);
  }
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */
void turnLEDAccept() {
    digitalWrite(LED_PIN, HIGH); // Liga o LED
    Serial.println("Ligando LED_PIN (Acesso Permitido)");
    delay(9000);                 // Aguarda 9 segundos
    digitalWrite(LED_PIN, LOW);  // Desliga o LED
    delay(1000);                 // Aguarda 1 segundo
}

void turnLEDDenied() {
    digitalWrite(LED_PIN_DENIED, HIGH); // Liga o LED
    Serial.println("Ligando LED_PIN_DENIED (Acesso Negado)");
    delay(9000);                        // Aguarda 9 segundos
    digitalWrite(LED_PIN_DENIED, LOW);  // Desliga o LED
    delay(1000);                        // Aguarda 1 segundo
}

void sendPutRequest(WiFiClient& client, const String& url, const String& jsonData) {
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int putResponse = http.PUT(jsonData);  // Envia o PUT

    if (putResponse > 0) {
        String responsePut = http.getString();
        Serial.print("Código da requisição: ");
        Serial.println(putResponse);
        Serial.print("Response: ");
        Serial.println(responsePut);
    } else {
        Serial.print("Erro na requisição HTTP: ");
        Serial.println(putResponse);
    }

    http.end();  // Finaliza a conexão HTTP
}
/* ________________________________________________________________________________ Function to display the results of reading the QR Code on the serial monitor. */
void dumpData(const struct quirc_data *data)
{
  Serial.printf("Version: %d\n", data->version);
  Serial.printf("ECC level: %c\n", "MLHQ"[data->ecc_level]);
  Serial.printf("Mask: %d\n", data->mask);
  Serial.printf("Length: %d\n", data->payload_len);
  Serial.printf("Payload: %s\n", data->payload);

  QRCodeResult = (const char *)data->payload;

  if ((millis() - lastTime) > timerDelay) {
        if (WiFi.status() == WL_CONNECTED) {
            WiFiClient client;
            HTTPClient http;

            http.begin(client, serverName);

            String jsonData = "{\"codigo\":\"" + String(QRCodeResult) + "\"}";
            http.addHeader("Content-Type", "application/json");

            int httpResponseCode = http.POST(jsonData); // Envia o JSON

            if (httpResponseCode > 0) {
                // Lê a resposta do servidor
                String response = http.getString();
                Serial.print("Código da requisição: ");
                Serial.println(httpResponseCode);
                Serial.print("Response: ");
                Serial.println(response);

                if (response.indexOf("\"valid\":1") > -1) {
                  sendPutRequest(client, ledURL, "{\"valido\":1}");
                  delay(5000);   
                  sendPutRequest(client, ledURL, "{\"valido\":0}");
                } else{
                  sendPutRequest(client, ledURL, "{\"valido\":2}");
                  delay(5000);   
                  sendPutRequest(client, ledURL, "{\"valido\":0}");
                }

            } else {
                Serial.print("Erro na requisição HTTP: ");
                Serial.println(httpResponseCode);
                sendPutRequest(client, ledURL, "{\"valido\":2}");
                delay(5000);   
                sendPutRequest(client, ledURL, "{\"valido\":0}");

            }
            http.end(); // Finaliza a conexão HTTP
        } else {
            Serial.println("WiFi desconectado!");
        }

        lastTime = millis();
    }
}