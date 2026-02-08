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

### Atualizando `.env`
- O arquivo [CMakeLists.txt](CMakeLists.txt) está configurado para monitorar `.env` e reconfigurar automaticamente o CMake quando você recompila. Em builds incrementais, basta rodar a tarefa "Compile Project" e as mudanças de `.env` entram.
- Se você apagou a pasta `build/`, é necessário configurar novamente antes de compilar. Use:
```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```
- Caso perceba que os logs ainda não refletiram o novo `.env`, force uma reconfiguração: apague `build/` e gere/compile novamente com os comandos acima.

### Como o `.env` é aplicado (por que não “puxa” em tempo de execução)
- O `.env` é lido em tempo de configuração do CMake (quando os arquivos de build são gerados). Os valores viram definições de pré‑processador (`WIFI_SSID`, `WIFI_PASSWORD`, `TEMP_THRESHOLD_C`, `DIST_THRESHOLD_MM`) embutidas no binário.
- Por isso, alterar `.env` não muda o comportamento “ao vivo”; é necessário reconfigurar e recompilar para que o novo binário inclua os valores atualizados.
- Em builds incrementais, o projeto monitora `.env` e reconfigura automaticamente quando você executa "Compile Project" com a pasta `build/` existente.
- Se `build/` foi removida, primeiro configure (CMake) e só então compile; a task "Compile Project" por si só pode falhar se não houver a configuração inicial.

### Fluxo recomendado
- Editar `.env`.
- Se `build/` existe: executar a task "Compile Project" e depois "Run Project".
- Se `build/` não existe ou algo não atualizou: executar no terminal
```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```
e então usar a task "Run Project" para carregar o binário.

### Observação sobre `TEMP_THRESHOLD_C`
- O regex de leitura no [CMakeLists.txt](CMakeLists.txt) foi ajustado para evitar capturar comentários acidentalmente. Se `TEMP_THRESHOLD_C` não aparecer nos logs, reconfigure e compile novamente.

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
- Em seguida, gere os arquivos de build e compile. Você pode usar o menu/atalho do CMake (Configure) ou executar no terminal:
```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```

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
 
