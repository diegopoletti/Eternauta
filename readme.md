# **Proyecto Eternauta \- Reproductor de Frases Aleatorias con ESP32**

Este proyecto utiliza un microcontrolador ESP32 para reproducir archivos de audio en formato MP3 de forma aleatoria desde una tarjeta SD. La reproducción se inicia con un pulsador y un segundo pulsador permite reiniciar el dispositivo en cualquier momento.

## **Funcionalidad Principal**

* **Reproducción Aleatoria:** Al presionar el pulsador "Play", el sistema selecciona uno de los archivos de audio de la tarjeta SD al azar y lo reproduce.  
* **Botón de Reinicio:** Un segundo pulsador permite reiniciar el microcontrolador en cualquier momento, volviendo al estado inicial.  
* **Basado en Tarjeta SD:** Las frases o audios se almacenan en una tarjeta microSD, lo que permite una fácil actualización y personalización del contenido.  
* **Salida de Audio I2S:** Utiliza el protocolo I2S para una salida de audio que puede ser conectada a un amplificador.  
* **Diseño Robusto:** El esquema de conexiones incluye componentes adicionales para filtrar ruido eléctrico, haciéndolo más confiable para su uso en entornos industriales o con fuentes de alimentación ruidosas.

## **Componentes Necesarios**

### **Hardware**

* Placa de desarrollo ESP32 (ej: ESP32 DEVKIT V1).  
* Módulo lector de tarjetas MicroSD con interfaz SPI.  
* Tarjeta MicroSD (formateada en FAT32).  
* Dos pulsadores (Push Buttons).  
* Módulo con DAC y amplificador integrado (ej: MAX98357A).  
* Componentes para robustez: 2x resistencias de 10kΩ, 2x capacitores cerámicos de 100nF, 1x capacitor electrolítico de 10µF a 100µF.  
* Protoboard y cables de conexión (jumpers).

### **Software**

* **Arduino IDE** (versión 1.8.19 o superior).  
* **Controlador de placas ESP32** para Arduino IDE.  
* **Biblioteca ESP826\_WiFi:** A pesar de su nombre, esta biblioteca es compatible y ampliamente utilizada para proyectos de audio con ESP32. Se puede instalar desde el Gestor de Bibliotecas del Arduino IDE.

## **Diagrama de Flujo del Programa**

El siguiente diagrama ilustra la lógica de funcionamiento del software:

## **Diagrama de Flujo del Programa (Lógica)**

A continuación se describe la secuencia lógica del programa:

1. **INICIO**  
2. **SETUP (Configuración Inicial)**  
   * Inicializar comunicación Serial.  
   * Inicializar pines de pulsadores con resistencia pull-up.  
   * Inicializar tarjeta SD. Si falla, detener ejecución.  
   * Inicializar objetos de audio (Fuente SD, Decodificador MP3, Salida I2S).  
3. **LOOP (Bucle Principal)**  
   * **Gestionar Pulsadores:**  
     * Leer estado del pulsador PLAY.  
     * ¿Fue presionado?  
       * **SÍ:** ¿Hay una reproducción en curso?  
         * **NO:** Llamar a la función `reproducirFraseAleatoria()`.  
         * **SÍ:** No hacer nada.  
     * Leer estado del pulsador RESET.  
     * ¿Fue presionado?  
       * **SÍ:** Enviar mensaje a la consola y reiniciar el ESP32.  
   * **Gestionar Audio:**  
     * ¿Hay una reproducción en curso?  
       * **SÍ:** ¿El archivo de audio ha terminado?  
         * **SÍ:** Detener la reproducción, cerrar el archivo y actualizar el estado a "no en curso".  
         * **NO:** Continuar enviando datos de audio a la salida I2S.  
   * Repetir el LOOP.

## **Diagrama de Conexiones**

Este diagrama muestra las conexiones del circuito, incluyendo los componentes para mejorar la inmunidad al ruido.

## **Diagrama de Conexiones (Descripción Detallada)**

Este es el esquema de conexiones recomendado, incluyendo componentes para mejorar la inmunidad al ruido.

#### **1\. Alimentación Principal**

* **ESP32 3.3V** \-\> Conectar al riel positivo (+) de la protoboard.  
* **ESP32 GND** \-\> Conectar al riel negativo (-) de la protoboard.  
* **Capacitor de bulk (10µF a 100µF):** Conectar entre el riel positivo y negativo de la protoboard para estabilizar la alimentación.

#### **2\. Módulo Lector de Tarjeta SD \-\> ESP32**

* **VCC** \-\> Riel de 3.3V (+)  
* **GND** \-\> Riel de GND (-)  
* **CS** \-\> GPIO 5  
* **SCK** \-\> GPIO 18  
* **MOSI** \-\> GPIO 23  
* **MISO** \-\> GPIO 19

#### **3\. Salida de Audio I2S (ESP32 \-\> Módulo MAX98357A)**

