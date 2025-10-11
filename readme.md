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

## **Diagrama de Conexiones**

Este diagrama muestra las conexiones del circuito, incluyendo los componentes para mejorar la inmunidad al ruido.

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