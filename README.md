# Sistema de Monitoramento Ambiental IoT (Pico W + FreeRTOS)

Projeto embarcado com Raspberry Pi Pico W (RP2040), FreeRTOS, sensores BMP280 (temperatura/pressão) e VL53L0X/VL53L1X (distância), display OLED SSD1306 e publicação MQTT via Wi‑Fi. Organização em tarefas FreeRTOS com fila para acoplamento entre aquisição e comunicação.

## Visão Geral
- Sensores: BMP280 (I2C0), VL53L0X/VL53L1X (I2C1)
- Display: SSD1306 (I2C1), exibe FRIO/QUENTE em tela cheia conforme `TEMP_THRESHOLD_C`
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

### Atalho: copiar modelo para `.env`
Se existir um arquivo de exemplo (por exemplo, `.env.example`), você pode copiá-lo rapidamente para `.env` no PowerShell:
```powershell
Copy-Item .env.example .env
```

Depois da cópia, abra o arquivo `.env` e substitua as variáveis de rede pelos seus valores reais:
- `WIFI_SSID`: o nome (SSID) da sua rede Wi‑Fi.
- `WIFI_PASSWORD`: a senha da sua rede Wi‑Fi.

Opcionalmente, ajuste os limiares conforme necessário:
- `TEMP_THRESHOLD_C`: temperatura em °C para trocar a imagem do display.
- `DIST_THRESHOLD_MM`: distância em milímetros para acionar lógica associada.

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

## Limpar e reconstruir o build
Quando o build quebrar ou você mudar configurações, faça uma recompilação limpa.

Opção 1 — VS Code (recomendado):
- Feche qualquer tarefa de build em execução.
- Exclua a pasta `build/`.
- Rode a tarefa "Compile Project" novamente (ela recria `build/` e compila).

Opção 2 — Terminal (PowerShell):
```powershell
# Na raiz do projeto
# Apagar completamente a pasta de build
Remove-Item -Recurse -Force build

# Regenerar e compilar (exemplo genérico)
cmake -S . -B build
cmake --build build

# Alternativa com Ninja (se preferir)
# cmake -S . -B build -G "Ninja"
# ninja -C build
```
Notas:
- `build/` já está ignorado pelo Git via [.gitignore](.gitignore).
- Se tarefas como "Run Project"/"Flash" usarem arquivos de `build/`, execute o rebuild antes de usá-las.

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

## Exibição e Sinalização
- Display SSD1306: renderiza a palavra **QUENTE** ou **FRIO** em fonte escalada e centralizada.
- LED RGB: QUENTE ativa vermelho (`LED_PIN_R`), FRIO ativa azul (`LED_PIN_B`).
 
## Segurança
- Credenciais (SSID/senha) e limiares ficam em `.env` e **não** são versionados.

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
 
