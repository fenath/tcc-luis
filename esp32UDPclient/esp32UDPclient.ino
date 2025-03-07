#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_camera.h>
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"
#define CHUNK_LENGTH 1460
#define CAMERA_FREQ 40 // em HERTZ

// Configurações da rede Wi-Fi
const char *ssid = "ALHN-7470";//"Fernando's Galaxy A32"; //
const char *password = "x8Kgn5Rj5,";//"fernando.01";//

WiFiUDP udp;
char serverIP[16] = "192.168.1.255";  // IP do Godot
const int serverPort = 4242;  // Porta do servidor Godot
const int localPort = 4211;   // Porta local do ESP32
boolean isBroadcasting = true;

// Variables
float fps = 0.0;

void setupCamera();
void sendPacketData(const char* buf, uint16_t len, uint16_t chunkLength);
void sendTextMessage(String message);
void enviarFPS();
void enviarImagem();
void enviarPing();
void enviarPowerArduino(String command);
void enviarBroadcastSignal();
void atualizarIp(const char *newIp);

void setup() {
    setupCamera();

    Serial.begin(115200);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");

    udp.begin(localPort);
    Serial.println("UDP iniciado.");
    isBroadcasting = true;
}

void setupCamera() {
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
  config.frame_size = FRAMESIZE_SVGA; //FRAMESIZE_UXGA;
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

// // Setup LED FLash if LED pin is defined in camera_pins.h
// #if defined(LED_GPIO_NUM)
//   setupLedFlash(LED_GPIO_NUM);
// #endif

}

void loop() {
  // Receber dados do servidor (Godot)
  char packet[255];
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packet, 255);
    packet[packetSize] = '\0';

    // Processar comandos do servidor
    if (strcmp(packet, "fps") == 0) {
      enviarFPS();
    } else if (strncmp(packet, "ping", 4) == 0) {
      enviarPing(packet);
    } else if (strncmp(packet, "myip:", 5) == 0) {    
      IPAddress ip = udp.remoteIP();
      atualizarIp(ip.toString().c_str());
    } else if (strncmp(packet, "power:", 6) == 0) {
      enviarPowerArduino(packet);
    }
  }

  // Enviar imagem periodicamente
  static unsigned long lastImageTime = 0;
  static unsigned long lastBroadcast = 0;
  if (millis() - lastImageTime > 1000 / CAMERA_FREQ) {
    if (isBroadcasting) {
      enviarBroadcastSignal();
    } else {
      enviarImagem();
      lastImageTime = millis();
    }
  }

  if (millis() - lastBroadcast > 1000) {
    lastBroadcast = millis();
    // A cada segundo manda um sinal a todas as faixas
    enviarBroadcastSignal(); 
  }
}

void enviarPowerArduino(String command){
  int x = 0, y = 0;

  if (sscanf(command.c_str(), "power:%d:%d", &x, &y) == 2) {
    Serial.print("X: "); Serial.println(x);
    Serial.print("Y: "); Serial.println(y);
  } else {
    Serial.println("Erro ao processar comando!");
    return;
  }

  if (x > 100 || x < -100) {
    Serial.printf("valor 1 de power (%d )está incorreto\n", x);
  }

  if (y > 100 || y < -100) {
    Serial.printf("valor 2 de power (%d) está incorreto\n", y);
  }

  Serial.printf("power:%d:%d\n", x, y);
}

void enviarBroadcastSignal() {
  char msg[30] = "esp_ping";
  sendTextMessage(msg);
}

void atualizarIp(const char* newIp) {

  if (strcmp(serverIP, newIp) == 0){
    return;
  }

  strncpy(serverIP, newIp, 15);    // Copia o novo IP (máximo de 15 caracteres)
  serverIP[15] = '\0';             // Garante terminação correta da string

  Serial.print("IP atualizado para: ");
  Serial.println(serverIP);
  isBroadcasting = false;
}

