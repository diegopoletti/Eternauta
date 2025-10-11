/**
 * @file Eternauta.ino
 * @brief Proyecto "Eternauta": Reproductor de frases aleatorias con ESP32 y tarjeta SD.
 * @details Este programa utiliza un microcontrolador ESP32 para reproducir un archivo de audio 
 * en formato MP3 de forma aleatoria desde una tarjeta SD. La reproducción se activa 
 * al presionar un pulsador. Un segundo pulsador permite reiniciar el microcontrolador.
 * Está diseñado para ser una base educativa y fácilmente modificable.
 * @author Diego Poletti
 * @version 1.1
 * @date 2025-10-11
 * @note Este código es una adaptación y simplificación del proyecto "Torneo3magos".
 */

// --- INCLUSIÓN DE BIBLIOTECAS ---
// Se incluyen todas las bibliotecas necesarias para el funcionamiento del proyecto.

#include "AudioFileSourceSD.h"     // Proporciona la capacidad de leer archivos desde una tarjeta SD como fuente de audio.
#include "AudioGeneratorMP3.h"     // Contiene el decodificador necesario para interpretar y reproducir archivos en formato MP3.
#include "AudioOutputI2SNoDAC.h"   // Gestiona la salida de audio digital a través del protocolo I2S, sin necesidad de un DAC externo.
#include "FS.h"                    // API del sistema de archivos (File System) para ESP32.
#include "SD.h"                    // Biblioteca para la comunicación y gestión de la tarjeta SD.
#include "SPI.h"                   // Biblioteca para la comunicación a través del bus SPI, usada por la tarjeta SD.
#include <esp_system.h>            // Funciones del sistema del ESP32, utilizada aquí para inicializar la semilla de números aleatorios y para el reinicio.


// --- CONFIGURACIÓN DEL PROYECTO ---
// En esta sección se definen las constantes y pines que se utilizarán.

/**
 * @brief Cantidad total de frases (archivos .mp3) en la tarjeta SD.
 * @details Es fundamental que este valor coincida exactamente con el número de archivos de audio
 * que ha guardado en la tarjeta SD. Los archivos deben estar en la raíz de la SD
 * y nombrarse secuencialmente: "frase1.mp3", "frase2.mp3", etc.
 */
const int CANTIDAD_TOTAL_FRASES = 15; // Ejemplo: 15 frases. ¡Modificar según la cantidad de archivos!

// --- DEFINICIÓN DE PINES ---

// Pines para el bus SPI de la Tarjeta SD.
#define PIN_SCK  18 // Pin de reloj (Serial Clock) para la comunicación SPI.
#define PIN_MISO 19 // Pin de datos de entrada (Master In Slave Out) para SPI.
#define PIN_MOSI 23 // Pin de datos de salida (Master Out Slave In) para SPI.
#define PIN_CS   5  // Pin de selección de chip (Chip Select) para la tarjeta SD.

// Pines para los pulsadores. Se asume una conexión a GND (activo en bajo).
#define PIN_PULSADOR_PLAY 4  // Pin GPIO al que se conecta el pulsador de reproducción.
#define PIN_PULSADOR_RESET 15 // Pin GPIO para el pulsador de reinicio del sistema.


// --- VARIABLES GLOBALES ---
// Declaración de variables que serán accesibles desde cualquier parte del programa.

// Punteros a los objetos de la biblioteca de audio. Se utiliza memoria dinámica (new).
AudioGeneratorMP3 *reproductor_mp3; // Puntero que apuntará al objeto generador (decodificador) de audio MP3.
AudioFileSourceSD *fuente_audio;    // Puntero que apuntará a la fuente del archivo de audio en la tarjeta SD.
AudioOutputI2SNoDAC *salida_audio;  // Puntero que apuntará al objeto de salida de audio a través del bus I2S.

// Variable para controlar el estado de la reproducción.
bool reproduccion_en_curso = false; // Almacena 'true' si un audio se está reproduciendo, o 'false' si está detenido.


