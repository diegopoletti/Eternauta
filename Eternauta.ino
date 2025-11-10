/**
 * @file Eternauta_SPIFFS_Web.ino
 * @brief Proyecto "Eternauta": Reproductor de frases aleatorias con ESP32 y memoria SPIFFS.
 * @details Este programa utiliza un ESP32 para reproducir un archivo MP3 aleatorio desde la
 * memoria flash interna (SPIFFS). La reproducción se activa con un pulsador.
 * Incluye un servidor web para gestionar (subir) los archivos de audio 
 * de forma remota a través de WiFi. Para Boorar se debe hacer borrando la flash desde Herramientas)
 * @author Diego Poletti 
 * @version 2.5 (Lógica v2.4, Interfaz Web v3.5)
 * @date 2025-11-09
 * @note Fusión de la lógica de reproducción de v2.4 con la interfaz web de v3.5.
 */

// --- INCLUSIÓN DE BIBLIOTECAS ---
// Bibliotecas base del sistema de archivos: permiten al ESP32 gestionar la memoria flash interna.
#include "FS.h"     // Interfaz base del Sistema de Archivos (File System).
#include "SPIFFS.h" // Sistema de Archivos Específico para memoria Flash.

// Bibliotecas de audio: necesarias para decodificar y reproducir archivos MP3.
#include "AudioFileSourceSPIFFS.h"  // Fuente de datos de audio que lee desde SPIFFS.
#include "AudioGeneratorMP3.h"      // Decodificador que convierte MP3 en datos de audio PCM.
#include "AudioOutputI2SNoDAC.h"    // Salida de audio a través de I2S, compatible con amplificadores externos como MAX98357A.
#include <esp_system.h>             // Biblioteca del sistema ESP32, usada aquí para la generación de números aleatorios segura.

// Bibliotecas para el Servidor Web: permiten la comunicación remota del ESP32.
#include <WiFi.h>           // Biblioteca fundamental para la conexión a redes WiFi.
#include <WebServer.h>      // Permite crear un servidor HTTP para manejar peticiones web.
#include <HTTPClient.h>     // Permite al ESP32 actuar como cliente HTTP (aunque no se usa directamente en este servidor).


// --- CONFIGURACIÓN DEL PROYECTO ---

/**
 * @brief Cantidad total de frases (archivos .mp3) en la memoria SPIFFS.
 * @details Este número define cuántos archivos de audio busca el sistema.
 * Los archivos deben nombrarse secuencialmente: "frase1.mp3", "frase2.mp3", etc.
 */
const int CANTIDAD_TOTAL_FRASES = 14; 

// --- CONFIGURACIÓN DE WIFI ---
// Reemplaza con los datos de tu red WiFi para que el ESP32 pueda conectarse.
const char* ssid = "estudiantes";       // Nombre de la red WiFi (Service Set Identifier - SSID).
const char* clave = ""; // Contraseña de la red WiFi.

// --- DEFINICIÓN DE PINES ---

#define PIN_PULSADOR_PLAY 23  // Pin GPIO (General Purpose Input/Output) para el pulsador de reproducción.
#define PIN_PULSADOR_RESET 19 // Pin GPIO para el pulsador de reinicio del sistema (hard reset).

// --- VARIABLES GLOBALES DE ESTADO ---

AudioGeneratorMP3 *reproductor_mp3;     // Puntero para controlar el decodificador de MP3.
AudioFileSourceSPIFFS *fuente_audio;    // Puntero para el objeto que lee los datos de audio desde SPIFFS.
AudioOutputI2SNoDAC *salida_audio;      // Puntero para el objeto que envía el audio decodificado al hardware de salida.

bool reproduccion_en_curso = false;     // Bandera de estado: 'true' si se está reproduciendo audio; 'false' en caso contrario.

WebServer servidorWeb(80);              // Objeto Servidor Web, escucha en el puerto 80 (puerto HTTP estándar).
File archivoDeSubida;                   // Objeto temporal para manejar el archivo que se está subiendo actualmente a SPIFFS.

// Variables de estado para el modo web y la comprobación de archivos.
bool servidor_web_activo = false;       // Bandera: 'true' si el WiFi y el servidor web están funcionando.
bool archivos_ok = false;               // Bandera: 'true' si se encontraron todos los archivos MP3 requeridos.


// --- PROTOTIPOS DE FUNCIONES ---
void setupServidorWeb();    // Función que configura las rutas (endpoints) del servidor web.
void activarModoWeb();      // Función que inicia la conexión WiFi y el servidor web.


