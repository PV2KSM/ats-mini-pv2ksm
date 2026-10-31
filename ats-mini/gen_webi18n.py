#!/usr/bin/env python3
"""Generates WebI18n.h: the client-side dictionary (English key -> Portuguese, Spanish) used by every
web page. English is the page text itself, so a missing key just stays in English.
Run:  python3 tools/gen_webi18n.py > ats-mini/WebI18n.h"""
import json

# (English text as it appears in the page (whitespace collapsed), Portuguese, Spanish)
T = [
# Navigation and titles
("Status","Status","Estado"),("Control","Controle","Control"),("Memory","Memória","Memoria"),
("Location","Localização","Ubicación"),("Finance","Finanças","Finanzas"),("Backup","Backup","Copia de seguridad"),
("Config","Configuração","Configuración"),("Wi-Fi","Wi-Fi","Wi-Fi"),("Update","Atualização","Actualización"),
("Patches","Patches","Parches"),
("ATS-Mini Status","ATS-Mini Status","ATS-Mini Estado"),("ATS-Mini Control","ATS-Mini Controle","ATS-Mini Control"),
("ATS-Mini Memory","ATS-Mini Memória","ATS-Mini Memoria"),("ATS-Mini Location","ATS-Mini Localização","ATS-Mini Ubicación"),
("ATS-Mini Finance","ATS-Mini Finanças","ATS-Mini Finanzas"),("ATS-Mini Backup","ATS-Mini Backup","ATS-Mini Copia de seguridad"),
("ATS-Mini Config","ATS-Mini Configuração","ATS-Mini Configuración"),("ATS-Mini Wi-Fi","ATS-Mini Wi-Fi","ATS-Mini Wi-Fi"),
("ATS-Mini Update","ATS-Mini Atualização","ATS-Mini Actualización"),
("ATS-Mini SI4732 Patches (Experimental)","ATS-Mini Patches do SI4732 (Experimental)","ATS-Mini Parches del SI4732 (Experimental)"),
# Status
("You are tuned to","Você está sintonizado em","Está sintonizado en"),
("Loading weather...","Carregando clima...","Cargando clima..."),
("No weather: this device needs internet access.","Sem clima: este aparelho precisa de acesso à internet.","Sin clima: este dispositivo necesita acceso a Internet."),
("Feels like","Sensação","Sensación"),("Humidity","Umidade","Humedad"),("Wind","Vento","Viento"),("Rain","Chuva","Lluvia"),
("Max","Máx","Máx"),("Min","Mín","Mín"),
("IP Address","Endereço IP","Dirección IP"),("MAC Address","Endereço MAC","Dirección MAC"),("Firmware","Firmware","Firmware"),
("Date/Time","Data/Hora","Fecha/Hora"),("Band","Banda","Banda"),("Frequency","Frequência","Frecuencia"),
("Signal Strength","Intensidade do sinal","Intensidad de la señal"),("Signal to Noise","Relação sinal/ruído","Relación señal/ruido"),
("Last reset","Último reset","Último reinicio"),("Battery Voltage","Tensão da bateria","Tensión de la batería"),
("Download screenshot","Baixar captura de tela","Descargar captura de pantalla"),
# Control
("Radio screen","Tela do rádio","Pantalla de la radio"),("Signal","Sinal","Señal"),("Volume","Volume","Volumen"),
("Step:","Passo:","Paso:"),("Go to frequency","Ir para a frequência","Ir a la frecuencia"),("Go","Ir","Ir"),
("e.g. 98.1 or 7150","ex.: 98.1 ou 7150","p. ej.: 98.1 o 7150"),
("connecting","conectando","conectando"),("online","online","en línea"),("offline","offline","sin conexión"),
("+1 step","+1 passo","+1 paso"),("+10 steps","+10 passos","+10 pasos"),("-1 step","-1 passo","-1 paso"),("-10 steps","-10 passos","-10 pasos"),
("Audio streaming is not available: the SI4732 audio goes straight to the speaker amplifier and never reaches the ESP32.",
 "O áudio não pode ser transmitido: o áudio do SI4732 vai direto ao amplificador do alto-falante e nunca chega ao ESP32.",
 "La transmisión de audio no está disponible: el audio del SI4732 va directo al amplificador del altavoz y nunca llega al ESP32."),
# Memory
("Edit frequencies in Hz (0 clears a slot). Names: up to 9 printable ASCII characters. Use the up/down buttons to reorder slots. Press Save to apply changes.",
 "Edite as frequências em Hz (0 apaga um slot). Nomes: até 9 caracteres ASCII imprimíveis. Use os botões de subir/descer para reordenar. Pressione Salvar para aplicar.",
 "Edite las frecuencias en Hz (0 borra una posición). Nombres: hasta 9 caracteres ASCII imprimibles. Use los botones subir/bajar para reordenar. Pulse Guardar para aplicar."),
("Name","Nome","Nombre"),("Frequency (Hz)","Frequência (Hz)","Frecuencia (Hz)"),("Mode","Modo","Modo"),("Move","Mover","Mover"),
("Move up","Mover para cima","Subir"),("Move down","Mover para baixo","Bajar"),("Save","Salvar","Guardar"),
("Export","Exportar","Exportar"),("Import","Importar","Importar"),("Unsaved changes","Alterações não salvas","Cambios sin guardar"),
("The memory file is too large.","O arquivo de memórias é grande demais.","El archivo de memorias es demasiado grande."),
# Location
("Observer position (Flights and Weather)","Posição do observador (Voos e Clima)","Posición del observador (Vuelos y Clima)"),
("Airport ICAO","ICAO do aeroporto","ICAO del aeropuerto"),("Latitude","Latitude","Latitud"),("Longitude","Longitude","Longitud"),
("Label","Rótulo","Etiqueta"),
("Short text drawn at your position on the radar, e.g. SJK.","Texto curto desenhado na sua posição no radar, ex.: SJK.","Texto corto dibujado en su posición en el radar, p. ej.: SJK."),
("Map range (nm)","Alcance do mapa (nm)","Alcance del mapa (nm)"),
("Distance from your position to the top/bottom edge of the Flights screen. The width shows about 1.9x more.",
 "Distância da sua posição até a borda superior/inferior da tela de Voos. A largura mostra cerca de 1,9x mais.",
 "Distancia desde su posición hasta el borde superior/inferior de la pantalla de Vuelos. El ancho muestra unas 1,9 veces más."),
("Radar map (Flights)","Mapa do radar (Voos)","Mapa del radar (Vuelos)"),("Show map","Mostrar mapa","Mostrar mapa"),
("Draw the basemap behind the radar","Desenhar o mapa de fundo atrás do radar","Dibujar el mapa base detrás del radar"),
("Map style","Estilo do mapa","Estilo del mapa"),
("Automatic (light 06:00-18:00, dark at night)","Automático (claro 06:00-18:00, escuro à noite)","Automático (claro 06:00-18:00, oscuro de noche)"),
("Light","Claro","Claro"),("Dark","Escuro","Oscuro"),
("Works with the default CARTO tiles and with any CARTO URL containing dark_all or light_all.",
 "Funciona com os tiles CARTO padrão e com qualquer URL CARTO que contenha dark_all ou light_all.",
 "Funciona con los tiles CARTO predeterminados y con cualquier URL CARTO que contenga dark_all o light_all."),
("Tile URL","URL dos tiles","URL de los tiles"),
("Raster PNG template with {z}, {x}, {y}. Put {key} where the key goes; without it the key is added as ?key=... (CARTO style). Clear the field to restore the default (CARTO, style above). The map is downloaded when the Flights screen opens (needs Wi-Fi).",
 "Modelo PNG raster com {z}, {x}, {y}. Coloque {key} onde vai a chave; sem ele a chave é adicionada como ?key=... (estilo CARTO). Deixe o campo vazio para restaurar o padrão (CARTO, estilo acima). O mapa é baixado quando a tela de Voos abre (precisa de Wi-Fi).",
 "Plantilla PNG ráster con {z}, {x}, {y}. Ponga {key} donde va la clave; sin él la clave se añade como ?key=... (estilo CARTO). Deje el campo vacío para restaurar el valor predeterminado (CARTO, estilo de arriba). El mapa se descarga al abrir la pantalla de Vuelos (requiere Wi-Fi)."),
("leave empty to keep the current key","deixe vazio para manter a chave atual","deje vacío para mantener la clave actual"),
("Stored only on this radio and never shown again.","Guardada só neste rádio e nunca mostrada de novo.","Se guarda solo en esta radio y no se vuelve a mostrar."),
("Erase the stored key","Apagar a chave guardada","Borrar la clave guardada"),
("e.g. SBSJ","ex.: SBSJ","p. ej.: SBSJ"),("stored","guardada","guardada"),("not set","não definida","sin definir"),
("ICAO code not found in the airport table. Nothing was saved.","Código ICAO não encontrado na tabela de aeroportos. Nada foi salvo.","Código ICAO no encontrado en la tabla de aeropuertos. No se guardó nada."),
("Latitude and longitude must be numbers. Nothing was saved.","Latitude e longitude precisam ser números. Nada foi salvo.","La latitud y la longitud deben ser números. No se guardó nada."),
("Saved. Flights and Weather will use the new position.","Salvo. Voos e Clima usarão a nova posição.","Guardado. Vuelos y Clima usarán la nueva posición."),
# Finance
("Finance screen","Tela de Finanças","Pantalla de Finanzas"),("Local currency","Moeda local","Moneda local"),
("The Finance item of the radio menu shows the US dollar, the euro and Bitcoin converted to this currency. Rates come from Coinbase (Bitcoin) and the European Central Bank via Frankfurter (dollar and euro). The radio needs Wi-Fi and the correct time.",
 "O item Finance do menu do rádio mostra o dólar, o euro e o Bitcoin convertidos para esta moeda. As cotações vêm da Coinbase (Bitcoin) e do Banco Central Europeu via Frankfurter (dólar e euro). O rádio precisa de Wi-Fi e da hora correta.",
 "El elemento Finance del menú de la radio muestra el dólar, el euro y el Bitcoin convertidos a esta moneda. Las cotizaciones vienen de Coinbase (Bitcoin) y del Banco Central Europeo a través de Frankfurter (dólar y euro). La radio necesita Wi-Fi y la hora correcta."),
("Saved. Open Finance in the radio menu.","Salvo. Abra Finance no menu do rádio.","Guardado. Abra Finance en el menú de la radio."),
# Backup
("Saves every setting, memory, band, position and map option into one file.","Salva todas as configurações, memórias, bandas, posição e opções do mapa em um único arquivo.","Guarda todos los ajustes, memorias, bandas, posición y opciones del mapa en un solo archivo."),
("Include Wi-Fi networks, the web login and the map key","Incluir redes Wi-Fi, o login da web e a chave do mapa","Incluir redes Wi-Fi, el acceso web y la clave del mapa"),
("Leave this off if you will share the file: those items are passwords.","Deixe desmarcado se for compartilhar o arquivo: esses itens são senhas.","Déjelo desmarcado si va a compartir el archivo: esos elementos son contraseñas."),
("Download backup","Baixar backup","Descargar copia"),
("Restores a backup file made by this page. The radio restarts afterwards.","Restaura um arquivo de backup feito por esta página. O rádio reinicia depois.","Restaura un archivo de copia hecho por esta página. La radio se reinicia después."),
("Settings in the file replace the current ones. Wi-Fi networks are only restored if the file includes them.","As configurações do arquivo substituem as atuais. As redes Wi-Fi só são restauradas se o arquivo as incluir.","Los ajustes del archivo reemplazan los actuales. Las redes Wi-Fi solo se restauran si el archivo las incluye."),
("Import and restart","Importar e reiniciar","Importar y reiniciar"),
("Choose a backup file first.","Escolha primeiro um arquivo de backup.","Elija primero un archivo de copia."),
("File too large.","Arquivo grande demais.","Archivo demasiado grande."),
("Not a valid JSON file.","Não é um arquivo JSON válido.","No es un archivo JSON válido."),
("This is not an ATS-Mini backup file.","Este não é um arquivo de backup do ATS-Mini.","Este no es un archivo de copia de ATS-Mini."),
("Replace the radio settings with this backup and restart?","Substituir as configurações do rádio por este backup e reiniciar?","¿Reemplazar los ajustes de la radio por esta copia y reiniciar?"),
("Sending...","Enviando...","Enviando..."),
("Failed (the radio may be restarting).","Falhou (o rádio pode estar reiniciando).","Falló (la radio puede estar reiniciándose)."),
# Config
("Use Browser Date/Time","Usar data/hora do navegador","Usar fecha/hora del navegador"),("UTC Date/Time","Data/hora UTC","Fecha/hora UTC"),
("Time Zone","Fuso horário","Zona horaria"),("UI","Interface","Interfaz"),("UI Layout","Layout da tela","Diseño de pantalla"),
("Language (Weather, Flights, Finance)","Idioma (Clima, Voos, Finanças)","Idioma (Clima, Vuelos, Finanzas)"),
("Theme","Tema","Tema"),("Reverse Scrolling","Inverter rolagem","Invertir desplazamiento"),("Zoomed Menu","Menu ampliado","Menú ampliado"),
("Radio","Rádio","Radio"),("FM Region","Região FM","Región FM"),("FM Stereo","FM estéreo","FM estéreo"),("Brightness","Brilho","Brillo"),
("Power","Energia","Energía"),("Sleep Timer","Temporizador de repouso","Temporizador de reposo"),
("Seconds; 0 disables sleep","Segundos; 0 desativa o repouso","Segundos; 0 desactiva el reposo"),
("Sleep Mode","Modo de repouso","Modo de reposo"),("Off Timer","Temporizador de desligamento","Temporizador de apagado"),
("Minutes; 0 disables shutdown. Changing the duration restarts the timer.","Minutos; 0 desativa o desligamento. Mudar a duração reinicia o temporizador.","Minutos; 0 desactiva el apagado. Cambiar la duración reinicia el temporizador."),
("Hardware","Hardware","Hardware"),("Half-step Encoder","Encoder de meio passo","Encoder de medio paso"),
("Enable if the receiver has an audio clamp circuit","Ative se o receptor tiver circuito de audio clamp","Active si el receptor tiene circuito de audio clamp"),
("Connectivity","Conectividade","Conectividad"),("USB Mode","Modo USB","Modo USB"),("TCP Mode","Modo TCP","Modo TCP"),
("Listens on port 60000; requires Wi-Fi","Escuta na porta 60000; requer Wi-Fi","Escucha en el puerto 60000; requiere Wi-Fi"),
("Bluetooth Mode","Modo Bluetooth","Modo Bluetooth"),("Splash Screen","Tela de abertura","Pantalla de inicio"),
("Current Image","Imagem atual","Imagen actual"),("Current splash screen","Tela de abertura atual","Pantalla de inicio actual"),
("Delete Image","Apagar imagem","Borrar imagen"),("Upload PNG","Enviar PNG","Subir PNG"),
# Wi-Fi
("Connection","Conexão","Conexión"),("Wi-Fi Mode","Modo Wi-Fi","Modo Wi-Fi"),
("Changing mode may disconnect this page. Off and Sync Only disable the web interface.","Mudar o modo pode desconectar esta página. Off e Sync Only desativam a interface web.","Cambiar el modo puede desconectar esta página. Off y Sync Only desactivan la interfaz web."),
("Scan Hidden SSIDs","Procurar SSIDs ocultos","Buscar SSID ocultos"),
("Wi-Fi Network 1","Rede Wi-Fi 1","Red Wi-Fi 1"),("Wi-Fi Network 2","Rede Wi-Fi 2","Red Wi-Fi 2"),("Wi-Fi Network 3","Rede Wi-Fi 3","Red Wi-Fi 3"),
("Password","Senha","Contraseña"),("This Web UI Login Credentials","Credenciais de login desta interface web","Credenciales de acceso de esta interfaz web"),
("Username","Usuário","Usuario"),
# Update
("Upload","Enviar","Subir"),("Manual upload","Envio manual","Subida manual"),("Use the","Use o","Use el"),("or","ou","o"),
("for your receiver variant.","para a variante do seu receptor.","para la variante de su receptor."),
("Check for updates","Verificar atualizações","Buscar actualizaciones"),
("Checking for updates...","Verificando atualizações...","Buscando actualizaciones..."),
("Already up to date.","Já está atualizado.","Ya está actualizado."),
("Update requested...","Atualização solicitada...","Actualización solicitada..."),
("Connecting to GitHub...","Conectando ao GitHub...","Conectando con GitHub..."),
("DONE! Rebooting...","PRONTO! Reiniciando...","¡LISTO! Reiniciando..."),
# Patches
("Active Patch Set","Conjunto de patches ativo","Conjunto de parches activo"),("Patch Set","Conjunto de patches","Conjunto de parches"),
("Apply","Aplicar","Aplicar"),("Installed Patches","Patches instalados","Parches instalados"),
("Upload Patch","Enviar patch","Subir parche"),
("One .bin file at a time; maximum size: 32 KiB","Um arquivo .bin por vez; tamanho máximo: 32 KiB","Un archivo .bin cada vez; tamaño máximo: 32 KiB"),
("Delete set","Apagar conjunto","Borrar conjunto"),
("A patch request is in progress.","Há uma solicitação de patch em andamento.","Hay una solicitud de parche en curso."),
# Footer pieces
("Default","Padrão","Predeterminado"),
]

