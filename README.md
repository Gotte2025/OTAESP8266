# OTA por Internet para ESP8266

## Cómo funciona

1. El ESP8266 corre `esp8266_ota_client.ino`, con una `FIRMWARE_VERSION` fija (ej: `"1.0.0"`).
2. Cada 10 minutos (y una vez al arrancar) hace un GET a `versionURL`, que apunta a un `version.json` público.
3. Compara la versión del JSON con la propia. Si son distintas, descarga el `.bin` indicado en `url` y se auto-flashea con `ESPhttpUpdate`.
4. Al terminar, el equipo se reinicia solo, ya corriendo el nuevo firmware.

## Publicar una nueva versión (flujo de trabajo)

1. Modificás tu sketch y le subís la versión, por ejemplo:
   ```cpp
   const String FIRMWARE_VERSION = "1.0.1";
   ```
2. Compilás en Arduino IDE: **Sketch > Export compiled Binary**. Esto genera `firmware.bin`.
3. Subís ese `.bin` a algún hosting público:
   - **GitHub Releases** (recomendado): creás un Release con el `.bin` como asset.
   - **Vercel** o cualquier servidor estático: subís el archivo a una carpeta pública.
4. Editás `version.json` con la nueva versión y la URL directa al `.bin`, y lo subís al mismo repo/hosting.
5. Listo. En el próximo chequeo (máx. 10 min), todos tus equipos en el campo van a detectar la versión nueva, descargarla y actualizarse solos.

## Primera carga

La primerísima vez, el firmware tiene que subirse por USB (con el cable), porque el equipo todavía no tiene código corriendo para hacer OTA. De ahí en adelante, todo lo demás es inalámbrico.

## Notas de seguridad

- El código usa `client->setInsecure()` para simplificar el manejo de certificados HTTPS. Esto **no valida el certificado del servidor**, así que un atacante en una posición de "man in the middle" podría en teoría servir un firmware falso.
- Para producción real te recomiendo:
  - Fijar el certificate fingerprint del servidor (`setFingerprint()`), o
  - Firmar el firmware y verificar la firma antes de aplicar el update (soportado por `Update.h` en el core de ESP8266/ESP32).
- Si vas a tener el `version.json` y el `.bin` en un repo público de GitHub, cualquiera puede ver tu firmware. Si eso te preocupa, usá un repo privado + un servidor propio (Vercel con una API sencilla) en vez de GitHub raw.

## Librerías necesarias

- `ESP8266WiFi` (viene con el core de ESP8266)
- `ESP8266HTTPClient` (viene con el core)
- `ESP8266httpUpdate` (viene con el core)
- `ArduinoJson` (instalar desde el Library Manager)

## Archivos de este paquete

- `esp8266_ota_client.ino` — firmware completo, listo para compilar y subir por USB la primera vez.
- `version.json` — ejemplo del archivo que controla las actualizaciones. Lo editás cada vez que sacás una versión nueva.