/**
 * @brief Comprueba que todos los archivos de frases necesarios estén en SPIFFS.
 * @details Itera desde "frase1.mp3" hasta "fraseN.mp3" para verificar su existencia en el sistema de archivos.
 * @return bool true si todos los archivos (frase1.mp3 a fraseN.mp3) existen, false en caso contrario.
 */
bool verificarArchivos() {
  Serial.println("Verificando archivos de frases en SPIFFS..."); // Mensaje informativo por consola.
  bool todo_ok = true; // Asumimos que todo está bien al inicio.
  
  // Bucle que verifica cada archivo secuencialmente (desde 1 hasta la CANTIDAD_TOTAL_FRASES).
  for (int i = 1; i <= CANTIDAD_TOTAL_FRASES; i++) {
    char ruta_archivo[20]; // Buffer para construir el nombre del archivo (e.g., "/frase1.mp3").
    
    // Concatena el número de frase en la cadena de texto con formato (sprintf).
    sprintf(ruta_archivo, "/frase%d.mp3", i); 
    
    // Verifica si el archivo existe en la memoria SPIFFS.
    if (!SPIFFS.exists(ruta_archivo)) { 
      Serial.print("ERROR: Falta el archivo: "); // Si falta, mostramos el error.
      Serial.println(ruta_archivo);
      todo_ok = false; // Cambiamos el estado a 'false' porque al menos uno falta.
    }
  }
  
  if (todo_ok) {
    Serial.println("Todos los archivos requeridos están presentes.");
  }
  return todo_ok; // Retorna el estado final de la verificación.
}

/**
 * @brief Lee el puerto serial para buscar comandos como 'web'.
 * @details Permite al usuario iniciar el modo de administración web enviando el comando por el Monitor Serial.
 */
void leerComandoSerial() {
  if (Serial.available()) { // Comprueba si hay datos disponibles para leer en el puerto serial.
    String comando = Serial.readStringUntil('\n'); // Lee la línea completa hasta un salto de línea.
    comando.trim();         // Elimina espacios en blanco al inicio y al final.
    comando.toLowerCase();  // Convierte el comando a minúsculas para facilitar la comparación.

    if (comando == "web") {
      activarModoWeb(); // Si se recibe "web", inicia la función de servidor web.
    }
  }
}

/**
 * @brief Activa la conexión WiFi y el servidor web si no está activo.
 * @details Este es un modo condicional que solo se activa por el comando serial o si faltan archivos críticos.
 */
void activarModoWeb() {
    if (servidor_web_activo) {
      Serial.println("El servidor web ya está activo.");
      return; // Sale de la función si ya está activo.
    }

    Serial.println("Comando 'web' recibido. Iniciando WiFi...");
    
    // Inicia la conexión a la red con el SSID y la clave definidos.
    WiFi.begin(ssid, clave); 
    Serial.print("Conectando a WiFi [");
    Serial.print(ssid);
    Serial.print("]...");
    
    int intentos = 0;
    // Bucle de espera hasta que la conexión se establezca o se excedan 20 intentos.
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
      delay(500); // Espera 500ms entre intentos.
      Serial.print(".");
      intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n¡Conectado!");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP()); // Muestra la dirección IP asignada.

        // Setup y lanzamiento del servidor.
        setupServidorWeb(); // Llama a la función que define las rutas.
        servidorWeb.begin(); // Inicia el servidor web, esperando peticiones.
        servidor_web_activo = true; // Marca el servidor como activo.
        Serial.println("Servidor web iniciado. Acceda a http://" + WiFi.localIP().toString());
    } else {
        Serial.println("\nERROR: No se pudo conectar a WiFi.");
        Serial.println("Por favor, verifica las credenciales en el código y reinicia para intentarlo de nuevo.");
    }
}


/**
 * @brief Función de configuración inicial. Se ejecuta una sola vez al encender o reiniciar el ESP32.
 */
