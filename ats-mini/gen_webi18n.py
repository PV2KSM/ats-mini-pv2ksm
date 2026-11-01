#!/usr/bin/env python3
"""Generates WebI18n.h: the client-side dictionary (English key -> Portuguese, Spanish) used by every
web page. English is the page text itself, so a missing key just stays in English.
Run:  python3 tools/gen_webi18n.py > ats-mini/WebI18n.h"""
import json

# (English text as it appears in the page (whitespace collapsed), Portuguese, Spanish)
T = [
# Map source, web navigation
("OpenStreetMap (default, no key needed)","OpenStreetMap (padrão, sem chave)","OpenStreetMap (predeterminado, sin clave)"),
("Custom (Tile URL below)","Personalizado (URL dos tiles abaixo)","Personalizado (URL de los tiles abajo)"),
("OpenStreetMap tiles are a free community service. Access may be blocked without notice if usage is heavy.","Os tiles do OpenStreetMap são um serviço comunitário gratuito. O acesso pode ser bloqueado sem aviso se o uso for intenso.","Los tiles de OpenStreetMap son un servicio comunitario gratuito. El acceso puede bloquearse sin aviso si el uso es intenso."),
("OpenStreetMap tile usage policy","Política de uso dos tiles do OpenStreetMap","Política de uso de los tiles de OpenStreetMap"),
("Raster PNG template with {z}, {x}, {y}. Use {key} where a key goes (only for custom providers that need one). Clear the field to restore OpenStreetMap. The map is downloaded when the Flights screen opens (needs Wi-Fi).","Modelo PNG raster com {z}, {x}, {y}. Use {key} onde vai a chave (só para provedores personalizados que exigem uma). Deixe o campo vazio para restaurar o OpenStreetMap. O mapa é baixado quando a tela de Voos abre (precisa de Wi-Fi).","Plantilla PNG ráster con {z}, {x}, {y}. Use {key} donde va la clave (solo para proveedores personalizados que la exijan). Deje el campo vacío para restaurar OpenStreetMap. El mapa se descarga cuando se abre la pantalla de Vuelos (necesita Wi-Fi)."),
("Navigation (same as the encoder)","Navegação (igual ao encoder)","Navegación (igual que el encoder)"),
("\u25c0 Rotate","\u25c0 Girar","\u25c0 Girar"),("Rotate \u25b6","Girar \u25b6","Girar \u25b6"),
("Click","Clique","Clic"),("Short press","Pressão curta","Pulsación corta"),
("Rotate down: tune down / previous menu item","Girar para baixo: sintoniza para baixo / item anterior do menu","Girar hacia abajo: sintoniza hacia abajo / elemento anterior del menú"),
("Rotate up: tune up / next menu item","Girar para cima: sintoniza para cima / próximo item do menu","Girar hacia arriba: sintoniza hacia arriba / siguiente elemento del menú"),
("Single click: select / open the menu","Clique simples: seleciona / abre o menu","Clic simple: selecciona / abre el menú"),
("Short press (over half a second)","Pressão curta (mais de meio segundo)","Pulsación corta (más de medio segundo)"),
("Rotate moves the frequency or scrolls the menu. Click opens or selects. Short press is the encoder button held for about half a second.","Girar muda a frequência ou rola o menu. Clique abre ou seleciona. Pressão curta é o botão do encoder mantido por cerca de meio segundo.","Girar cambia la frecuencia o desplaza el menú. Clic abre o selecciona. La pulsación corta es el botón del encoder mantenido aproximadamente medio segundo."),

# New look (Visual, colours, menu style, new layouts)

("Accent color","Cor de destaque","Color de acento"),
("Text color","Cor do texto","Color del texto"),
("Background","Fundo","Fondo"),
("Menu color","Cor do menu","Color del menú"),
("Menu style","Estilo do menu","Estilo del menú"),
("Original","Original","Original"),

("Same as accent","Igual ao destaque","Igual que el acento"),
("Carousel","Carrossel","Carrusel"),
("Grid","Grade","Cuadrícula"),
("Amber","Âmbar","Ámbar"),
("Green","Verde","Verde"),
("Cyan","Ciano","Cian"),
("Blue","Azul","Azul"),
("Magenta","Magenta","Magenta"),
("Red","Vermelho","Rojo"),
("White","Branco","Blanco"),
("Accent","Destaque","Acento"),
("Cream","Creme","Crema"),
("Orange","Laranja","Naranja"),
("Yellow","Amarelo","Amarillo"),
("Mint","Menta","Menta"),
("Sky","Céu","Cielo"),
("Pink","Rosa","Rosa"),
("Graphite","Grafite","Grafito"),
("Black","Preto","Negro"),
("Navy","Marinho","Marino"),
("Wine","Vinho","Vino"),
("Forest","Floresta","Bosque"),
("Neo Analog","Neo Analógico","Neo Analógico"),
("Neo S-Meter","Neo S-Meter","Neo S-Meter"),
("Vintage","Vintage","Vintage"),
("Clock","Relógio","Reloj"),
("Memories","Memórias","Memorias"),
("Minimal","Mínimo","Mínimo"),
("VFD","VFD","VFD"),
("LCD","LCD","LCD"),
("Neon","Neon","Neón"),
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
("Step:","Passo:","Paso:"),("Tuning","Sintonia","Sintonía"),("Go to frequency","Ir para a frequência","Ir a la frecuencia"),("Go","Ir","Ir"),
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
("Tile URL","URL dos tiles","URL de los tiles"),("Map source","Fonte do mapa","Fuente del mapa"),
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

# News screen
("News","Notícias","Noticias"),
("ATS-Mini News","ATS-Mini Notícias","ATS-Mini Noticias"),
("News screen","Tela de Notícias","Pantalla de Noticias"),
("Brazil","Brasil","Brasil"),("United States","Estados Unidos","Estados Unidos"),("United Kingdom","Reino Unido","Reino Unido"),
("Canada","Canadá","Canadá"),("Australia","Austrália","Australia"),("India","Índia","India"),("Spain","Espanha","España"),("Mexico","México","México"),
("Colombia","Colômbia","Colombia"),("France","França","Francia"),("Germany","Alemanha","Alemania"),("Italy","Itália","Italia"),
("Netherlands","Holanda","Países Bajos"),("Poland","Polônia","Polonia"),

("Current news and QR code","Notícias atuais e QR code","Noticias actuales y código QR"),
("Lists the items the radio holds now (open News on the radio first). Tap a headline to open the article. Last column: length of the article address; the QR code needs 1000 or less.","Lista as notícias que o rádio guarda agora (abra Notícias no rádio antes). Toque numa manchete para abrir a matéria. A última coluna é o tamanho do endereço da matéria; o QR code precisa de 1000 ou menos.","Lista las noticias que la radio guarda ahora (abra Noticias en la radio antes). Toque un titular para abrir el artículo. La última columna es la longitud de la dirección del artículo; el código QR necesita 1000 o menos."),
("Refresh list","Atualizar lista","Actualizar lista"),
("Country","País","País"),
("Headlines come from the Google News edition of this country.","As manchetes vêm da edição deste país do Google Notícias.","Los titulares vienen de la edición de este país de Google Noticias."),
("City or subject (optional)","Cidade ou assunto (opcional)","Ciudad o tema (opcional)"),
("Adds the news about this city or subject, for example Campinas or solar energy.","Adiciona as notícias sobre esta cidade ou assunto, por exemplo Campinas ou energia solar.","Añade las noticias sobre esta ciudad o tema, por ejemplo Campinas o energía solar."),
("Topics","Tópicos","Temas"),
("Top stories","Principais","Principales"),("World","Mundo","Mundo"),("Nation","País (nacional)","Nacional"),("Business","Economia","Economía"),
("Technology","Tecnologia","Tecnología"),("Science","Ciência","Ciencia"),("Sports","Esportes","Deportes"),("Health","Saúde","Salud"),("Entertainment","Entretenimento","Entretenimiento"),
("Your own site or feed (optional)","Seu próprio site ou feed (opcional)","Su propio sitio o feed (opcional)"),
("Test","Testar","Probar"),
("Paste the address of a news site or of its RSS feed. The radio looks for the feed by itself. Each publisher sets the terms of use of its own feed. Leave empty to use only Google News.","Cole o endereço de um site de notícias ou do seu feed RSS. O rádio procura o feed sozinho. Cada publicador define os termos de uso do próprio feed. Deixe vazio para usar só o Google Notícias.","Pegue la dirección de un sitio de noticias o de su feed RSS. La radio busca el feed sola. Cada editor define los términos de uso de su propio feed. Déjelo vacío para usar solo Google Noticias."),
("Update every","Atualizar a cada","Actualizar cada"),
("15 min","15 min","15 min"),("30 min","30 min","30 min"),("1 hour","1 hora","1 hora"),
("Seconds per item","Segundos por notícia","Segundos por noticia"),
("Show","Mostrar","Mostrar"),("Summary","Resumo","Resumen"),("Source and age","Fonte e hora","Fuente y hora"),
("The News item of the radio menu shows the headlines. The radio needs Wi-Fi and the correct time. Google News feeds have no summary. The radio screen has no accents, so they are removed there.","O item Notícias do menu do rádio mostra as manchetes. O rádio precisa de Wi-Fi e da hora correta. Os feeds do Google Notícias não têm resumo. A tela do rádio não tem acentos, então eles são removidos ali.","El elemento Noticias del menú de la radio muestra los titulares. La radio necesita Wi-Fi y la hora correcta. Los feeds de Google Noticias no tienen resumen. La pantalla de la radio no tiene acentos, por lo que se eliminan allí."),
("Saved. Open News in the radio menu.","Salvo. Abra Notícias no menu do rádio.","Guardado. Abra Noticias en el menú de la radio."),
("Unknown country.","País desconhecido.","País desconocido."),
("The address cannot have spaces.","O endereço não pode ter espaços.","La dirección no puede tener espacios."),
("That does not look like a web address.","Isso não parece um endereço da web.","Eso no parece una dirección web."),
("Only https:// addresses are supported.","Só endereços https:// são aceitos.","Solo se admiten direcciones https://."),
("Testing...","Testando...","Probando..."),
("Took too long. Try again.","Demorou demais. Tente de novo.","Tardó demasiado. Inténtelo de nuevo."),
("Could not reach the radio.","Não consegui falar com o rádio.","No se pudo contactar con la radio."),

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
("Off","Desligado","Apagado"),("Night","Noite","Noche"),("Space","Espaço","Espacio"),("Phosphor","Fósforo","Fósforo"),
("Locked","Travado","Bloqueado"),("Unlocked","Destravado","Desbloqueado"),("CPU Sleep","Sono da CPU","Reposo de CPU"),
("Unpair All","Desparear tudo","Desemparejar todo"),("AP Only","Somente AP","Solo AP"),("AP+Connect","AP + conectar","AP + conectar"),
("Connect","Conectar","Conectar"),("Sync Only","Somente sincronizar","Solo sincronizar"),
# Radio messages shown on the web pages
('A screenshot download is already in progress.','Já há uma captura de tela sendo baixada.','Ya hay una captura de pantalla en descarga.'),
('An update is already in progress.','Já há uma atualização em andamento.','Ya hay una actualización en curso.'),
('Another patch request is in progress.','Outra solicitação de patch está em andamento.','Otra solicitud de parche está en curso.'),
('Cannot create the patch file. Check free storage.','Não foi possível criar o arquivo do patch. Verifique o espaço livre.','No se pudo crear el archivo del parche. Compruebe el espacio libre.'),
('Connect Wi-Fi first.','Conecte o Wi-Fi primeiro.','Conecte primero el Wi-Fi.'),
('Connecting to WiFi network...','Conectando à rede Wi-Fi...','Conectando a la red Wi-Fi...'),
('Coordinates cannot both be zero.','As coordenadas não podem ser ambas zero.','Las coordenadas no pueden ser ambas cero.'),
('Coordinates out of range.','Coordenadas fora do intervalo.','Coordenadas fuera de rango.'),
('Could not delete all patches. Try again.','Não foi possível apagar todos os patches. Tente novamente.','No se pudieron borrar todos los parches. Inténtelo de nuevo.'),
('Could not install patch. Check free storage and try again.','Não foi possível instalar o patch. Verifique o espaço livre e tente novamente.','No se pudo instalar el parche. Compruebe el espacio libre e inténtelo de nuevo.'),
('Could not write patch data. Check free storage.','Não foi possível gravar os dados do patch. Verifique o espaço livre.','No se pudieron escribir los datos del parche. Compruebe el espacio libre.'),
('Currency must be a 3-letter code.','A moeda deve ser um código de 3 letras.','La moneda debe ser un código de 3 letras.'),
('Currency not in the supported list.','Moeda fora da lista suportada.','Moneda fuera de la lista admitida.'),
('Firmware download failed.','Falha ao baixar o firmware.','Error al descargar el firmware.'),
('Firmware download interrupted.','Download do firmware interrompido.','Descarga del firmware interrumpida.'),
('Firmware download timed out.','Tempo esgotado no download do firmware.','Tiempo agotado en la descarga del firmware.'),
('Firmware exceeds the declared size.','O firmware excede o tamanho declarado.','El firmware supera el tamaño declarado.'),
('Firmware is incomplete.','O firmware está incompleto.','El firmware está incompleto.'),
('Frequency is out of range.','Frequência fora do intervalo.','Frecuencia fuera de rango.'),
('Frequency or mode does not match the band.','Frequência ou modo não correspondem à banda.','La frecuencia o el modo no coinciden con la banda.'),
('Incompatible firmware; use USB.','Firmware incompatível; use USB.','Firmware incompatible; use USB.'),
('Incomplete or unexpected patch upload.','Envio do patch incompleto ou inesperado.','Envío del parche incompleto o inesperado.'),
('Incomplete patch or missing trailer.','Patch incompleto ou sem o trecho final.','Parche incompleto o sin el final.'),
('Invalid Wi-Fi mode.','Modo Wi-Fi inválido.','Modo Wi-Fi no válido.'),
('Invalid band.','Banda inválida.','Banda no válida.'),
('Invalid configuration value. Reload the Config page and try again.','Valor de configuração inválido. Recarregue a página Config e tente novamente.','Valor de configuración no válido. Recargue la página Config e inténtelo de nuevo.'),
('Invalid coordinates.','Coordenadas inválidas.','Coordenadas no válidas.'),
('Invalid firmware download size.','Tamanho de download do firmware inválido.','Tamaño de descarga del firmware no válido.'),
('Invalid firmware size.','Tamanho de firmware inválido.','Tamaño de firmware no válido.'),
('Invalid frequency in Hz.','Frequência em Hz inválida.','Frecuencia en Hz no válida.'),
('Invalid frequency.','Frequência inválida.','Frecuencia no válida.'),
('Invalid mode.','Modo inválido.','Modo no válido.'),
('Invalid navigation command.','Comando de navegação inválido.','Comando de navegación no válido.'),
('Invalid patch slot or action.','Slot ou ação de patch inválido.','Posición o acción de parche no válida.'),
('Invalid patch slot or mode.','Slot ou modo de patch inválido.','Posición o modo de parche no válido.'),
('Invalid patch slot.','Slot de patch inválido.','Posición de parche no válida.'),
('Invalid patch structure or file exceeds 32 KiB.','Estrutura de patch inválida ou arquivo maior que 32 KiB.','Estructura de parche no válida o archivo mayor de 32 KiB.'),
('Invalid step.','Passo inválido.','Paso no válido.'),
('Invalid volume.','Volume inválido.','Volumen no válido.'),
('Key too long.','Chave longa demais.','Clave demasiado larga.'),
('Missing OTA metadata; use USB.','Metadados OTA ausentes; use USB.','Faltan metadatos OTA; use USB.'),
('Missing patch slot or action.','Slot ou ação do patch ausente.','Falta la posición o la acción del parche.'),
('Missing slot fields.','Campos do slot ausentes.','Faltan campos de la posición.'),
('Mod: use Manual upload.','Mod: use o envio manual.','Mod: use la carga manual.'),
('Names must be at most 9 characters.','Os nomes devem ter no máximo 9 caracteres.','Los nombres deben tener como máximo 9 caracteres.'),
('Names must contain printable ASCII characters.','Os nomes devem conter caracteres ASCII imprimíveis.','Los nombres deben contener caracteres ASCII imprimibles.'),
('No complete firmware uploaded.','Nenhum firmware completo foi enviado.','No se ha enviado ningún firmware completo.'),
('No data received.','Nenhum dado recebido.','No se recibieron datos.'),
('Not enough PSRAM for update.','PSRAM insuficiente para a atualização.','PSRAM insuficiente para la actualización.'),
('Not enough memory to process the patch set.','Memória insuficiente para processar o conjunto de patches.','Memoria insuficiente para procesar el conjunto de parches.'),
('Not enough memory to save the slots.','Memória insuficiente para salvar os slots.','Memoria insuficiente para guardar las posiciones.'),
('Not enough memory to validate the PNG image.','Memória insuficiente para validar a imagem PNG.','Memoria insuficiente para validar la imagen PNG.'),
('Nothing to do.','Nada a fazer.','Nada que hacer.'),
('Off and Sync Only do not provide a web interface.','Off e Sync Only não oferecem interface web.','Off y Sync Only no ofrecen interfaz web.'),
('Patch storage is unavailable.','O armazenamento de patches não está disponível.','El almacenamiento de parches no está disponible.'),
('Range must be 5 to 110 nm.','O alcance deve ser de 5 a 110 nm.','El alcance debe ser de 5 a 110 nm.'),
('Receiver is busy. Try again.','O receptor está ocupado. Tente novamente.','El receptor está ocupado. Inténtelo de nuevo.'),
('Release has no matching OTA file.','A versão não tem o arquivo OTA correspondente.','La versión no tiene el archivo OTA correspondiente.'),
('Select a firmware .bin file.','Selecione um arquivo .bin de firmware.','Seleccione un archivo .bin de firmware.'),
('Select another patch set before editing this slot.','Selecione outro conjunto de patches antes de editar este slot.','Seleccione otro conjunto de parches antes de editar esta posición.'),
('Set the clock before updating.','Acerte o relógio antes de atualizar.','Ajuste el reloj antes de actualizar.'),
('Synchronizing time...','Sincronizando a hora...','Sincronizando la hora...'),
('The splash image could not be installed.','Não foi possível instalar a imagem de abertura.','No se pudo instalar la imagen de inicio.'),
('The splash image could not be stored.','Não foi possível guardar a imagem de abertura.','No se pudo guardar la imagen de inicio.'),
('The splash image filename must end in .png.','O nome do arquivo da imagem de abertura deve terminar em .png.','El nombre del archivo de la imagen de inicio debe terminar en .png.'),
('The splash image must not exceed 512 KB.','A imagem de abertura não pode passar de 512 KB.','La imagen de inicio no puede superar 512 KB.'),
('This patch slot is empty.','Este slot de patch está vazio.','Esta posición de parche está vacía.'),
('Tile URL has invalid characters.','A URL dos tiles tem caracteres inválidos.','La URL de los tiles tiene caracteres no válidos.'),
('Tile URL too long.','URL dos tiles longa demais.','URL de los tiles demasiado larga.'),
('Unable to allocate screenshot buffer.','Não foi possível alocar o buffer da captura de tela.','No se pudo reservar el búfer de la captura de pantalla.'),
('Unable to capture screenshot.','Não foi possível capturar a tela.','No se pudo capturar la pantalla.'),
('Unable to connect to GitHub.','Não foi possível conectar ao GitHub.','No se pudo conectar con GitHub.'),
('Unable to find the latest release.','Não foi possível encontrar a versão mais recente.','No se pudo encontrar la última versión.'),
('Unable to start firmware download.','Não foi possível iniciar o download do firmware.','No se pudo iniciar la descarga del firmware.'),
('Unable to start screenshot download.','Não foi possível iniciar o download da captura de tela.','No se pudo iniciar la descarga de la captura de pantalla.'),
('Unable to start update.','Não foi possível iniciar a atualização.','No se pudo iniciar la actualización.'),
('Unexpected release version.','Versão inesperada.','Versión inesperada.'),
('Unknown mode.','Modo desconhecido.','Modo desconocido.'),
('Update canceled.','Atualização cancelada.','Actualización cancelada.'),
('Update failed.','A atualização falhou.','La actualización falló.'),
('Upload discarded.','Envio descartado.','Envío descartado.'),
('Upload exactly one .bin patch file.','Envie exatamente um arquivo de patch .bin.','Envíe exactamente un archivo de parche .bin.'),
('Upload interrupted.','Envio interrompido.','Envío interrumpido.'),
('Upload one complete .bin patch.','Envie um patch .bin completo.','Envíe un parche .bin completo.'),
('Wrong firmware variant; select the matching image.','Variante de firmware errada; selecione a imagem correspondente.','Variante de firmware incorrecta; seleccione la imagen correspondiente.'),
('Audio Clamp','Audio Clamp (grampo de áudio)','Audio Clamp (limitador de audio)'),
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
(r"^Data: map © OpenStreetMap contributors, weather Open-Meteo\.com \(CC BY 4\.0\) · rain radar RainViewer · flights adsb\.lol · METAR NOAA Aviation Weather Center · prices Coinbase and Frankfurter \(ECB\) · airports OurAirports · font B612 \(Airbus, SIL OFL\) · added features by PV2KSM$",
 "Dados: mapa © colaboradores do OpenStreetMap, clima Open-Meteo.com (CC BY 4.0) · radar de chuva RainViewer · voos adsb.lol · METAR NOAA Aviation Weather Center · cotações Coinbase e Frankfurter (BCE) · aeroportos OurAirports · fonte B612 (Airbus, SIL OFL) · recursos adicionais por PV2KSM",
 "Datos: mapa © colaboradores de OpenStreetMap, clima Open-Meteo.com (CC BY 4.0) · radar de lluvia RainViewer · vuelos adsb.lol · METAR NOAA Aviation Weather Center · cotizaciones Coinbase y Frankfurter (BCE) · aeropuertos OurAirports · fuente B612 (Airbus, SIL OFL) · funciones añadidas por PV2KSM"),
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
var mo=null,tm=0;
function again(){tm=0;if(mo)mo.disconnect();apply();if(mo)mo.observe(document.body,{childList:true,subtree:true,characterData:true});}
function start(){apply();if(!IX||!window.MutationObserver)return;mo=new MutationObserver(function(){if(!tm)tm=setTimeout(again,60);});mo.observe(document.body,{childList:true,subtree:true,characterData:true});}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',start);else start();
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
