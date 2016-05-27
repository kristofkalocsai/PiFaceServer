# PiFaceServer Dokumentáció

## Előkövetelmények a fordításhoz:
+ Raspberry Pi kereszt-fordító eszközök [telepítése](https://www.raspberrypi.org/documentation/linux/kernel/building.md)
+ Eclipse

## Előkövetelmények a telepítéshez:
+ wiringPi [telepítése](http://wiringpi.com/download-and-install/)

## Működés:
Az alkalmazás inicializálja a wiringPi perifériakönyvtárat, ezután egy létrehoz es poll-listát és egy socketet, amely a 3344 -es porton hallgatózik. Majd végtelen ciklusban figyeli a bejövő kapcsolatokat, és kiszolgálja azokat.

## Kiszolgálás:

`void handle_new_connection()`

Ha nincs eddigi kapcsolat, az újonnan beérkező hívást a poll-listába teszi.

Ha már csatlakozott valaki, a bejövő kapcsolatot visszautasítja, és visszatér.

`void process_read(int csock)`

Ha a poll listából POLLIN érkezik a bejövő kapcsolaton, fogadja és feldolgozza az adatokat, illetve válaszol a kérésekre.
A kérések formátuma: `XYZ\n`, ahol X a következő részben tárgyalt módok egyike, Y a GPIO lábnak megfelelő szám ASCII-ben, Z pedig a kívánt jelszint/lábfunkció

#### Kérések:
+ **A (0x41):**
  + set pin mode:
    + Y: pin
    + Z: ASCII 0 ha kimenet, ASCII 1 ha bemenet
> pl: `A01\n` : a 0-s GPIO lábat kimenetre állítja.


+ **B (0x42):**
  + set pin level:
    + Y: pin
    + Z: 0 ha low, 1 ha high
> pl: `BI0\n` : a 25-ös GPIO lábra magas jelet ad.

+ **C (0x43):**
  + read pin level: Y: pin, Z: omitted!
  + Válasz: egy byte, ami ASCII 1, ha magas, ASCII 0, ha alacsony.
> pl: `C1\n` : beolvassa az 1-es láb szintjét.


+ **D (0x44):**
  + read pin mode: Y: pin, Z: omitted!
  + Válasz: egy byte, ami ASCII 0, ha output, ASCII 1, ha input.
> pl: `DH\n` : beolvassa a 24-es láb funckióját


+ **Y (0x59):**
  + read all pins: Y: omitted!, Z: omitted!
  + Válasz: 64 byte, ahol 2n az n. láb funckiója, és 2n+1 az n. láb szintje.
> pl: `Y\n` : beolvassa az összes láb funkcióját és szintjét.


+ **X (0x58):**
  +exit: Y: omitted!, Z: omitted!
> pl: `X\n` : terminálja a kapcsolatot.
