# 💬 ft_irc  
**C++ · Sockets · Multiplexación con select · Protocolo IRC · Makefile**

## 📌 Descripción  
**ft_irc** es un proyecto desarrollado como parte del currículo de la escuela 42. Su objetivo es implementar desde cero un servidor IRC que cumpla con el protocolo estándar, permitiendo a varios clientes conectarse y comunicarse en tiempo real. Este proyecto permite a los estudiantes explorar el funcionamiento de los protocolos de red, el manejo de múltiples conexiones simultáneas y la implementación de un sistema de chat básico mediante sockets.

## ⚙️ Características  
- Servidor IRC basado en el protocolo RFC 1459  
- Soporte para múltiples clientes simultáneamente  
- Uso de sockets TCP y multiplexación de E/S con `select()`  
- Gestión de canales, comandos básicos (/join, /nick, /privmsg, etc.)  
- Control de errores y desconexión limpia de clientes  
- Código modular y conforme al estilo de 42  

## 🛠 Lenguajes y Tecnologías  
- **Lenguaje**: C++  
- **Redes**: Sockets TCP  
- **Multiplexación**: `select()`  
- **Compilación**: Makefile  
- **Protocolo**: IRC (RFC 1459)  
- **Sistemas**: Linux

## 💡 Conceptos de Programación Aplicados  
- Programación de redes en bajo nivel  
- Concurrencia y manejo de múltiples clientes  
- Protocolo IRC y estructuras de mensajes  
- Gestión de errores y robustez del servidor  
- Sistemas y arquitectura cliente-servidor

## 🚀 Compilación y Uso  
1. Clonar el repositorio  
```bash
git clone https://github.com/RikiGuerrero/ft_irc.git
cd ft_irc
```
2. Compilar el servidor
```bash
make
```
3. Ejecutar el servidor IRC
```bash
./ircserv <port> <password>
```
   📌 Ejemplo:
```bash
./ircserv 6667 mypassword
```
4. Conectar un cliente IRC
```bash
nc localhost 6667
```

## 📜 Licencia
Este proyecto es parte del currículo de 42 y sigue las reglas de la escuela.