/**
 * @brief Función de configuración inicial. Se ejecuta una sola vez al encender o reiniciar el ESP32.
 * @details Aquí se inicializa la comunicación serial para depuración, la tarjeta SD,
 * los pines de los pulsadores, la semilla para números aleatorios y los componentes de audio.
 */
void setup() {
  // Inicializa la comunicación serial a 115200 baudios para poder enviar mensajes a la consola de depuración.
  Serial.begin(115200);

  // Muestra un mensaje de bienvenida en el monitor serial.
  Serial.println("Inicializando el proyecto Eternauta...");

  // Configura los pines de los pulsadores como entradas digitales.
  // INPUT_PULLUP activa una resistencia interna a VCC, por lo que el pulsador solo necesita conectarse a GND.
  // Para entornos con ruido eléctrico, se recomienda usar una resistencia pull-up externa de 10kΩ.
  pinMode(PIN_PULSADOR_PLAY, INPUT_PULLUP);
  pinMode(PIN_PULSADOR_RESET, INPUT_PULLUP);

  // Inicializa el controlador de la tarjeta SD utilizando los pines SPI definidos.
  if (!SD.begin(PIN_CS)) {
    // Si la inicialización falla, se imprime un mensaje de error y el programa se detiene.
    Serial.println("Error al inicializar la tarjeta SD. Verifique las conexiones y el formato.");
    // El bucle infinito detiene la ejecución para evitar más errores.
    while (true);
  }

  // Muestra un mensaje de éxito si la tarjeta SD fue encontrada y montada correctamente.
  Serial.println("Tarjeta SD inicializada correctamente.");

  // Inicializa la semilla del generador de números aleatorios con un valor único del chip.
  // Esto asegura que la secuencia de frases "aleatorias" sea diferente en cada reinicio.
  randomSeed(esp_random());

  // Asigna memoria para los objetos de audio.
  salida_audio = new AudioOutputI2SNoDAC();      // Crea el objeto para la salida de audio.
  reproductor_mp3 = new AudioGeneratorMP3();   // Crea el objeto para el decodificador MP3.
  fuente_audio = new AudioFileSourceSD();      // Crea el objeto para la fuente de audio desde la SD.
  
  // Configura la salida de audio en modo monofónico.
  // NOTA: Por defecto, esta librería usa los pines I2S:
  // LRC (Word Select)   -> GPIO 25
  // BCLK (Bit Clock)    -> GPIO 26
  // DOUT (Data Out)     -> GPIO 22
  salida_audio->SetOutputModeMono(true);
}


/**
 * @brief Bucle principal del programa. Se ejecuta continuamente después de la función setup().
 * @details Su responsabilidad es detectar la pulsación de los botones (con antirrebote),
 * gestionar la reproducción de audio y manejar la solicitud de reinicio.
 */
void loop() {
  // Llama a las funciones que gestionan la lógica de los pulsadores y el audio.
  gestionarPulsadores();
  gestionarReproduccionAudio();
}


/**
 * @brief Detecta y gestiona la pulsación de los botones de Play y Reset.
 * @details Implementa una lógica de antirrebote (debounce) por software para evitar lecturas falsas.
 */