# Regular expressions for texts with numbers inside: (pattern, Portuguese, Spanish); $1.. are the groups
R = [
(r"^If filled, the airport position is used \((\d+) airports in the table\) and the fields below are ignored\.$",
 "Se preenchido, a posição do aeroporto é usada ($1 aeroportos na tabela) e os campos abaixo são ignorados.",
 "Si se rellena, se usa la posición del aeropuerto ($1 aeropuertos en la tabla) y se ignoran los campos de abajo."),
(r"^Key \((.*)\)$","Chave ($1)","Clave ($1)"),
(r"^Required resolution: (\S+) pixels; maximum size: 512 KB$","Resolução necessária: $1 pixels; tamanho máximo: 512 KB","Resolución requerida: $1 píxeles; tamaño máximo: 512 KB"),
(r"^Custom (\d+)$","Personalizado $1","Personalizado $1"),
(r"^New release v([\d.]+) available\.$","Nova versão v$1 disponível.","Nueva versión v$1 disponible."),
(r"^Failed: (.*)$","Falhou: $1","Falló: $1"),
(r"^The file must contain exactly (\d+) memory slots\.$","O arquivo deve conter exatamente $1 slots de memória.","El archivo debe contener exactamente $1 posiciones de memoria."),
(r"^Slot (\d+): invalid memory\.$","Slot $1: memória inválida.","Posición $1: memoria no válida."),
(r"^Import failed: (.*)$","Falha ao importar: $1","Error al importar: $1"),
(r"^Slot (\d+): (.*) No changes saved\. Use your browser's Back button to correct the table\.$",
 "Slot $1: $2 Nada foi salvo. Use o botão Voltar do navegador para corrigir a tabela.",
 "Posición $1: $2 No se guardó nada. Use el botón Atrás del navegador para corregir la tabla."),
(r"^Internal heap: ([\d.]+) KiB used / ([\d.]+) KiB free · PSRAM: ([\d.]+) KiB used / ([\d.]+) KiB free \(at page render\)$",
 "Heap interno: $1 KiB usados / $2 KiB livres · PSRAM: $3 KiB usados / $4 KiB livres (ao gerar a página)",
 "Heap interno: $1 KiB usados / $2 KiB libres · PSRAM: $3 KiB usados / $4 KiB libres (al generar la página)"),
(r"^Data: map © OpenStreetMap contributors, © CARTO · weather Open-Meteo\.com \(CC BY 4\.0\) · rain radar RainViewer · flights adsb\.lol · METAR NOAA Aviation Weather Center · prices Coinbase and Frankfurter \(ECB\) · airports OurAirports · font B612 \(Airbus, SIL OFL\) · added features by PV2KSM$",
 "Dados: mapa © colaboradores do OpenStreetMap, © CARTO · clima Open-Meteo.com (CC BY 4.0) · radar de chuva RainViewer · voos adsb.lol · METAR NOAA Aviation Weather Center · cotações Coinbase e Frankfurter (BCE) · aeroportos OurAirports · fonte B612 (Airbus, SIL OFL) · recursos adicionais por PV2KSM",
 "Datos: mapa © colaboradores de OpenStreetMap, © CARTO · clima Open-Meteo.com (CC BY 4.0) · radar de lluvia RainViewer · vuelos adsb.lol · METAR NOAA Aviation Weather Center · cotizaciones Coinbase y Frankfurter (BCE) · aeropuertos OurAirports · fuente B612 (Airbus, SIL OFL) · funciones añadidas por PV2KSM"),
]

