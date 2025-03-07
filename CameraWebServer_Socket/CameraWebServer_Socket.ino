#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"
#define CHUNK_LENGTH 1460


// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "ALHN-7470";//"Fernando's Galaxy A32"; //
const char *password = "x8Kgn5Rj5,";//"fernando.01";//

const char *udpAddress = "192.168.1.255";
const int udpPort = 4242;
const int clientPort = 4241;
WiFiUDP udp;
char packetBuffer[255]; // Buffer que armazena mensagens recebidas via UDP
float fps = 0.0; // calculo de frames por segundo

void udpLoop();
void receiveUDPCommands();
void broadcastImageUDP();
void processUDPCommand(String command);
void sendPacketData(const char* buf, uint16_t len, uint16_t chunkLength);

int extract_number(char * payload);

int flash_status = 0;
int x_speed = 0;
int y_speed = 0;

float right_power(int x_spd);
float left_power(int x_spd);
int motor_spd(float x_spd, float y_spd);
void move_motors();

void setupLedFlash(int pin);

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
  config.frame_size = FRAMESIZE_UXGA; //FRAMESIZE_SVGA;
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

  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(500);
  digitalWrite(LED_GPIO_NUM, LOW);

  udp.begin(clientPort);
  Serial.println("Cliente udp iniciado!");
}

void loop() {
  udpLoop();
}

int extract_number(char * payload) {
  String nr_string = String(payload).substring(3);
  int numero = nr_string.toInt();
  return numero;
}

void udpLoop() {
  
  broadcastImageUDP();
  // Recebendo comandos via UDP
  receiveUDPCommands();

}

void broadcastImageUDP() {

  static unsigned long lastImageTime = 0;

  // setting frequence
  if (millis() - lastImageTime <= 1000 / 60) {
    return;
  }

  lastImageTime = millis();

  camera_fb_t *fb = NULL;

  float alpha = 0.1; // Fator de suavização de frames

  static unsigned long lastTime = 0;

  if (lastTime == 0) {
    lastTime = millis();
  }

  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
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
}

void receiveUDPCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    processUDPCommand(packetBuffer);
  }
}

void processUDPCommand(String command){
  // TODO: Processar comandos que
  // - 'fps': Retorna o valor de fps calculado aqui

  Serial.println(" -------> Comando ----> " + command);

  if (command == "fps") {
    String response = "fps:" + String(fps, 2);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print(response);
    udp.endPacket();
    Serial.println(response);
    return;
  }

  // - 'power:x:y': Envia este comando para serial, com os valores para cada eixo
  if (command.startsWith("power")) {
    
    int firstColon = command.indexOf(':');
    int secondColon = command.indexOf(':', firstColon + 1);
    
    if (secondColon > firstColon) {
      String powerStr1 = command.substring(firstColon + 1, secondColon);
      String powerStr2 = command.substring(secondColon + 1);

      int powerLevel1 = powerStr1.toInt();
      int powerLevel2 = powerStr2.toInt();

      if (powerLevel1 < 0 || powerLevel1 > 100 || powerLevel2 < 0 || powerLevel2 > 100) {
        Serial.print("Niveis de power precisam estar entre 0 e 100\n");
        return;
      }
      Serial.printf("Níveis de potência atualizados: %d e %d\n", powerLevel1, powerLevel2);
    } else {
      Serial.printf("Comando power enviado incorretamente (%s) \n", command);
    }
    return;
  }

  Serial.printf("Comando não conhecido: %s\n", command);

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
  // Use snprintf for safe string formatting
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "power:%d,%d\n", left_motor, right_motor);
  Serial.print(buffer);
}