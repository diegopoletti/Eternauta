# **🤖 Proyecto "Eternauta": Reproductor de Frases Aleatorias con Gestión Web (ESP32)**

## **🌟 Descripción General del Proyecto**

El **Proyecto Eternauta** es un sistema de reproducción de audio basado en el microcontrolador ESP32. Su objetivo principal es reproducir archivos de sonido (frases) almacenados en su memoria interna (SPIFFS) de manera aleatoria al presionar un botón.

Este proyecto va más allá, ya que incorpora un **servidor web condicional** que permite a los usuarios (estudiantes o profesores) cargar, subir y eliminar los archivos de audio de forma remota a través de una red Wi-Fi, sin necesidad de desconectar el ESP32 del circuito. Es una aplicación práctica que integra el hardware (pulsadores y audio) con la comunicación de red.

### **Objetivos de Aprendizaje**

Este proyecto es ideal para estudiantes que deseen aprender sobre:

1. **Manejo de Memoria Flash (SPIFFS):** Utilización del sistema de archivos interno del ESP32 para almacenar datos (MP3).  
2. **Reproducción de Audio:** Decodificación de MP3 y envío de datos de audio a través de la interfaz I2S/PDM del ESP32.  
3. **Programación Reactiva:** Uso de la función loop() para gestionar múltiples tareas (botones, audio, red) sin bloquear el sistema.  
4. **Servidores Web Embebidos:** Configuración de un servidor HTTP básico para interacción remota.

## **🛠️ Componentes de Hardware Necesarios**

Para ensamblar el "Eternauta", necesitarás los siguientes elementos:

| Componente | Cantidad | Función |
| :---- | :---- | :---- |
| **ESP32 Dev Module** | 1 | Microcontrolador principal con Wi-Fi y Bluetooth integrado. |
| **Salida de Audio** | 1 | (Ej. Altavoz conectado a una etapa de amplificación o filtro RC y transistor si se usa salida PDM). Convierte la señal digital en sonido audible. |
| **Pulsadores Táctiles** | 2 | Uno para activar la reproducción (PLAY) y otro para reiniciar el sistema (RESET). |
| **Protoboard y Cables** | Suficiente | Para realizar las conexiones entre los componentes. |

## **🔌 Diagrama de Conexiones (Cableado)**

A continuación, se detalla cómo deben conectarse los componentes al ESP32. El código utiliza las resistencias internas **INPUT\_PULLUP** del ESP32, lo que simplifica las conexiones al no requerir resistencias físicas externas para los pulsadores.

### **Esquema de Conexión Detallado**

| Componente | Pin del Componente | Pin del ESP32 (GPIO) | Descripción |
| :---- | :---- | :---- | :---- |
| **Pulsador PLAY** | Un terminal | **GPIO 23** | Activa la reproducción aleatoria al ser presionado a **GND**. |
| **Pulsador PLAY** | Otro terminal | **GND (Tierra)** | Cierra el circuito para activar la entrada LOW. |
| **Pulsador RESET** | Un terminal | **GPIO 19** | Reinicia el microcontrolador al ser presionado a **GND**. |
| **Pulsador RESET** | Otro terminal | **GND (Tierra)** | Cierra el circuito. |
|  |  |  |  |
| **Salida de Audio** | LRCK (Word Clock) | **GPIO 22** (I2S/PDM) | Señal de Marco (Frame Clock), necesaria para sincronización. |

⚠️ **Nota Importante sobre Audio:** La biblioteca AudioOutputI2SNoDAC utiliza el pin **GPIO 22** como una interfaz digital I2S. Si no se usa un módulo I2S, necesitarás implementar una etapa de filtro RC y un amplificador con un transistor para convertir la señal de PDM o la modulación de I2S a sonido audible. Para simplificar, asume que conectarás un módulo I2S en el futuro o un circuito de audio compatible con este pin.

## **🧠 Diagrama de Flujo del Programa**

El proyecto opera bajo dos modos de funcionamiento principales: la **Lógica de Reproducción** (prioritaria) y el **Modo de Gestión Web** (activación condicional).

### **1\. Inicialización (setup())**