# Weather descriptions for the Status page (Open-Meteo WMO codes) per language
WX = {
 "en": {0:"Clear sky",1:"Mostly clear",2:"Partly cloudy",3:"Overcast",45:"Fog",48:"Freezing fog",51:"Light drizzle",53:"Drizzle",55:"Heavy drizzle",56:"Freezing drizzle",57:"Freezing drizzle",61:"Light rain",63:"Rain",65:"Heavy rain",66:"Freezing rain",67:"Freezing rain",71:"Light snow",73:"Snow",75:"Heavy snow",77:"Snow grains",80:"Light showers",81:"Rain showers",82:"Heavy showers",85:"Snow showers",86:"Snow showers",95:"Thunderstorm",96:"Thunderstorm, hail",99:"Thunderstorm, hail"},
 "pt": {0:"Céu limpo",1:"Quase limpo",2:"Parcialmente nublado",3:"Nublado",45:"Neblina",48:"Neblina gelada",51:"Garoa fraca",53:"Garoa",55:"Garoa forte",56:"Garoa gelada",57:"Garoa gelada",61:"Chuva fraca",63:"Chuva",65:"Chuva forte",66:"Chuva gelada",67:"Chuva gelada",71:"Neve fraca",73:"Neve",75:"Neve forte",77:"Grãos de neve",80:"Pancadas fracas",81:"Pancadas de chuva",82:"Pancadas fortes",85:"Pancadas de neve",86:"Pancadas de neve",95:"Trovoadas",96:"Trovoada com granizo",99:"Trovoada com granizo"},
 "es": {0:"Despejado",1:"Poco nuboso",2:"Parcialmente nuboso",3:"Nublado",45:"Niebla",48:"Niebla helada",51:"Llovizna débil",53:"Llovizna",55:"Llovizna fuerte",56:"Llovizna helada",57:"Llovizna helada",61:"Lluvia débil",63:"Lluvia",65:"Lluvia fuerte",66:"Lluvia helada",67:"Lluvia helada",71:"Nieve débil",73:"Nieve",75:"Nieve fuerte",77:"Granos de nieve",80:"Chubascos débiles",81:"Chubascos",82:"Chubascos fuertes",85:"Chubascos de nieve",86:"Chubascos de nieve",95:"Tormenta",96:"Tormenta con granizo",99:"Tormenta con granizo"},
}