void setup() {
  Serial.begin(115200); // Inicia la comunicación serial a 115200 baudios para mensajes de depuración.
  Serial.println("\nInicializando el proyecto Eternauta (Versión SPIFFS + Web Condicional)...");

  // Configura los pines de los pulsadores como entradas con resistencia pull-up interna.
  // Esto hace que el pin esté en HIGH (encendido) por defecto y vaya a LOW (apagado) al presionar el botón.
  pinMode(PIN_PULSADOR_PLAY, INPUT_PULLUP); 
  pinMode(PIN_PULSADOR_RESET, INPUT_PULLUP);

  // Inicializa el sistema de archivos SPIFFS. 'true' intenta formatear si falla la inicialización.
  if (!SPIFFS.begin(true)) {
    Serial.println("Error al inicializar SPIFFS. El programa se detendrá.");
    while (true); // Bucle infinito: el programa se detiene si no hay memoria.
  }
  Serial.println("Sistema de archivos SPIFFS inicializado.");

  // Verifica si los archivos MP3 necesarios están presentes.
  archivos_ok = verificarArchivos(); 
  
  if (archivos_ok) {
    Serial.println("--- Sistema en Modo Espera de Botón ---");
    Serial.println("Enviar 'web' por serial para activar la gestión de archivos.");
  } else {
    Serial.println("--- Advertencia: Faltan archivos MP3. Esperando comandos o pulsadores. ---");
    Serial.println("Enviar 'web' por serial para subir los archivos faltantes.");
  }

  randomSeed(esp_random()); // Inicializa la semilla para la función random() con un valor aleatorio real del chip.

  // Crea las instancias (objetos) de audio.
  salida_audio = new AudioOutputI2SNoDAC();     // Objeto para manejar la salida I2S.
  reproductor_mp3 = new AudioGeneratorMP3();    // Objeto para decodificar MP3.
  fuente_audio = new AudioFileSourceSPIFFS();   // Objeto para leer archivos de SPIFFS.
  
  // Configura la salida I2S en modo mono para simplificar la conexión al amplificador.
  salida_audio->SetOutputModeMono(true); 
}

/**
 * @brief Bucle principal del programa. Se ejecuta continuamente después de 'setup()'.
 * @details Este es el corazón del programa donde se realizan todas las tareas repetitivas.
 */
void loop() {
  // Maneja las peticiones del servidor web si está activo. ¡CRÍTICO para la funcionalidad web!
  if (servidor_web_activo) {
    servidorWeb.handleClient(); 
  }
  yield(); // Permite que el sistema operativo del ESP32 realice otras tareas (como WiFi o gestión de tiempo).
  leerComandoSerial(); // Comprueba si el usuario ha enviado un comando por serial.
  yield();
  gestionarPulsadores(); // Verifica el estado de los botones (Play y Reset).
  gestionarReproduccionAudio(); // Mantiene la reproducción de audio en curso (si la hay).
}


// --- FUNCIONES DE LÓGICA PRINCIPAL ---

/**
 * @brief Detecta y gestiona la pulsación de los botones de Play y Reset.
 * @details Implementa una lógica de 'antirrebote' (debounce) para evitar múltiples pulsaciones con una sola presión física.
 */
void gestionarPulsadores() {
  const int demora_antirrebote = 50; // Tiempo de espera en milisegundos para ignorar rebotes eléctricos.
  
  // Variables estáticas para el botón de Play. Mantienen su valor entre llamadas a la función.
  static bool estado_anterior_play = HIGH; 
  static unsigned long ultimo_tiempo_play = 0;
  
  // Lee el estado actual del pin. LOW significa que el botón ha sido presionado.
  bool estado_actual_play = digitalRead(PIN_PULSADOR_PLAY);
  yield(); // Cede el control al planificador del ESP32.

  // Detección de flanco de bajada (de HIGH a LOW) y verificación de antirrebote.
  if (estado_actual_play == LOW && estado_anterior_play == HIGH) {
    // Si ha pasado el tiempo de antirrebote desde la última pulsación válida...
    if (millis() - ultimo_tiempo_play > demora_antirrebote) {
      if (!reproduccion_en_curso) {
        reproducirFraseAleatoria(); // Solo reproduce si no hay audio en curso.
      }
    }
    ultimo_tiempo_play = millis(); // Guarda el tiempo actual como la última pulsación válida.
  }
  estado_anterior_play = estado_actual_play; // Actualiza el estado para la próxima iteración.

  // Repetimos la lógica para el botón de Reset.
  static bool estado_anterior_reset = HIGH;
  static unsigned long ultimo_tiempo_reset = 0;

  bool estado_actual_reset = digitalRead(PIN_PULSADOR_RESET);

  if (estado_actual_reset == LOW && estado_anterior_reset == HIGH) {
    if (millis() - ultimo_tiempo_reset > demora_antirrebote) {
      Serial.println("Botón de reinicio presionado. Reiniciando el sistema...");
      delay(100); // Pequeña pausa para asegurar que el mensaje serial se envíe.
      ESP.restart(); // Función del sistema para reiniciar completamente el ESP32.
    }
    ultimo_tiempo_reset = millis();
  }
  estado_anterior_reset = estado_actual_reset;
}

