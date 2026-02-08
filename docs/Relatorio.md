Sistema de Monitoramento Ambiental IoT com RP2040 (Pico W) e FreeRTOS

Título
Sistema de Monitoramento Ambiental IoT com Raspberry Pi Pico W, FreeRTOS e sensores BMP280 + VL53L0X/L1X, com visualização em SSD1306 e publicação MQTT.

Introdução
Este relatório apresenta o desenvolvimento de um sistema embarcado para monitoramento ambiental baseado no microcontrolador RP2040 (Raspberry Pi Pico W), integrando sensores de temperatura/pressão (BMP280) e de distância (VL53L0X/VL53L1X), um display OLED SSD1306 e conectividade Wi‑Fi para envio de dados em tempo real via MQTT. O firmware utiliza FreeRTOS para organização das tarefas concorrentes e a pilha lwIP para rede.

O projeto segue a orientação do documento "PROJETO FINAL RESIDÊNCIA ENBARCATECH" e foi implementado com foco em modularidade, confiabilidade e reprodutibilidade, explorando boas práticas de desenvolvimento embarcado.

Objetivos
Os objetivos do projeto são medir temperatura e pressão com o BMP280 e distância com sensores ToF (VL53L0X/VL53L1X), exibir estado e alertas no display SSD1306 por meio de ícones de fogo/gelo conforme limiares definidos, publicar em tempo real as leituras no broker MQTT público (test.mosquitto.org) no tópico `pico_w/sensor`, receber comandos simples via MQTT no tópico `pico_w/recv` (por exemplo, acender/apagar o LED por PWM), parametrizar limiares e credenciais por meio do arquivo `.env` (como `TEMP_THRESHOLD_C`, `DIST_THRESHOLD_MM`, `WIFI_SSID`, `WIFI_PASSWORD`) e organizar o firmware em tarefas FreeRTOS com uso de filas para desacoplar aquisição, visualização e comunicação.

Justificativa
Monitoramento ambiental distribuído é crítico em aplicações de IoT (laboratórios, salas de servidores, estufas, ambientes educacionais). A plataforma Pico W oferece baixo custo, Wi‑Fi integrado e bom ecossistema. FreeRTOS melhora a previsibilidade e organização de tarefas, enquanto MQTT facilita integração com aplicações web, dashboards e serviços de nuvem.

Problemática
Integração de múltiplos periféricos I2C em diferentes buses/pinos e necessidade de alternância segura entre display e sensor ToF no I2C1 (evitando conflito de pull‑ups).
Conexão Wi‑Fi robusta e publicação confiável em MQTT, incluindo resolução DNS e reconexões.
Debounce de botões e controle PWM de LED sem bloquear o agendamento das tarefas.
Parametrização de limiares para alertas sem recompilar (via `.env`).
Tratamento de variações de disponibilidade de sensores (fallback entre VL53L0X e VL53L1X).

Arquitetura do Sistema (Hardware + Firmware + IoT)
Hardware:
  Placa: Raspberry Pi Pico W (RP2040 + CYW43 para Wi‑Fi).
  Sensores: BMP280 (I2C0, GP0/GP1), VL53L0X ou VL53L1X (I2C1, GP2/GP3).
  Display: SSD1306 (I2C1, GP14/GP15).
  Atuadores: LED RGB via PWM (GP11/12/13).
  Botão de controle: GP6 (pausar/retomar tarefas).
Firmware:
  FreeRTOS: tarefas `tarefaSensorBMP280`, `tarefaMQTT` e fila `filaMQTT` (`DadosSensor`).
  Drivers: `bmp280`, `vl53l0x`, `vl53l1x`, `ssd1306`, PWM/ADC/I2C do SDK.
  Config: `.env` lido no `CMakeLists.txt` para `WIFI_SSID`, `WIFI_PASSWORD`, `TEMP_THRESHOLD_C`, `DIST_THRESHOLD_MM`.
  Controle Dinâmico: Implementação de controle via `BUTTON6_PIN`, utilizando as funções `vTaskSuspend` e `vTaskResume` para gerenciar o estado das tarefas em tempo real.