JS = r"""(function(){
var LG=window.LANG||'en',IX={pt:1,es:2}[LG]||0;
window.LOCALE={pt:'pt-BR',es:'es-ES',en:'en-US'}[LG]||'en-US';
var T=%(T)s;
var R=%(R)s;
var WX=%(WX)s;
var M={};for(var i=0;i<T.length;i++)M[T[i][0]]=T[i];
function norm(s){return String(s).replace(/\s+/g,' ').trim();}
function tr(s){
  if(!IX||s===null||s===undefined)return s;
  var n=norm(s),e=M[n];
  if(e)return e[IX];
  for(var j=0;j<R.length;j++){var m=n.match(new RegExp(R[j][0]));if(m){return R[j][IX].replace(/\$(\d)/g,function(_,k){return tr(m[+k])||m[+k];});}}
  return s;
}
window.TR=tr;
window.WXDESC=function(c){var d=WX[LG]||WX.en;return d[c]||d[3];};
function apply(){
  if(!IX)return;
  var w=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT,null),n,list=[];
  while((n=w.nextNode())){var p=n.parentNode&&n.parentNode.nodeName;if(p==='SCRIPT'||p==='STYLE')continue;if(/\S/.test(n.nodeValue))list.push(n);}
  list.forEach(function(t){var v=t.nodeValue,lead=v.match(/^\s*/)[0],trail=v.match(/\s*$/)[0],x=tr(v);if(x!==v&&x!==norm(v))t.nodeValue=lead+x+trail;else if(x!==norm(v))t.nodeValue=lead+x+trail;});
  var at=['placeholder','title','alt','aria-label'];
  document.querySelectorAll('[placeholder],[title],[alt],[aria-label]').forEach(function(el){at.forEach(function(a){if(el.hasAttribute(a)){var v=el.getAttribute(a),x=tr(v);if(x!==v)el.setAttribute(a,x);}});});
  document.querySelectorAll('input[type=submit],input[type=button]').forEach(function(el){var v=el.value,x=tr(v);if(x!==v)el.value=x;});
  document.title=tr(document.title);
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',apply);else apply();
})();
"""

def js(o): return json.dumps(o, ensure_ascii=False, separators=(',',':'))

def main():
    # regex patterns are JS regex source strings (already escaped for Python raw strings)
    body = JS % {"T": js([list(x) for x in T]), "R": js([list(x) for x in R]), "WX": js(WX)}
    assert ')JS"' not in body
    print('// Generated by tools/gen_webi18n.py - do not edit by hand.')
    print('#ifndef WEB_I18N_H\n#define WEB_I18N_H\n')
    print('static const char webI18nJs[] PROGMEM = R"JS(' + body + ')JS";')
    print('\n#endif // WEB_I18N_H')

main()
