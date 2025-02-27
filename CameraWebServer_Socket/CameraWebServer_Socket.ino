#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#define CONFIG_HTTPD_WS_SUPPORT

#define PWM_FREQ 1
#define MICRO_SECOND 1000000
#define CHUNK_LENGTH 1460

#define LEFT_MOTOR_PIN 0
#define RIGHT_MOTOR_PIN 16

#define CAMERA_MODEL_AI_THINKER // Has PSRAM
// #define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#include "camera_pins.h"
#include <WebSocketsServer.h>


// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "ALHN-7470";//"Fernando's Galaxy A32"; //
const char *password = "x8Kgn5Rj5,";//"fernando.01";//

const char *udpAddress = "192.168.1.255";
const int udpPort = 4242;
WiFiUDP udp;
float fps = 0.0; // calculo de frames por segundo

WebSocketsServer webSocket = WebSocketsServer(4242);

void udpLoop();
void sendPacketData(const char* buf, uint16_t len, uint16_t chunkLength);

void startCameraServer();
void setupLedFlash(int pin);

void write_pwm(int pin, int value);
int extract_number(char * payload);

int flash_status = 0;
int x_speed = 0;
int y_speed = 0;

float right_power(int x_spd);
float left_power(int x_spd);
int motor_spd(float x_spd, float y_spd);
void move_motors();

// Called when receiving any WebSocket message
void onWebSocketEvent(uint8_t num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from ", num);
        Serial.println(ip.toString());
      }
      break;

    // Echo text message back to client
    case WStype_TEXT:
      Serial.printf("[%u] Text: %s\n", num, payload);
      Serial.println((char*)payload);

      if (String((char*)payload) == "capture")
      {
        Serial.println("Capture Command Received - capturing frame");

        camera_fb_t * fb = NULL;
        fb = esp_camera_fb_get(); // get image... part of work-around to get latest image
        esp_camera_fb_return(fb); // return fb... part of work-around to get latest image
        
        fb = NULL;
        fb = esp_camera_fb_get(); // get fresh image
        size_t fbsize = fb->len;
        Serial.println(fbsize);
        Serial.println("Image captured. Returning frame buffer data.");
        webSocket.sendBIN(num, fb->buf, fbsize);
        esp_camera_fb_return(fb);
        Serial.println("Done");
      } else if (String((char*)payload) == "flash")
      {
        Serial.println("Flash command received - Toggling flash");
        if (!flash_status) {
          flash_status = 1;
          digitalWrite(LED_GPIO_NUM, HIGH);
          webSocket.sendTXT(num, "flash ON");
        } else {
          flash_status = 0;
          digitalWrite(LED_GPIO_NUM, LOW);
          webSocket.sendTXT(num, "flash OFF");
        }
      } else if (String((char*)payload) == "command")
      {
        Serial.println("command received");
      } else if (String((char*)payload).startsWith("mvx"))
      {

        // String nr_string = String((char*)payload).substring(4);
        int numero = extract_number((char*)payload);

        //Serial.println("moving x: " + String(numero));
        x_speed = numero;
        move_motors();
      } else if (String((char*)payload).startsWith("mvy"))
      {
        // mvy: nome do comando
        // +,-: sentido do comando
        // 00~99: intensidade do sinal
        // FIXME: actually move y
        int numero = extract_number((char*)payload);
        // Serial.println("moving y: " + String(numero));
        y_speed = numero;
        move_motors();
      }else if (String((char*)payload).startsWith("fps"))
      {
        String fpsStr = String(fps, 2);
        webSocket.sendTXT(num, fpsStr);
      } else
      {
        webSocket.sendTXT(num, payload);
      }
      break;

    // For everything else: do nothing
    case WStype_BIN:
     // Serial.printf("[%u] get binary length: %u\n", num, length);
     // hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
     // break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //startCameraServer();

  Serial.println("trying to flash");
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(500);
  digitalWrite(LED_GPIO_NUM, LOW);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  //pinMode(LEFT_MOTOR_PIN, OUTPUT);
  //pinMode(RIGHT_MOTOR_PIN, OUTPUT);

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  udp.begin(udpPort);
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  webSocket.loop();
  udpLoop();
  
  //Serial.println("x speed: " + String(x_speed) + " | y speed: " + String(y_speed));
}


