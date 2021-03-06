﻿Dokumentation Betriebssysteme Labor – Gruppe dosa1013, depa1016, odda1011

1. Aufgabenstellung und Vorgaben

1.1 Aufgabenstellung
Die uns gestellte Aufgabe in diesem Labor bestand darin, ein Dateisystem zu erstellen. Dieses soll verwendet werden, um einen Dateiträger zu formatieren und somit Dateien mit den gewöhnlichen Attributen zu unterstützen. Diese Attribute umschließen zum Beispiel den Namen, die Größe, die Zugriffsrechte und die verschiedenen Zeitstempel der Dateien.
Weiterhin soll ermöglicht werden, dass ein Datenträger, der mit dem Dateisystem formatiert wurde, in einem Verzeichnisbaum eingetragen wird. Die Einbindung soll dann in einem freien, wählbarem und leerem Verzeichnis erfolgen, in welchem der Inhalt des Datenträgers anschließend erscheinen soll.

Anstatt wie bei traditionellen Dateisystemen mit Daten auf einem Datenträger zu arbeiten, verwendeten wir, wie vorgegeben, FUSE. FUSE, was für “File System In User Space” steht, hilft einem dabei, wie der Name schon sagt, Dateisysteme zu schreiben ohne dabei auf Kernel Ebene programmieren zu müssen und wird dementsprechend vor allem bei virtuellen Filesystemen verwendet.

Während bei traditionellen Dateisystemen mit Kernel-Programmierung Anfragen aus dem User Space den VFS (Virtual Filesystem Switch) durchlaufen müssen, gefolgt vom Block-Layer, dem Input-Output-Layer, dem Gerätetreiber bis schließlich der Datenträger erreicht wird, wird das bei FUSE Filesystemen vermieden.

Stattdessen werden Anfragen aus dem User Space vom VFS direkt an FUSE weitergegeben, welches sich um die betroffenen Bereiche kümmert. FUSE führt anschließend ein Programm aus, gibt diesem die Anfragen weiter und erhält eine Antwort, die es zum anfragenden Programm weiterleitet. Wodurch das virtuelle Dateisystem sich praktisch ebenfalls im User Space befindet.


Um diesen Workaround zu ermöglichen, mussten für die Verwendung von FUSE entsprechende Operationen implementiert werden. 

Im ersten Aufgabenteil war das Ziel mittels dem Kommando “mkfs.myfs” einen Datenträger zu erstellen und alle zugehörigen nötigen Strukturen.
Beim Erstellen sollen zuvor ausgewählte Files einmalig auf den Datenträger kopiert werden. Nachdem durch FUSE der Datenträger in den Verzeichnisbaum eingebunden wurde, soll es möglich sein Dateien lesen zu können, aber noch nicht das Bearbeiten oder Löschen.
Beim oben beschriebenen Aufgabenteil war vor allem das Design des Dateisystems entscheidend was den Aufbau und die Einteilung verschiedener Elemente angeht.
Anschließend ging es ans Erstellen und Befüllen des Datenträgers durch das Kommando “mkfs.myfs”. Das Einbinden erfolgte über das Kommando “mount.myfs”. Schlussendlich deckten wir mögliche Fehlerquellen mit ausführlichen Testfällen ab.

Das Ziel des zweiten Aufgabenteils war es, ähnlich wie im ersten Aufgabenteil, einen leeren Datenträger mit fester Größer zu erstellen. Nach dem Einbinden des Datenträgers im Verzeichnisbaum durch FUSE, soll diesmal Daten jedoch bearbeitet und gelöscht werden können. Hierfür mussten zusätzlich die FUSE-Operationen zum Anlegen, Ändern, Schreiben und Löschen von Dateien implementiert werden.


Der dritte und letzte Teil der Laborabgabe ist das Anfertigen der Dokumentation, über den von uns verfassten Code.




1.2 Vorgaben
Das zu erstellende Dateisystem soll eine Größe von mindestens 30MB freien Platz für Dateien anbieten. Dementsprechend war die Größe aller Verwaltungsstrukturen zu beachten um sicherzugehen, dass genug Platz für Dateien übrig bleibt.

Als Konstanten wurde zuerst die maximal Länge eines Dateinamens “NAME_LENGTH” auf 255 Charaktere festgelegt. 
Die logische Blockgröße “BLOCK_SIZE” soll 512 Byte sein, die maximale Anzahl an Verzeichniseinträgen “NUM_DIR_ENTRIES” soll 64 sein, genau wie die Anzahl der maximal offenen Dateien pro einer MyFS Containerdatei.

Die Übung soll von 2-4 Studenten durchgeführt werden, im ILIAS werden Templates bereitgestellt, außerdem detaillierte Vorgaben zu den unterschiedlichen Aufgabenstellungen, in denen ebenfalls einzelne Funktionen genauer erläutert werden. Das zugehörige Skript zur eigentlichen Betriebssysteme Vorlesung ist ebenfalls im ILIAS auffindbar und dient dazu tiefreichend potentielle Fragen und Unklarheiten zu klären.

2. Read-Only Filesystem
2.1 Container Datei Aufbau
Grundbestandteil des Filesystems ist die Erstellung der Containerdatei und diese anschließend zu befüllen. Die Containerdatei muss mindestens eine Größe von 30 MB besitzen und diese in 512 Byte große Blöcke aufgeteilt werden. Maximal sollen bis zu 64 Dateien gespeichert werden können. Als Gesamtgröße wählten wir 33.324.544 Bytes welche 65.536 Blöcke entsprechen.
Die Größe setzt sich wie folgt zusammen:

33.554.432 byte / 512 byte = 65536 Blöcke
65536 Blöcke - 1 Block - 128 Blöcke - 256 Blöcke - 64 Blöcke = 65087 Blöcke
65087 Blöcke × 512 byte = 33324544 byte / (1024^2) byte =~ 31,78 MB


Die eigentliche Containerdatei ist dann folgendermaßen angelegt:
Block 0
Enthält den Superblock, in welchem die Anzahl der Dateien gespeichert werden soll

Block 1-128
Enthält die DMAP in der abgelegt werden soll, welche Blöcke frei oder belegt sind. Dabei soll entweder der Character F für free, oder A für allocated gespeichert werden.

Block 129-384
Enthält einen FAT in dem Datenpointer abgelegt werden, welche die nächste Blocknummer beinhaltet. Beim letzten Block wird eine 0 geschrieben.

Block 385-448
Enthält das Rootverzeichnis mit Einträgen für die Dateinamen, Dateigrößen, Benutzer/Gruppen-ID, Zugriffsberechtigungen, Zeitstempel für den letzten Zugriff/Veränderung/Statusänderung und den Zeiger auf den ersten Datenblock.

Block 449-65.535
Datenblöcke der Dateien. Abhängig von der Größe der Datei werden mehrere Blöcke verwendet.












2.2 Container Erstellung und Befüllung

Die Containerdatei wird mit einem Kommandos erstellt mit folgendem Aufbau: "./mkfs.myfs container.bin file1.txt file2.txt". Die einzelnen Dateien zum Befüllen werden dem Kommando beigefügt.















2.3 Mounten und Lesen

Das Mounten des Dateisystems erfolgt durch ein Kommando mit folgendem Aufbau: "./mount.myfs container.bin log.txt <mount-dir> -s"
Dabei ist darauf zu achten, dass absolute Pfade für Containerdatei, Logdatei und Zielordner, der zu mountenden Containerdatei, angegeben werden, "-s" steht für Single-Threaded Mode.
Im Template wurde uns die Datei "mount.myfs.c" mitgegeben die sicherstellt, 