* **VCC (VIN)** \-\> Riel de 3.3V (+)  
* **GND** \-\> Riel de GND (-)  
* **GPIO 22 (DOUT)** \-\> Pin **DIN** del MAX98357A  
* **GPIO 26 (BCLK)** \-\> Pin **BCLK** del MAX98357A  
* **GPIO 25 (LRC)** \-\> Pin **LRC** del MAX98357A

#### **4\. Pulsador PLAY (con filtro de ruido RC)**

* Una pata del pulsador \-\> Riel de GND (-).  
* La otra pata del pulsador \-\> Conectar a **GPIO 4**.  
* **Resistencia Pull-up (10kΩ):** Conectar entre **GPIO 4** y el riel de 3.3V (+).  
* **Capacitor de filtro (100nF):** Conectar entre **GPIO 4** y el riel de GND (-).

#### **5\. Pulsador RESET (con filtro de ruido RC)**

* Una pata del pulsador \-\> Riel de GND (-).  
* La otra pata del pulsador \-\> Conectar a **GPIO 15**.  
* **Resistencia Pull-up (10kΩ):** Conectar entre **GPIO 15** y el riel de 3.3V (+).  
* **Capacitor de filtro (100nF):** Conectar entre **GPIO 15** y el riel de GND (-).

### **Notas sobre el Diseño Robusto**

* **Capacitores de desacoplo (100nF):** Se colocan cerca de los pines de alimentación del ESP32 y del módulo SD para filtrar ruido de alta frecuencia.  
* **Capacitor de bulk (10µF):** Se coloca en la entrada de alimentación principal para estabilizar el voltaje y suplir picos de corriente.  
* **Filtros RC en pulsadores:** La combinación de la resistencia pull-up de 10kΩ y el capacitor de 100nF en cada pulsador forma un filtro paso bajo que elimina rebotes (debounce) y ruido eléctrico captado por los cables.  
* **Cables apantallados:** Para instalaciones en entornos industriales, se recomienda encarecidamente utilizar cables apantallados para las conexiones de los pulsadores y la salida de audio, conectando la malla del cable a GND.

## **Preparación y Uso**

### **1\. Preparar la Tarjeta SD**

* Formatee la tarjeta MicroSD utilizando el sistema de archivos **FAT32**.  
* Copie sus archivos de audio en formato .mp3 a la raíz de la tarjeta SD.  
* Renombre los archivos de forma secuencial: frase1.mp3, frase2.mp3, frase3.mp3, y así sucesivamente.

### **2\. Configurar el Código**

* Abra el archivo Eternauta.ino en el Arduino IDE.  
* Localice la siguiente línea de código:  
  const int CANTIDAD\_TOTAL\_FRASES \= 15;

* **Modifique el número 15** para que coincida exactamente con la cantidad de archivos .mp3 que guardó en la tarjeta SD. Si tiene 25 archivos, la línea debería ser const int CANTIDAD\_TOTAL\_FRASES \= 25;.

### **3\. Cargar el Programa**

* Conecte su placa ESP32 al ordenador.  
* En el Arduino IDE, seleccione la placa ESP32 correcta y el puerto COM correspondiente.  
* Presione el botón "Subir" para compilar y cargar el programa en el microcontrolador.

Una vez cargado, el dispositivo está listo. Puede ver mensajes de estado y depuración abriendo el **Monitor Serie** del Arduino IDE a una velocidad de **115200 baudios**.

### **4\. Frases sugeridas**
"El héroe verdadero de El Eternauta es un héroe colectivo, un grupo humano… el único héroe válido es el héroe en grupo, nunca el héroe individual.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“¿Por qué esperarlo todo de afuera? ¿Acaso no podemos socorrernos a nosotros mismos?” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Lo viejo funciona.” (Oesterheld, 1957, como se cita en SomosOHLALÁ, 2025)​

“Muy pronto esto será como la jungla… todos contra todos.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“La brújula anda bien, lo que se rompió es el mundo.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Todos necesitamos confiar en alguien.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“No podíamos abandonarnos, dejarnos vencer por la desesperanza.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Esto ya lo viví.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Nadie se salva solo.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Ahora no es tiempo de odiar, es tiempo de luchar.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“¿Ir a la guerra y abandonar los seres queridos no se ve asaltado en ningún momento por los presentimientos más pesimistas?” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Todo sobreviviente, ya lo había dicho Favalli, era un enemigo en potencia.” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Es tanto lo que ha pasado, que me cuesta ser el mismo de siempre. Me parece ser otro, que todo esto lo está viendo otro, no yo…” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“No fue un combate. Ni tiempo les dimos de apuntar, pero no pensábamos lo que habíamos hecho. Total, ellos ya no eran hombres, eran simples cuerpos sin inteligencia, esclavizados a los ‘manos’. Además, no era momento de compasiones…” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)​

“Tienes mucho que aprender, Juan Salvo… Tienes que aprender que en el universo hay muchas especies inteligentes, algunas más, otras menos que la especie humana. Que todas tienen algo en común: el espíritu…” (Oesterheld, 1957, como se cita en LMNeuquén, 2025)