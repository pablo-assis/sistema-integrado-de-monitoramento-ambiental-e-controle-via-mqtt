# Sistema de Monitoramento Ambiental IoT (Pico W + FreeRTOS)

Projeto embarcado com Raspberry Pi Pico W (RP2040), FreeRTOS, sensores BMP280 (temperatura/pressão) e VL53L0X/VL53L1X (distância), display OLED SSD1306 e publicação MQTT via Wi‑Fi. Organização em tarefas FreeRTOS com fila para acoplamento entre aquisição e comunicação.

## Visão Geral
- Sensores: BMP280 (I2C0), VL53L0X/VL53L1X (I2C1)
- Display: SSD1306 (I2C1)
- IoT: Wi‑Fi (CYW43) + lwIP + MQTT
- Tarefas: `tarefaSensorBMP280` (aquisição/visualização) e `tarefaMQTT` (rede/MQTT)
- Fila: `filaMQTT` com estrutura `DadosSensor { temp_c, press_pa, dist_mm }`
- Parametrização: credenciais e limiares via `.env` (sem commit)

Veja detalhes no relatório em [docs/Relatorio.md](docs/Relatorio.md).

## Hardware
- Placa: Raspberry Pi Pico W
- Conexões sugeridas:
  - BMP280: I2C0 nos pinos GP0 (SDA) / GP1 (SCL)
  - VL53L0X/VL53L1X: I2C1 nos pinos GP2 (SDA) / GP3 (SCL)
  - SSD1306: I2C1 nos pinos GP14 (SDA) / GP15 (SCL)
  - LED RGB via PWM: GP11/12/13
  - Botão de controle: GP6

## Pré‑requisitos
- VS Code com extensões C/C++ e CMake
- Pico SDK configurado (o projeto já inclui integração via `pico_sdk_import.cmake`)
- Toolchain CMake/Ninja (usada pelas tarefas do VS Code)
- Acesso a Internet para MQTT em `test.mosquitto.org:1883`

## Configuração (`.env`)
Crie um arquivo `.env` na raiz do projeto com, por exemplo:
```
WIFI_SSID=MinhaRede
WIFI_PASSWORD=MinhaSenha
TEMP_THRESHOLD_C=30.0
DIST_THRESHOLD_MM=200
```
- O `.env` é lido no `CMakeLists.txt` para definir macros usadas no firmware.
- O `.env` está ignorado pelo Git (veja [.gitignore](.gitignore)).

## Build e Execução (VS Code Tasks)
- Compilar: tarefa "Compile Project"
- Carregar UF2/ELF: tarefa "Run Project"
- Flash por debug (CMSIS‑DAP): tarefa "Flash"

Alternativamente, via terminal (se desejado):
```powershell
# Na pasta do projeto
# Geração/compilação (exemplo genérico; prefira as tasks do VS Code)
# cmake -S . -B build
# cmake --build build
```

## MQTT
- Publicação: tópico `pico_w/sensor` com payload JSON, exemplo:
```json
{"temp_c": 29.8, "press_pa": 100560, "dist_mm": 350}
```
- Assinatura: tópico `pico_w/recv` para comandos simples ("acender"/"apagar").
- Testes rápidos no host:
```bash
mosquitto_sub -h test.mosquitto.org -t pico_w/sensor
mosquitto_pub -h test.mosquitto.org -t pico_w/recv -m "acender"
```

## Segurança
- Credenciais (SSID/senha) e limiares ficam em `.env` e **não** são versionados.
- Recomenda‑se adotar TLS futuramente (`MQTT_TLS=1`) e watchdogs.

## Estrutura do Projeto (resumo)
- Raiz:
  - [blink.c](blink.c) (exemplo/entrada de firmware)
  - [CMakeLists.txt](CMakeLists.txt)
  - [inc/](inc/) drivers (`bmp280`, `vl53l0x`, `vl53l1x`, `ssd1306`)
  - [FreeRTOS-LTS/](FreeRTOS-LTS/) dependências
  - [docs/Relatorio.md](docs/Relatorio.md) documentação
  - [pico_sdk_import.cmake](pico_sdk_import.cmake) integração Pico SDK
  - [build/](build/) artefatos gerados (ignorado pelo Git)

## Logs e Diagnóstico
- Conecte via USB e abra o terminal serial (stdout habilitado).
- Exemplos de logs:
  - `[Wi‑Fi] Conectando a <SSID>...`
  - `[MQTT] Conectado ao Broker!`
  - `[VL53L1X] Distância: 350 mm | status=0x00`
  - `[MQTT] Enviado: { ... }`

## Como Publicar no GitHub
1. Crie o repositório em sua conta sem README/.gitignore/licença.
2. Configure o remoto (já definido para `origin`).
3. Faça o push:
```powershell
git push -u origin main
```
Se for solicitado, autentique via Git Credential Manager (HTTPS) ou use um Token (PAT).

## Próximos Passos
- Habilitar TLS para MQTT
- Adicionar testes automatizados e watchdog
- Expandir visualização no display
