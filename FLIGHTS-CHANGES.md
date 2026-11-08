# ATS Mini v2.45 + Flights (api.adsb.lol) — correcoes

## Causa do HTTP 426
`Flights.cpp` chamava `http.useHTTP10(true)`. Com isso o HTTPClient envia `GET ... HTTP/1.0`
(e omite `Accept-Encoding`). HTTP 426 = "Upgrade Required": o servidor exige HTTP/1.1 ou superior.
Causa mais provavel, deduzida do codigo do core ESP32 3.3.12 — NAO foi testada contra o servidor
real (o ambiente de build nao alcanca api.adsb.lol).
O `useHTTP10(true)` existe no codigo OTA oficial porque o GitHub aceita HTTP/1.0; nao serve para qualquer API.

## O que mudou
Flights.cpp / Flights.h
- Pedido em HTTP/1.1 (sem useHTTP10). Como HTTP/1.1 pode vir em "chunked", o corpo e lido com
  `HTTPClient::writeToStream()` (remove o chunk framing) para um buffer limitado (192 KB na PSRAM,
  48 KB no heap se nao houver PSRAM) e o JSON e lido da memoria, com filtro.
- TLS verificado com o mesmo CA bundle do OTA (antes: setInsecure). Exige hora valida (NTP), mesma
  regra do OTA; sem hora, a tela mostra "Sem hora (NTP)".
- `addHeader("Accept")` agora vem DEPOIS de `begin()` (begin() apaga os cabecalhos; antes o header nunca era enviado).
- User-Agent oficial do radio (`getUserAgent()`), `Connection: close`, redirecionamentos seguidos.
- Mantem as FLIGHTS_MAX aeronaves MAIS PROXIMAS (antes: as 40 primeiras da resposta, depois ordenadas).
- Concorrencia: lista, status e versao protegidos pelo mutex; o status era um char[] lido/escrito sem protecao.
- Task com 16 KB de stack (TLS); `flightsSetLocation` so grava na NVS se algo mudou (antes gravava a cada boot).
- Logs `[FLIGHT]` na serial so com `FLIGHTS_DEBUG 1` (a serial USB tambem carrega o protocolo de controle remoto).

Integracao
- Removido o desvio do `loop()`: enquanto a tela estava aberta, o loop retornava cedo e parava
  controle remoto (USB/BLE/TCP), web/Wi-Fi/NTP (`webTick`, `netTickTime`), salvamento de preferencias,
  timers e leitura do encoder (as rotacoes acumulavam e "pulavam" a frequencia ao sair).
- A tela agora e um comando normal (`CMD_FLIGHTS`, como About): o loop segue completo, o clique do
  encoder sai (tratamento padrao), nao ha timeout de inatividade nessa tela e o redesenho ocorre
  quando chegam dados e 1x por segundo.

Arquivos tocados vs v2.45 original: Draw.cpp, Menu.cpp, Menu.h, ats-mini.ino, sketch.yaml (ArduinoJson 7.4.2),
Flights.cpp, Flights.h. SI4732/Rotary/Display/menus originais: inalterados.

## Uso
Settings > Flights (ultimo item). Wi-Fi conectado e hora sincronizada. Clique no encoder para sair.
Posicao/raio padrao: -23.2237, -45.9009, 40 nm (chamada em `setup()`).

## r2 — rolagem da lista e novo visual
- Girar o encoder move a selecao na lista (ate 24 aeronaves mais proximas; o contador mostra o total no raio).
  A selecao acompanha o mesmo aviao (ICAO hex) entre as atualizacoes de 15 s; o clique ainda sai da tela.
- Radar: varredura animada com rastro, aneis de distancia, pontos cardeais (N/L/S/O), aeronaves como setas
  orientadas pelo rumo e coloridas por altitude, ground como ponto cinza, selecionada com anel e indicativo.
- Painel: linha selecionada destacada, barra de rolagem, quadro de detalhes (distancia, direcao, velocidade,
  rumo, altitude) e legenda das cores de altitude.
- Redesenho ~8x/s so enquanto a tela de voos esta aberta (animacao da varredura).

## r3 — icone de aviao
- Os pontos/setas foram trocados por uma silhueta de aviao (nariz, fuselagem, asas enflechadas e cauda),
  girada pelo rumo e colorida pela altitude. O selecionado e maior, com anel.
- Na lista, cada linha mostra um aviao miniatura girando pelo rumo no lugar do quadradinho de cor.
- Aeronaves no solo continuam como um pequeno quadrado cinza.

## r4 — menu principal, clima, controle web, local por ICAO
Menu
- "Flights" e "Weather" agora ficam no menu principal, logo antes de "Settings" (saiu de Settings).
- Girar o encoder troca de pagina no clima (Agora / Proximos dias / Proximas horas); clique sai.

Clima (Weather.cpp, WeatherUI.h)
- Fonte: Open-Meteo (https://api.open-meteo.com/v1/forecast), sem chave. Atual + 6 dias + proximas 12 horas.
- Atualiza ao abrir (se os dados tem mais de 5 min) e a cada 10 min com a tela aberta.
- Interface: degrade de ceu conforme a condicao e dia/noite, icones animados (sol com raios girando,
  lua e estrelas, nuvens se movendo, chuva, neve, neblina, raio), temperatura grande, cartoes de
  umidade/vento (com seta)/chuva/pressao, previsao de 5 dias e grafico das proximas horas com barras de chuva.
- WeatherUI.h e um template: o mesmo codigo de desenho foi renderizado num preview no PC para conferir o visual.

Local (Location.cpp, Airports.h)
- Um unico local para Flights e Weather, gravado na NVS ("location"). Padrao: Sao Jose dos Campos, rotulo SJK, 40 nm.
- Pagina web /location: ICAO do aeroporto (tabela embutida de 6907 aeroportos: todo o Brasil com ICAO +
  todos os large/medium do mundo, fonte OurAirports) ou latitude/longitude manual, nome, rotulo, raio.
- O rotulo (SJK) aparece ao lado da sua posicao no radar de voos.
- Campo para a chave CARTO (fica so no aparelho; nunca aparece no codigo nem volta na pagina). Ainda nao e usada: o mapa vem depois.
- Removida a chamada flightsSetLocation() do setup(): ela sobrescreveria o local escolhido na web a cada boot.

Controle web (Web.cpp, PageControl.h)
- /control: frequencia (passos -10/-1/+1/+10 e digitar), banda, modo (LSB/USB/AM), volume, S-meter ao vivo.
- API: GET /api/status; POST /api/tune (freq em Hz, band, mode, volume, step). Mesma autenticacao das outras paginas.
- Os comandos entram numa fila e so a task principal mexe no SI4732 (nunca em paralelo com o encoder).
- Audio pela web/porta: NAO e possivel neste hardware (o audio do SI4732 vai direto ao amplificador).