/**
 * @brief Gestiona el estado de la reproducción de audio si está en curso.
 * @details Llama continuamente a la función 'loop' del reproductor para procesar los datos MP3.
 */
void gestionarReproduccionAudio() {
  if (reproduccion_en_curso) {
    // Si el reproductor está funcionando (reproduciendo un archivo)...
    if (reproductor_mp3->isRunning()) {
      // La función loop() procesa un fragmento de audio. Retorna 'false' cuando el archivo termina.
      if (!reproductor_mp3->loop()) { 
        reproductor_mp3->stop(); // Detiene explícitamente el reproductor.
        fuente_audio->close();  // Cierra el archivo de audio para liberar recursos.
        yield();
        reproduccion_en_curso = false; // Actualiza el estado a inactivo.
        Serial.println("Reproducción finalizada.");
      }
    }
  }
}

/**
 * @brief Selecciona una frase aleatoria de SPIFFS y la reproduce.
 */
void reproducirFraseAleatoria() {
  // Genera un número aleatorio en el rango [1, CANTIDAD_TOTAL_FRASES].
  int numero_frase_aleatoria = random(1, CANTIDAD_TOTAL_FRASES + 1);

  char ruta_archivo[20]; // Buffer para la ruta del archivo.
  
  // Construye la ruta del archivo MP3 seleccionado (e.g., "/frase5.mp3").
  sprintf(ruta_archivo, "/frase%d.mp3", numero_frase_aleatoria);

  Serial.print("Intentando reproducir desde SPIFFS: ");
  Serial.println(ruta_archivo);
  
  // Comprobación de seguridad: verifica si el archivo existe antes de intentar abrirlo.
  if (!SPIFFS.exists(ruta_archivo)) {
    Serial.println("El archivo no existe en SPIFFS. NO SE PUEDE REPRODUCIR.");
    return; // Sale de la función.
  }

  // Intenta abrir el archivo con el objeto fuente_audio.
  if (!fuente_audio->open(ruta_archivo)) {
    Serial.print("Error al abrir el archivo: ");
    Serial.println(ruta_archivo);
    return; // Sale de la función si hay un error al abrir.
  }
  yield();

  // Inicia el proceso de reproducción, conectando la fuente (SPIFFS) con la salida (I2S).
  reproductor_mp3->begin(fuente_audio, salida_audio);
  reproduccion_en_curso = true; // Marca el estado como "reproduciendo".
  Serial.println("Iniciando reproducción de audio...");
}


// --- INICIO: FUNCIONES DEL SERVIDOR WEB ---

/**
 * @brief Genera una cadena HTML con la lista de archivos en SPIFFS.
 * @details Itera a través de los archivos en la raíz de SPIFFS y crea una lista HTML con un botón de "Eliminar" para cada uno.
 * @return String con el listado de archivos en formato HTML.
 */
String listarArchivosSPIFFS() {
  String lista = ""; // Cadena vacía para acumular el código HTML de la lista.
  File root = SPIFFS.open("/"); // Abre el directorio raíz del sistema de archivos.
  
  if (!root) {
    return "<p>Error al abrir el directorio raíz.</p>";
  }
  
  File archivo = root.openNextFile(); // Obtiene el primer archivo.
  while (archivo) { // Bucle que se repite hasta que no haya más archivos.
    String nombreArchivo = String(archivo.name());
    
    // Asegurarse de que el nombre de archivo no tenga el '/' inicial para el parámetro URL.
    if (nombreArchivo.startsWith("/")) {
        nombreArchivo = nombreArchivo.substring(1);
    }
    
    // Genera el elemento de lista (li), mostrando nombre y tamaño.
    lista += "<li><span>" + String(archivo.name()) + " (" + String(archivo.size()) + " bytes)</span>";
    // Crea el enlace para eliminar, pasando el nombre del archivo como parámetro URL.
    lista += "<a href='/delete?filename=" + nombreArchivo + "'>Eliminar</a></li>"; 
    
    archivo = root.openNextFile(); // Pasa al siguiente archivo.
  }
  root.close(); // Cierra el directorio raíz.
  
  // Si la lista está vacía, muestra un mensaje; si no, la envuelve en etiquetas <ul>.
  return (lista == "") ? "<p>No hay archivos en SPIFFS.</p>" : "<ul>" + lista + "</ul>"; 
}