IoT:
  Conectividade: Wi‑Fi (CYW43) + lwIP.
  Protocolo: MQTT (cliente lwIP), publicação em `pico_w/sensor`, inscrição em `pico_w/recv`.
  Broker: `test.mosquitto.org:1883` (sem TLS por padrão, `MQTT_TLS=0`).
  Sincronização: fila `filaMQTT` para troca de dados entre sensores e rede.
  Segurança: parametrização de credenciais via `.env`.

Diagrama de Blocos
```mermaid
flowchart LR
    subgraph Hardware
        PICO[Pico W (RP2040 + CYW43)]
        BMP[BMP280\n(I2C0 GP0/GP1)]
        TOF[(VL53L0X/VL53L1X)\n(I2C1 GP2/GP3)]
        OLED[SSD1306\n(I2C1 GP14/GP15)]
        LED[LED RGB PWM\n(GP11/12/13)]
    end

    subgraph Firmware
        RTOS[FreeRTOS]
        T2[tarefaSensorBMP280]
        T3[tarefaMQTT]
        Q[filaMQTT\n(DadosSensor)]
        DRV[Drivers I2C/PWM/ADC]
    end

    subgraph IoT
        WIFI[Wi‑Fi CYW43]
        LWIP[lwIP]
        MQTT[Cliente MQTT]
        BROKER[(test.mosquitto.org)]
    end

    BMP -I2C0 --> PICO
    TOF -I2C1 --> PICO
    OLED -I2C1 --> PICO
    LED -PWM --> PICO

    PICO <-.-> RTOS
    RTOS --> T2
    RTOS --> T3
    T2 --> Q
    Q --> T3
    T2 --> OLED

    PICO --> WIFI --> LWIP --> MQTT --> BROKER
    BROKER --> MQTT --> T3
```

Desenvolvimento do Sistema
Inicialização de periféricos: configuração de I2C0 (BMP280) e I2C1 com funções de alternância segura (`i2c1_use_display_pins` e `i2c1_use_sensor_pins`). Varredura de endereços (`i2c_scan_bus`) para diagnóstico.
Sensores ToF: tentativa de `VL53L0X` e fallback para `VL53L1X` com modo contínuo e verificação de `data_ready`.
BMP280: leitura agregada `bmp280_get_all(I2C_ADDR)` e montagem de `DadosSensor`.
Display: limpeza, renderização de bitmaps “fogo”/“gelo” conforme `TEMP_THRESHOLD_C`, atualização via `ssd1306_update()`.
FreeRTOS:
  `tarefaSensorBMP280`: aquisição periódica, visualização e envio para `filaMQTT`.
  `tarefaMQTT`: conexão Wi‑Fi, resolução DNS, conexão MQTT, publicação de payload JSON no tópico `pico_w/sensor`, callback de assinatura para comandos ("acender"/"apagar").
Parametrização: leitura de `.env` no CMake e definição de macros para uso no código sem recompilação de credenciais.

Descrição dos Módulos, Tarefas, Sensores e Protocolos
Módulos (inc/):
  `bmp280.[ch]`: driver de temperatura/pressão.
  `vl53l0x.[ch]` e `vl53l1x.[ch]`: drivers de sensor de distância ToF.
  `ssd1306.[ch]`: driver de display OLED.
  `max30101.[ch]`: driver incluído no build (não utilizado no firmware atual).
Tarefas (FreeRTOS):
  `tarefaSensorBMP280`: coleta BMP280 + ToF, atualiza display, envia `DadosSensor` à fila.
  `tarefaMQTT`: gerencia Wi‑Fi/MQTT e publica payloads recebidos da fila.
Estruturas:
  `DadosSensor`: `{ temperatura (float), pressao (uint32_t), distancia_mm (uint16_t) }`.
  Fila: `filaMQTT` (`xQueueCreate(5, sizeof(DadosSensor))`).