void write_pwm(int pin, int value){
  static unsigned long last_micros = 0;
  static unsigned long i = 0;
  static unsigned int pin_status = 0;

  // Limitar o duty cycle entre 0% e 100%
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  
  unsigned long now = micros(); 
  unsigned long delta = now - last_micros;
  
  // periodo pwm (em microssegundos)= (10^6 (microssegundos)) / fequencia
  unsigned long period = MICRO_SECOND / PWM_FREQ; 
  int prop = (value) * period / 100;
  i++;
  if (delta >= period) {
    last_micros = now;
    Serial.print(". Reseting after " + String(i) + " counts\n");
    i = 0;
  }

  if (delta < prop)
  {
    if (pin_status == 0){
      pin_status = 1;
      digitalWrite(pin, HIGH);
    }
  } else {
    if (pin_status == 1){
      pin_status = 0;
      digitalWrite(pin, LOW);
    }
  } 
}

int extract_number(char * payload) {
  String nr_string = String(payload).substring(3);
  int numero = nr_string.toInt();
  return numero;
}


void udpLoop() {
  camera_fb_t *fb = NULL;

  float alpha = 0.1; // Fator de suavização de frames

  static unsigned long lastTime = 0;

  if (lastTime == 0) {
    lastTime = millis();
  }

  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    // esp_camera_fb_return(fb);
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("PIXFORMAT_JPEG not implemented");
    esp_camera_fb_return(fb);
    return;
  }

  unsigned long frameTime = millis();
  unsigned long deltaTime = frameTime - lastTime;

  sendPacketData((const char*)fb->buf, fb->len, CHUNK_LENGTH);
  esp_camera_fb_return(fb);

  if (deltaTime > 0){
    fps = alpha * (1000.0 / (frameTime - lastTime)) + (1 - alpha) * fps;
  } else fps = 0;
  
  lastTime = frameTime;

  // Recebendo comandos via UDP
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
      Serial.print("Comando recebido: ");
      Serial.println(incomingPacket);

      if (strncmp(incomingPacket, "power:", 6) == 0) {
        int L = 0;
        int R = 0;

        if (sscanf(incomingPacket, "power:%d:%d", &L, &R) == 2) {
          if (L >= 0 && L <= 100 && R >= 0 && R <= 100) {
            Serial.print("Potencia L: ");
            Serial.print(L);
            Serial.print(", Potência R: ");
            Serial.println(R);
          } else {
            Serial.println("Erro: Os valores de L e R devem estar entre 0 e 100");
          }
        } else {
          Serial.println("Erro: Formato invalido para o comando power");
        }
      } else {
        Serial.println("Erro: Comando inválido");
      }
    }
  }


}

void sendPacketData(const char* buf, uint16_t len, uint16_t chunkLength) {
  uint8_t buffer[chunkLength];
  size_t blen = sizeof(buffer);
  size_t rest = len % blen;

  for (uint8_t i = 0; i < len / blen; ++i) {
    memcpy(buffer, buf + (i * blen), blen);
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer, chunkLength);
    udp.endPacket();
  }

  if (rest) {
    memcpy(buffer, buf + (len - rest), rest);
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer, rest);
    udp.endPacket();
  }
}


float left_power(int x_spd){
  if (x_spd >= 0) return 1.0;
  return 1.0 + float(x_spd)/100.0;
}

float right_power(int x_spd){
  if (x_spd <= 0) return 1.0;
  return 1.0 - float(x_spd)/100.0;
}


int motor_spd(float x_spd, float y_spd){
  float calc = y_spd * x_spd * 100.0;
  return int(calc);
}


void move_motors(){
  float y_spd = float(y_speed) / 100.0;

  int left_motor = motor_spd(left_power(x_speed), y_spd);
  int right_motor = motor_spd(right_power(x_speed), y_spd);
  //Serial.print("yspd: " +String(y_speed)+"| x-speed: "+String(x_speed)+"| right_pwr: "+String(right_power(x_speed))+"| left_pwr: "+String(left_power(x_speed))+"| L: "+String(left_motor) + "| R: " +String(right_motor)+"\n");
  //Serial.print("power:"+String(left_motor)+","+String(right_motor)+"\n");
  // Use snprintf for safe string formatting
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "power:%d,%d\n", left_motor, right_motor);
  Serial.print(buffer);
}