| Símbolo | Proceso | Descripción |
| :---- | :---- | :---- |
| **(Inicio)** | **INICIO DEL PROGRAMA** | Se enciende el ESP32. |
| **(Proceso)** | Configuración Serial, Pines y Audio | Se inicializa la comunicación serial (115200 bps) y se preparan los pines 23 y 19 como INPUT\_PULLUP. |
| **(Decisión)** | ¿Inicialización de **SPIFFS** Exitosa? | Intenta montar el sistema de archivos. **SI** continúa. **NO** se detiene en un bucle infinito. |
| **(Decisión)** | ¿Existen las frase1.mp3 a frase14.mp3? | La función verificarArchivos() comprueba si todos los MP3 necesarios están presentes. |
| **(Proceso)** | Mensaje: Modo Espera de Botón | **SI** (archivos OK): El sistema prioriza la espera de botones. |
| **(Proceso)** | Mensaje: Faltan Archivos (Modo Advertencia) | **NO** (archivos faltantes): El sistema advierte y el modo web puede ser más urgente. |
| **(Proceso)** | Inicialización de Semilla Aleatoria y Objetos Audio | Se prepara el generador de números aleatorios y las librerías de MP3/I2S. |
| **(Conector)** | **IR a loop()** | Pasa al ciclo principal de ejecución continua. |

### **2\. Ciclo Principal (loop())**

El ciclo loop() es la parte más importante y se ejecuta continuamente, gestionando las tres tareas principales de forma no bloqueante:

graph TD  
    A\[Inicio Loop\] \--\> B{servidor\_web\_activo es TRUE?};  
    B \-- Sí \--\> C\[servidorWeb.handleClient()\];  
    B \-- No \--\> D\[Lectura de Comandos Seriales (leerComandoSerial())\];  
    C \--\> D;  
    D \--\> E\[Gestión de Pulsadores (gestionarPulsadores())\];  
    E \--\> F\[Gestión de Reproducción de Audio (gestionarReproduccionAudio())\];

    F \--\> G{Reproducción en Curso?};  
    G \-- Sí \--\> H{reproductor\_mp3-\>loop() / ¿Terminó el MP3?};  
    H \-- No \--\> A;  
    H \-- Sí \--\> I\[Detener Reproductor y Liberar Recursos\];  
    I \--\> A;

    E \--\> J{Pulsador PLAY Presionado?};  
    J \-- Sí y No Reproduciendo \--\> K\[reproducirFraseAleatoria()\];  
    J \-- No / O Reproduciendo \--\> L\[Pulsador RESET Presionado?\];  
    K \--\> L;  
    L \-- Sí \--\> M\[ESP.restart()\];  
    L \-- No \--\> F;

**Explicación Detallada del Flujo en loop():**

1. **Manejo Web (Handle Client):** Si el servidor web está activo (servidor\_web\_activo \= true), el código da prioridad a la función servidorWeb.handleClient(). Esto permite que el ESP32 responda a las peticiones HTTP (navegadores) para subir o eliminar archivos.  
2. **Comando Serial:** Se verifica si el usuario ha enviado el comando web por el Monitor Serial para activar el servidor web si no lo está.  
3. **Gestión de Botones (gestionarPulsadores()):** Se verifica el estado de los pines 23 (PLAY) y 19 (RESET).  
   * Si se presiona **PLAY** y *no* hay audio en curso, se ejecuta reproducirFraseAleatoria().  
   * Si se presiona **RESET**, se llama a ESP.restart(), que reinicia el microcontrolador.  
4. **Gestión de Audio (gestionarReproduccionAudio()):** Si la bandera reproduccion\_en\_curso es true, el código llama repetidamente a reproductor\_mp3-\>loop().  
   * Esta función es la clave: procesa pequeñas porciones del archivo MP3. Si loop() devuelve false, significa que el archivo ha terminado, se detiene el reproductor y se restablece la bandera reproduccion\_en\_curso a false.

Este ciclo garantiza que el ESP32 nunca se "congele" esperando a que termine una canción o una conexión, manteniendo el sistema receptivo tanto a los botones como a la red.