Protocolos:
  I2C (100 kHz sensores / 400 kHz display), PWM para LED.
  MQTT: publicação tópicos `pico_w/sensor`, assinatura `pico_w/recv`.

Evidências de Funcionamento
Logs típicos (serial USB):
  `[Wi‑Fi] Conectando a <SSID>...`
  `[DNS] Resolvendo test.mosquitto.org...`
  `[MQTT] Conectado ao Broker!`
  `[VL53L0X] Distância: 345 mm` ou `[VL53L1X] Distância: 350 mm | status=0x00 | stream=42`
  `[MQTT] Enviado: {"temp_c": 29.8, "press_pa": 100560, "dist_mm": 350}`
Display: alterna entre ícones “fogo/gelo” conforme `TEMP_THRESHOLD_C`.
Controle: comandos MQTT "acender"/"apagar" ajustam brilho do canal.

Como capturar logs
1. Conecte a Pico W via USB. 
2. Compile e carregue o projeto pela tarefa do VS Code.
3. Abra um terminal serial (stdout USB habilitado). Os `printf` serão exibidos.

Execução rápida (VS Code Tasks)
Build: "Compile Project".
Load UF2/ELF: "Run Project".
Flash por debug (CMSIS‑DAP): "Flash".

Prints, Fotos, Logs de Comunicação
Adicione capturas de tela do terminal e fotos do hardware à pasta `docs/images/`.
Sugestões:
  Foto do setup com Pico W, sensores e display ligados.
  Print do broker MQTT (ex.: `mosquitto_sub -h test.mosquitto.org -t pico_w/sensor`).
  Print do display com ícones “fogo”/“gelo”.

Vídeo (5 a 7 minutos) – Roteiro sugerido
Contexto e objetivos (30–45s).
Hardware e diagrama de blocos (60–90s).
Demonstração: leitura de sensores + display (90–120s).
Conectividade: Wi‑Fi, MQTT (90–120s), mostrar mensagens no broker.
Interações: comando MQTT (60–90s).
Resultados, desafios e próximos passos (30–60s).

Conclusão
O sistema integra múltiplos sensores à Pico W com FreeRTOS, disponibilizando dados via MQTT e interface local com display. A arquitetura modular facilita manutenção e expansão (ex.: inclusão do MAX30101, dashboards em nuvem, persistência). A parametrização por `.env` torna a solução adaptável a diferentes ambientes. Como próximos passos, recomenda‑se adicionar TLS ao MQTT, watchdogs e testes automatizados de integração.

Referências
Raspberry Pi Pico SDK: https://github.com/raspberrypi/pico-sdk
FreeRTOS: https://www.freertos.org/
lwIP: https://savannah.nongnu.org/projects/lwip/
MQTT (mosquitto): https://mosquitto.org/
BMP280 datasheet: https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp280/
VL53L0X/VL53L1X: https://www.st.com/en/imaging-and-photonics-solutions/time-of-flight-sensors.html
SSD1306: documentação de drivers e especificações de OLED.

Apêndices
A. Estrutura de Payload MQTT
Exemplo de JSON publicado:
```json
{"temp_c": 29.8, "press_pa": 100560, "dist_mm": 350}
```

B. `.env` de exemplo
```
WIFI_SSID=MinhaRede
WIFI_PASSWORD=SenhaSecreta
TEMP_THRESHOLD_C=30.0
DIST_THRESHOLD_MM=200
```

C. Tópicos MQTT
Publicação: `pico_w/sensor`
Assinatura (comandos): `pico_w/recv` (mensagens: "acender"/"apagar").

D. Observações de I2C
`i2c1_use_display_pins()` vs `i2c1_use_sensor_pins()` evitam conflito no barramento compartilhado.
Velocidades: 400 kHz para display, 100 kHz para sensores.

E. Build & Flash
Via VS Code tasks: Compile/Run/Flash.
Saída adicional: UF2/ELF gerados em `build/`.