/**
 * @brief Manejador para la página principal (GET /).
 * @details Genera todo el código HTML, CSS y la estructura del formulario de subida y la lista de archivos.
 */
void handleRoot() {
    String html = "<html><head><title>Admin Eternauta</title>"; // Inicio del HTML.
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>"; // Etiqueta para asegurar la visualización correcta en móviles.
    
    // Estilos CSS para dar una apariencia moderna y funcional.
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 1em; background: #f4f4f4; }"; // Estilos generales del cuerpo.
    html += "h1, h2 { color: #333; }"; // Color de los títulos.
    html += "div { background: #fff; border-radius: 8px; padding: 1em; margin-bottom: 1em; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"; // Estilo de las cajas de contenido.
    html += "ul { list-style-type: none; padding: 0; }"; // Quita los puntos de la lista.
    html += "li { margin-bottom: .5em; background: #eee; padding: .5em 1em; border-radius: 5px; display: flex; justify-content: space-between; align-items: center; }"; // Estilo de cada elemento de la lista (con Flexbox).
    html += "li span { flex-grow: 1; }"; // El nombre del archivo ocupa el espacio disponible.
    html += "li a { color: #fff; background-color: #d9534f; padding: 5px 10px; border-radius: 4px; text-decoration: none; font-weight: bold; }"; // Estilo del botón "Eliminar" (rojo).
    html += "li a:hover { background-color: #c9302c; }"; // Efecto hover del botón "Eliminar".
    html += "input { margin: .5em 0; }";
    html += "input[type='file'] { display: block; }";
    html += "input[type='submit'] { background-color: #5cb85c; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; }"; // Estilo del botón "Subir" (verde).
    html += "input[type='submit']:hover { background-color: #4cae4c; }"; // Efecto hover del botón "Subir".
    html += "</style>";
    html += "</head><body>";
    
    html += "<h1>Proyecto Eternauta - Admin SPIFFS</h1>";
    
    html += "<div><h2>Archivos en Memoria</h2>";
    html += listarArchivosSPIFFS(); // Llama a la función para insertar la lista de archivos actual.
    html += "</div>";

    html += "<div><h2>Subir Nuevo Archivo(s)</h2>";
    // Formulario para subir archivos, usa el método POST y codificación multipart/form-data.
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>"; 
    // Campo de archivo. El atributo 'multiple' permite seleccionar varios archivos a la vez.
    html += "<input type='file' name='archivo_subido' multiple required>"; 
    html += "<br><input type='submit' value='Subir Archivo(s)'>";
    html += "</form>";
    // Nota al usuario sobre el esquema de nombres.
    html += "<p><small>Nota: Los archivos deben llamarse 'frase1.mp3', 'frase2.mp3', etc., hasta " + String(CANTIDAD_TOTAL_FRASES) + ".</small></p>"; 
    html += "</div>";

    html += "</body></html>";
    
    // Envía la respuesta al navegador con el código 200 (OK) y el contenido HTML.
    servidorWeb.send(200, "text/html", html); 
}

/**
 * @brief Manejador para el cuerpo de la subida de archivos (POST /upload).
 * @details Esta función se ejecuta repetidamente a medida que el navegador envía bloques de datos del archivo.
 */
void handleFileUpload() {
  // Obtiene el objeto de subida que contiene la información del archivo y el estado.
  HTTPUpload& upload = servidorWeb.upload(); 

  // Lógica de chequeo de seguridad: No permitir subidas si hay audio reproduciéndose.
  if (reproduccion_en_curso && (upload.status == UPLOAD_FILE_START)) {
      Serial.println("Subida de archivo rechazada: reproducción en curso.");
      servidorWeb.send(409, "text/plain", "Rechazado: Reproducción en curso. Intente más tarde.");
      return;
  }
  // Si la reproducción está en curso y no es el inicio, simplemente ignora los paquetes de datos.
  if (reproduccion_en_curso) {
    return; 
  }
  
  // --- Estado: INICIO de la Subida ---
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    
    // Asegurarse de que el nombre comience con '/' (requerido por SPIFFS).
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    
    Serial.print("Iniciando subida de archivo: ");
    Serial.println(filename);

    // Si el archivo ya existe, lo borra para sobrescribir la versión antigua.
    if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
    }
    
    // Abre el archivo en SPIFFS en modo escritura ('w').
    archivoDeSubida = SPIFFS.open(filename, "w"); 
    if (!archivoDeSubida) {
      Serial.println("Error al abrir el archivo en SPIFFS para escritura.");
    }
  } 
  // --- Estado: ESCRITURA del Archivo ---
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (archivoDeSubida) {
      // Escribe el bloque de datos recibido (upload.buf) en el archivo de SPIFFS.
      archivoDeSubida.write(upload.buf, upload.currentSize); 
    }
  } 
  // --- Estado: FIN de la Subida ---
  else if (upload.status == UPLOAD_FILE_END) {
    if (archivoDeSubida) {
      archivoDeSubida.close(); // Cierra el archivo, guardando todos los datos escritos.
      Serial.print("Subida de archivo finalizada: ");
      Serial.println(upload.filename);
      // La redirección y verificación final se manejan en la función anónima de 'setupServidorWeb'.
    }
  }
}

