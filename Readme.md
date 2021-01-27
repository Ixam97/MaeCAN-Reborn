# <b>MäCAN _Reborn_ </b>

Die zweite Generaton der MäCAN-Komponenten für den [Märklin CAN-Bus][candoku]

- <b>[MäCAN _Bootloader_][bootloader]</b><br>
    <img src="https://img.shields.io/badge/Software-V1.5-007EC6?style=flat-square"/>
- <b>[MäCAN _MP5x16_][mp5x16]</b><br>
    <img src="https://img.shields.io/badge/Hardware-V1.0-FE7D37?style=flat-square"/>
    <img src="https://img.shields.io/badge/Software-V1.4-FE7D37?style=flat-square"/>
- <b>[MäCAN _Dx32_][dx32]</b><br>
    <img src="https://img.shields.io/badge/Hardware-V1.1-FF4D47?style=flat-square"/>
    <img src="https://img.shields.io/badge/Software-V0.4-FF4D47?style=flat-square"/>
## Changelog

### [2020-01-27.1]
#### Hinzugefügt:
- Allgemein | In mcp2515_basic ist nun SLCAN integriert. Ist das Symbol "SLCAN" definiert wird die serielle Schnittstelle als CAN-Interface genutzt. Standard sind 500000 Baud. Hierfür wurde die Interruptbehandlung in mcp2515_basic integriert. Nach `init_mcp2515()` kann mit `readCanFrame()` ausgewertet werden. SLCAN und der physische CAN werden gleichwertig behandelt. Im selben Zuge wurde für CAN-Frames ein Ring-Buffer implementiert. SLCAN ist auch im Bootloader verfügbar.

#### Änderungen:
- [MP5x16][MP5x16] | Die Ansteuerung der Status-LED wurde an Dx32 angeglichen.

### [2020-01-25.1]
#### Hinzugefügt:
- Allgemein | Neue Funktion "sendStatus" in der mcan-Bibliothek zum senden von Statuskanalwerten.
- [Dx32][dx32] | Konfigurationskanal-Werte können abgefragt werden.

### [2021-01-24.1]
#### Änderungen:
- Allgemein | Ein Fehler in der Übertragung von Konfigurationskanälen in der mcan.c wurde behoben.
- [Dx32][dx32] | Basisfunktionalität wurde hergestellt (Belegterkennung mit Timeout und Gleisspannungserkennung).

### [2021-01-05.1]
#### Änderungen:
- Allgemein | Die Firmware aller Koponenten sowie des Bootloaders wurden in einer Atmel Studio-Solution zusammen geführt, um auf gemeinsame Dateien zugreifen zu können.
- Allgemein | Vereinheitlichung von Ordner- und Dateinamen ohne Umlaute.
- Allgemein | Nicht benötigte defines in mcan.h entfernt und für den ATmega1280 hinzugefügt.
- [MP5x16][mp5x16] | Die Heartbeat-LED hat einen Fade-Effekt erhalten.
- [Bootloader][bootloader] | Unterstützung für den Dx32 mit ATmega1280.
- [Dx32][dx32] | Leitungsbenennung für zwei Rückmelde-LEDs an die Portbenennung angepasst.

#### Hinzugefügt:
- [Dx32][dx32] | Erste Testsoftware mit einem Lauflicht und Helligkeitssteuerung für die Rückmelde-LEDs.

### [2020-12-26.2]
#### Änderungen:
- [Bootloader][bootloader] + [MP5x16][mp5x16] | Anpassung an die Compiler-Flags für die CPU-Frequenz, da Delays nicht richtig funktioniert haben.
- [MP5x16][mp5x16] | Kleinere Änderungen, mit denen bisherige Compiler-Warnungen behandelt wurden.

### [2020-12-26.1]
#### Änderungen:
- [Dx32][dx32] | Fehlerhafte Anbindung der externen Taktquelle für die USB-Schnittstelle korrigiert (Hardware V1.1).
- [Bootloader][bootloader] | Makefile hinzugefügt, mit der der Bootloader Außerhalb von Atmel Studio für eines oder alle Boards kompilliert werden kann (Software V1.2).
- [Bootloader][bootloader] | main.c durch maecan-bootloader.c ersetzt.

#### Hinzugefügt:
- [Bootloader][bootloader] | Vorkompillierte Hex-Dateien für Busankoppler und MP5x16 hinzugefügt.

### [2020-12-22.1]
#### Änderungen:
- [MP5x16][mp5x16] | Möglichkeit einer einstellbaren ID über den DIP-Schalter für eine einfachere Adressierung.

#### Hinzugefügt
- [Dx32][dx32] | Erster Upload der Hardware.

### [2020-08-27.2]
#### Änderungen:
- Readme-Dateien angepasst

### [2020-08-27.1]
#### Hinzugefügt:
- [MP5x16][mp5x16] | Erster Upload
- [Bootloader][bootloader] |  Erster Upload

[candoku]: https://www.maerklin.de/fileadmin/media/service/software-updates/cs2CAN-Protokoll-2_0.pdf
[bootloader]: Bootloader
[mp5x16]: MäCAN_MP5x16
[dx32]: MäCAN_Dx32

[//]: # (Orange: #FE7D37, Blau: #007EC6)