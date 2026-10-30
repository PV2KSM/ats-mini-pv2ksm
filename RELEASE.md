# ATS-Mini v2.45 — PV2KSM r28 (modificação da comunidade)

**Modificado por PV2KSM.** Baseado no projeto de código aberto **ESP32-SI4732 / ATS-Mini** (licença MIT, © 2025 ESP32-SI4732 Radio — veja `LICENSE` e `NOTICE.txt`). Firmware não oficial, gratuito e **sem garantia**, baseado no ATS-Mini v2.45 (MIT).

> ## ⚠️ Hardware testado
> **Testado apenas no ATS-Mini variante OSPI** (ESP32-S3, PSRAM octal), por PV2KSM.
> As variantes **QSPI** e **LilyGO T-Embed** foram **compiladas**, mas **não foram testadas em hardware**. Use por sua conta e risco e relate o resultado nos *Issues*.
> Em caso de dúvida, grave **somente** o arquivo da sua variante (a atualização pela web recusa a variante errada).

## Novidades
**Layouts de tela** (Settings → UI Layout / Theme)
- **Cockpit** (estilo painel de rádio de avião): ACTIVE/STANDBY (outro VFO), botões com lâmpadas, janelas de sinal S e SNR; arte pré-renderizada com suavização.
- **Analog**: escala de sintonia com ponteiro vermelho e mostradores analógicos de S e SNR (tema "Analog").
- **Avionics**: painel de aviação geral com LED vermelho de 7 segmentos e barras de LED (tema "Avionics").

**Voos / mapa**: rastreador de voos (adsb.lol) com mapa de fundo (CARTO/OSM, chave do usuário), aeroportos próximos.
**Clima**: agora, dias, horas, **METAR** e **radar de chuva** (Open-Meteo, aviationweather.gov, RainViewer).
**Finanças**: dólar e bitcoin, moeda local escolhida na web.
**Rede**: tela de status; **backup** importar/exportar; **tela do rádio ao vivo** na aba *Control* da web; RDS no Status; fuso de São Paulo por padrão.
**Estabilidade/diagnóstico**: tarefas de rede no núcleo 1, Wi-Fi sem economia de energia, "Last reset" no Status.

## Arquivos
| Arquivo | Para quê |
|---|---|
| `*-ota.bin` | Atualização pelo navegador (Update → Manual upload) |
| `*.zip` | Pacote da variante: OTA + imagem completa (`merged`, endereço 0x0) + bootloader/partições + SHA256 |
| `ats-mini-v2.45-PV2KSM-r28-source.zip` | Código-fonte desta versão |

Variantes: `ospi` (**testada**), `qspi` e `lilygo-t-embed` (**não testadas**).

## Como atualizar
1. Faça **backup** (página web → Backup → exportar, sem segredos se for compartilhar).
2. Abra o IP do rádio no navegador → *Update* → *Manual upload* → escolha o `*-ota.bin` da **sua** variante.
3. Aguarde o reinício. Não desligue durante a gravação.

## Privacidade
O firmware não contém senhas de Wi-Fi, chaves nem dados pessoais. A chave do mapa (CARTO) e o Wi-Fi ficam só na memória do seu rádio.

## Limitações
- API gratuita de radar (RainViewer): só radar passado, zoom limitado; os termos podem mudar.
- Clima (Open-Meteo): gratuito só para **uso não comercial**. Não use este firmware em produto comercial sem as licenças dos serviços. Veja `AVISOS-DE-TERCEIROS.md` no pacote.
- Câmbio, METAR, radar e importação de backup foram pouco testados contra os serviços reais.
- Ao relatar problemas, inclua a linha **Last reset** da página Status e a mensagem na tela.

## Créditos e licenças
ESP32-SI4732 / ATS-Mini (MIT) · fonte B612 (SIL OFL 1.1) · LovyanGFX · dados: adsb.lol, OpenStreetMap/CARTO, Open-Meteo, NOAA, RainViewer, Coinbase, Frankfurter. Modificações por **PV2KSM**.

---

# English summary
**Unofficial community mod by PV2KSM**, no warranty. Based on ATS-Mini v2.45 (MIT).

**⚠️ Tested only on the OSPI variant.** QSPI and LilyGO T-Embed builds are **compiled but untested on hardware** — flash only the file for your own variant and please report results in Issues.

New: Cockpit (airliner radio panel), Analog (tuning scale + gauges) and Avionics (GA radio) layouts; flights tracker with map and airports; weather, METAR and rain radar; USD/BTC page; network status; backup import/export; live screen mirror on the web *Control* tab; stability fixes and a "Last reset" diagnostic.

Update: back up, open the radio's web UI → Update → Manual upload → pick the `*-ota.bin` of your variant.
Third-party data services keep their own terms (Open-Meteo is free for non-commercial use only). No Wi-Fi credentials or keys are included.