void enviarFPS() {
  char msg[30];
  snprintf(msg, sizeof(msg), "fps:%.2f", fps); 
  sendTextMessage(msg);
  Serial.println("FPS enviado.");
}

void enviarPing(String command) {
  char msg[30];
  //TODO: mudar para o "id" do ping
  snprintf(msg, sizeof(msg), "pong:%d:%s\n", millis(), command); 
  sendTextMessage(msg);
  Serial.println("Ping enviado.");
}

void enviarImagem() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Erro ao capturar imagem!");
        return;
    }

    sendPacketData((const char*)fb->buf, fb->len, CHUNK_LENGTH);
    esp_camera_fb_return(fb);

    float alpha = 0.1;
    static unsigned long lastTime = millis();
    unsigned long frameTime = millis();
    unsigned long deltaTime = frameTime - lastTime;
    if (deltaTime > 0) {
      fps = alpha * (1000.0 / deltaTime) + (1 - alpha) * fps;
    } else fps = 0.0;
    lastTime = frameTime;
}

void sendPacketData(const char* buf, uint16_t len, uint16_t chunkLength) {
  // ID único para cada imagem -> Evitar pacotes misturados
  static uint16_t imageID = 0;  
  
  // o id ao cabecalho e o tipo de dado enviado (0x49 = 'I' para tipar como imagem)
  size_t payloadSize = chunkLength - 5;
  size_t fullPackets = len / payloadSize;
  size_t lastPacketSize = len % payloadSize;

  // Criando buffer dinamicamente
  // O buffer vai ser 5 bytes maior que a imagem original para adicionar 
  uint8_t* buffer = (uint8_t*)malloc(payloadSize + 5);
  if (!buffer) {
    Serial.println("Erro: Falha ao alocar buffer!");
    return;
  }

  imageID++;

  for (uint16_t i = 0; i < fullPackets; ++i) {

    // Criacao do cabecalho
    buffer[0] = 0x49;
    buffer[1] = (imageID >> 8) & 0xFF;
    buffer[2] = imageID & 0xFF;
    buffer[3] = (i >> 8) & 0xFF;
    buffer[4] = i & 0xFF;

    memcpy(buffer + 5, buf + (i * payloadSize), payloadSize);

    udp.beginPacket(serverIP, serverPort);
    udp.write(buffer, payloadSize + 5);
    udp.endPacket();

    // Debug: Imprimir informações do pacote
    // Serial.printf("Enviando pacote %d/%d - ImageID: %d\n", 
    //               i+1, fullPackets + (lastPacketSize > 0 ? 1 : 0), imageID);
  }

  if (lastPacketSize > 0) {
    
    // Criacao do cabecalho
    buffer[0] = 0x49;
    buffer[1] = (imageID >> 8) & 0xFF;
    buffer[2] = imageID & 0xFF;
    buffer[3] = (fullPackets >> 8) & 0xFF;
    buffer[4] = fullPackets & 0xFF;

    memcpy(buffer + 5, buf + (fullPackets * payloadSize), lastPacketSize);

    udp.beginPacket(serverIP, serverPort);
    udp.write(buffer, lastPacketSize + 5);
    udp.endPacket();

    // Debug
    // Serial.printf("Enviando último pacote - Tamanho: %d\n", lastPacketSize);
  }

  // Debug: Informações gerais
  // Serial.printf("Imagem total: %d bytes, Pacotes: %d + %d resto\n", 
  //               len, fullPackets, lastPacketSize > 0 ? 1 : 0);

  free(buffer);
}

void sendTextMessage(String message) {
  uint8_t buffer[message.length() + 1];
  buffer[0] = 0x54; // 'T' - indicando texto

  memcpy(buffer + 1, message.c_str(), message.length());

  udp.beginPacket(serverIP, serverPort);
  udp.write(buffer,  message.length() + 1);
  udp.endPacket();
}