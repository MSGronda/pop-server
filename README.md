# PopServer
Trabajo práctico especial - 1C 2023 - 72.07 - Protocolos de Comunicación
  

## Estructura del proyecto

El directorio raiz tiene 2 subdirectorios y 3 archivos. El primer directorio alberga el informe del trabajo, mientras que el segundo los directorios y archivos fuente del trabajo práctico. El primer archivo es el de .gitignore (configuración del repositorio), el segundo es la licencia y el tercero y útlimo el README

*  **docs** - Contiene el informe del trabajo práctico realizado.

*  **src** - Contiene directorios y archivos fuentes.

* client - Contiene el código fuente y ejecutable del cliente del protocolo m3.

* include - Contiene los archivos principales de include (main.h  y common.h).

* parser - Tiene el código fuente del parser utilizado en el proyecto.

* pop3 - Tiene el código fuente del protocolo pop3 y manejo de usuarios del servidor.

* server - Tiene los archivos que controlan tanto el servidor que implementa pop3 como el de m3.

* utils - Tiene archivos de libreria de utilidad.

- Makefile - makefile principal del proyecto

- Makefile.inc - archivo propio del makefile

- main.c - encargado de levantar el servidor principal

cabe destacar que cada subdirectorio cuenta con un archivo de make propio y un subdirectorio con los propios archivos de include.
## Construcción del proyecto
Para construir el proyecto, desde el directorio principal moverse al subdirectorio `src`  y ejecutar el comando `make all` para generar los archivos objeto y ejecutables. `make clean` para eliminarlos.

## Directorio maildir
Se debe contar con un directorio con mails para cada usuario. El directorio debe sub-directorios, uno para cada cliente con el mismo nombre que el usuario. Dentro de estos sub-directorios se pueden encontrar los mails correspondientes a dicho usuario.

Un ejemplo de la estructura:
```
maildir/
    ├── marcos/
    |     └── mail1
    |
    ├── maximo/
          ├── a.txt
          └── mail2            
```
## Ejecución del proyecto
El archivo con el nombre `server.out` es el enargado de inicializar el servidor principal. Dentro del directorio client, tambien se generara el archivo `client.out` que es el encargado que inicializar el cliente del servidor con el protocolo m3. El servidor tiene varias opciones y argumentos a ser utilizados para la configuración de este, siendo:
* -h : imprime la ayuda y termina la ejecución.
* -p <puerto pop3\> : indica el puerto donde estara escuchando el socket pasivo de pop3. Por defecto 44443.
* -P <puerto configuración\> : indica el puerto donde estara escuchando el socket pasivo del servidor de configuración. Por defecto 55552.
* -f <ruta\> :  indica la ruta absoluta donde se encuentra el diectorio que contiene las distintas casillas de correo. Si no se indica ninguna ruta, no se accedera a ninguna casilla.
* -u <usuario\:contraseña> : genera un usuario dentro del servidor con las credenciales dadas. El limite es 10 usuarios.
* -v : imprime información acerca de la versión del servidor y termina la ejecución.
 ## Conección a los servidores

 * pop3
Para conectarse al servidor principal, es necesario establecer una conexión TCP del estilo de `netcat -C` y enviar como argumento la direccion ip (127.0.0.1) y puerto del socket pasivo donde se ejecuto el servidor con `server.out`.
* m3
Para conectarse al servidor de monitoreo y configuración se debe ejecutar el ejecutable `client.out` en el subdirectorio client (/src/client/) indicando como argumento, primero la dirección ip donde se esta ejecutando el servidor principal (127.0.0.1) y segundo el puerto donde el servidor escucha las conexiones de m3 (por defecto 55552).
## Funciones de pop3 soportadas
A continuación, un listado de ellas (no son sensibles a mayusculas, entre <> obligatorio, [ ] opcional ):
| Comando | Descripción |
|---------|---------|
| - USER <user\>  | - establece el nombre de usuario para iniciar sesion |
| - PASS <pass\>  | - establece la contraseña para el usuario elegido en USER |
| - CAPA  | - devuelve una lista de las funcionalidades disponibles |
| - STAT  | - devuelve la cantidad de mensajes y octetos actuales de la casilla |
| - LIST [msg] | - devuelve la cantidad de mensajes y octetos totales de la casilla o la cantidad de octetos del mensaje elegido |
| - RETR <msg\>  | - devuelve el mensaje elegido |
| - DELE <msg\>  | - borra el mensaje elegido |
| - RSET  | - reincorpora todos los mensajes eliminados |
| - NOOP  | - devuelve un mensaje |
| - QUIT  | - cierra la conexión del usuario |
## Funciones de monitoreo
| Comando | Descripción |
|---------|---------|
| - help  | - devuelve los comandos que se pueden ejecutar |
| - noop  | - devuelve un mensaje |
| - btsent  | - devuelve la cantidad de bytes enviados por el servidor |
| - btrec  | - devuelve la cantidad de bytes recibidos por el servidor |
| - currusers  | - devuelve la cantidad de usuarios conectados en el servidor |
| - hisusers  | - devuelve la cantidad de usuarios que se conectaron al servidor |
| - adduser <usuario\:contraseña> | - agrega un usuario al servidor |
| - quit  | - cierra la conexión del usuario |