void gestionarPulsadores() {
  // Constante que define el tiempo de espera para el antirrebote en milisegundos.
  const int demora_antirrebote = 50;
  
  // --- Lógica para el botón de PLAY ---
  static bool estado_anterior_play = HIGH; 
  static unsigned long ultimo_tiempo_play = 0;
  
  bool estado_actual_play = digitalRead(PIN_PULSADOR_PLAY);

  if (estado_actual_play == LOW && estado_anterior_play == HIGH) {
    if (millis() - ultimo_tiempo_play > demora_antirrebote) {
      if (!reproduccion_en_curso) {
        reproducirFraseAleatoria();
      }
    }
    ultimo_tiempo_play = millis();
  }
  estado_anterior_play = estado_actual_play;

  // --- Lógica para el botón de RESET ---
  static bool estado_anterior_reset = HIGH;
  static unsigned long ultimo_tiempo_reset = 0;

  bool estado_actual_reset = digitalRead(PIN_PULSADOR_RESET);

  if (estado_actual_reset == LOW && estado_anterior_reset == HIGH) {
    if (millis() - ultimo_tiempo_reset > demora_antirrebote) {
      Serial.println("Botón de reinicio presionado. Reiniciando el sistema...");
      delay(100); // Pequeña demora para asegurar que el mensaje serial se envíe.
      ESP.restart(); // Función que reinicia el microcontrolador.
    }
    ultimo_tiempo_reset = millis();
  }
  estado_anterior_reset = estado_actual_reset;
}


/**
 * @brief Gestiona el estado de la reproducción de audio si está en curso.
 * @details Se encarga de llamar al método loop() del decodificador y de detener
 * la reproducción una vez que el archivo de audio ha finalizado.
 */
void gestionarReproduccionAudio() {
  // Se verifica constantemente si hay una reproducción en curso.
  if (reproduccion_en_curso) {
    // Si el reproductor MP3 sigue en funcionamiento, se ejecuta su lógica interna.
    if (reproductor_mp3->isRunning()) {
      // El método loop() del reproductor se encarga de decodificar y enviar los datos de audio.
      if (!reproductor_mp3->loop()) {
        // Si el método loop() devuelve false, significa que la canción ha terminado.
        reproductor_mp3->stop(); // Se detiene el reproductor.
        fuente_audio->close();   // Se cierra el archivo de audio en la tarjeta SD.
        reproduccion_en_curso = false; // Se actualiza la variable de estado.
        Serial.println("Reproducción finalizada."); // Se informa por el monitor serial.
      }
    }
  }
}


/**
 * @brief Selecciona una frase aleatoria, construye su ruta y la reproduce.
 * @details Esta función genera un número aleatorio, crea el nombre del archivo
 * correspondiente (ej: "/frase5.mp3") y llama a la función que inicia la
 * reproducción si el archivo existe.
 */
void reproducirFraseAleatoria() {
  // Genera un número entero aleatorio entre 1 y la cantidad total de frases disponibles.
  int numero_frase_aleatoria = random(1, CANTIDAD_TOTAL_FRASES + 1);

  // Crea un buffer de caracteres para almacenar el nombre del archivo.
  char ruta_archivo[20]; 
  // Construye la ruta del archivo usando el número aleatorio. ej: "/frase" + "12" + ".mp3".
  sprintf(ruta_archivo, "/frase%d.mp3", numero_frase_aleatoria);

  // Muestra en la consola qué archivo se intentará reproducir.
  Serial.print("Intentando reproducir: ");
  Serial.println(ruta_archivo);
  
  // Verifica si el archivo de audio construido existe en la tarjeta SD.
  if (!SD.exists(ruta_archivo)) {
    // Si el archivo no se encuentra, se notifica el error por el monitor serial.
    Serial.println("El archivo no existe en la tarjeta SD.");
    // Se termina la ejecución de la función para evitar más errores.
    return;
  }

  // Intenta abrir el archivo de audio desde la tarjeta SD.
  if (!fuente_audio->open(ruta_archivo)) {
    // Si hay un error al abrir el archivo, se notifica.
    Serial.print("Error al abrir el archivo: ");
    Serial.println(ruta_archivo);
    // Se termina la ejecución de la función.
    return;
  }
  
  // Si el archivo se abrió correctamente, se inicia el proceso de reproducción.
  reproductor_mp3->begin(fuente_audio, salida_audio);
  // Se establece la variable de estado para indicar que la reproducción ha comenzado.
  reproduccion_en_curso = true;
  // Se informa por la consola que la reproducción ha comenzado con éxito.
  Serial.println("Iniciando reproducción de audio...");
}