/**
 * @brief Manejador para la eliminación de archivos (GET /delete).
 * @details Recibe el nombre del archivo a eliminar a través de un parámetro en la URL.
 */
void handleDelete() {
  // Chequeo de seguridad: No permite la eliminación si hay audio reproduciéndose.
  if (reproduccion_en_curso) {
      Serial.println("Eliminación de archivo rechazada: reproducción en curso.");
      servidorWeb.send(409, "text/plain", "Rechazado: Reproducción en curso. Intente más tarde.");
      return;
  }
  
  // Comprueba si la petición incluye el parámetro 'filename'.
  if (servidorWeb.hasArg("filename")) {
    String filename = servidorWeb.arg("filename"); // Obtiene el valor del parámetro 'filename'.
    String rutaCompleta = "/" + filename; // Añade la barra inicial para formar la ruta completa de SPIFFS.

    if (SPIFFS.exists(rutaCompleta)) {
      SPIFFS.remove(rutaCompleta); // Elimina el archivo de la memoria flash.
      Serial.print("Archivo eliminado: ");
      Serial.println(rutaCompleta);
      
      archivos_ok = verificarArchivos(); // Vuelve a verificar si faltan archivos necesarios.
      
      servidorWeb.sendHeader("Location", "/"); // Configura el encabezado de redirección.
      servidorWeb.send(303); // Envía una respuesta 303 (See Other) para redirigir al usuario a la página principal.
    } else {
      Serial.print("Intento de eliminar archivo inexistente: ");
      Serial.println(rutaCompleta);
      servidorWeb.send(404, "text/plain", "Archivo no encontrado."); // Respuesta 404 si el archivo no existe.
    }
  } else {
    // Respuesta 400 (Bad Request) si falta el parámetro requerido.
    servidorWeb.send(400, "text/plain", "Solicitud incorrecta. Falta el parámetro 'filename'."); 
  }
}

/**
 * @brief Manejador para 404 Not Found.
 * @details Se ejecuta si el usuario intenta acceder a una ruta que no ha sido definida.
 */
void handleNotFound() {
  servidorWeb.send(404, "text/plain", "Pagina no encontrada."); // Envía una respuesta HTTP 404.
}


/**
 * @brief Configura todas las rutas (endpoints) del servidor web.
 */
void setupServidorWeb() {

  // Asocia la función handleRoot a las peticiones GET en la raíz del sitio ("/").
  servidorWeb.on("/", HTTP_GET, handleRoot);

  // Asocia la función handleFileUpload a las peticiones POST en "/upload".
  servidorWeb.on("/upload", HTTP_POST, 
    // Función 'on-complete': Se ejecuta UNA VEZ después de que se han subido TODOS los archivos.
    [](){ 
      Serial.println("Carga de todos los archivos completada.");
      archivos_ok = verificarArchivos(); // Verifica el estado de los archivos una vez terminada la subida.
      servidorWeb.sendHeader("Location", "/"); // Configura la redirección.
      servidorWeb.send(303); // Redirige al usuario a la página principal.
    }, 
    // Función 'on-upload': Se ejecuta por cada bloque de datos que se sube (manejo del cuerpo de la subida).
    handleFileUpload 
  );

  // Asocia la función handleDelete a las peticiones GET en "/delete".
  servidorWeb.on("/delete", HTTP_GET, handleDelete);

  // Asocia la función handleNotFound a cualquier otra ruta no definida.
  servidorWeb.onNotFound(handleNotFound);
}

// --- FIN: FUNCIONES DEL SERVIDOR WEB ---