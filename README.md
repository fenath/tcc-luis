# Robô carro Godot + ESP32-CAM

Este é o projeto de tcc do luis ... (escrever mais..)

# Estrutura

![integraçao](https://github.com/user-attachments/assets/a492f32e-1874-454a-8559-5999070e27e6)

Há dois servidores, um responsável pelo stream do vídeo e um websocket responsável para os comandos

## Comandos

- flash: liga/desliga o flash da camera
- capture: tira uma foto e envia
- mv_x:+000: muda direção da roda (direita/esquerda) (IMPLEMENTAR)
- mv_y:-000: muda velocidade do carro (frente/ré) (IMPLEMENTAR)

# Roadmap

- [x] mvx, mvy
- [ ] melhora na stream pelo godot (ao acessar pelo navegador (IP:81/stream) a transmissão é muito rápida,
      mas no godot estou usando request no momento, o que torna lento
- [x] implementar PWM digital para poder usar valores variáveis e nao somente discretos para o robo
      (ex: acelerar 50%, virar 80% para esquerda)
- [ ] Montar circuito do carro
