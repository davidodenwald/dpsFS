# dpsFS

dpsFS ist ein FUSE-Dateisystem, für das Betriebssystemlabor der HSKA.

## Aufbau der ConainerDatei

| Name       | Superblock | DMAP  | FAT     | Rootverzeichnis | Dateien   |
|------------|------------|-------|---------|-----------------|-----------|
| Größe      | 1          | 128   | 256     | 64              | 65087     |
| Blockindex | 0          | 1-128 | 129-384 | 385-448         | 449-65535 |

### Superblock:

ToDo:

Definieren was hier reigeschreiben wird.

### DMAP:

In der DMAP steht für jeden Block ein Character.
Dabei steht F (Free) für einen freien Block und A (Allocated) für einen belegten Block.

    65536 chars / 512 byte = 128 Blöcke

### FAT:

Es wird an die Stelle der Blocknr die nächste Blocknummer geschrieben.

ToDO: Was steht an der Stelle wenn es keinen Nachfolger gibt?

    (unsigned short = 2 byte)
    65536 Blöcke * 2 byte = 131072 byte
    122880 byte / 512 byte = 256 Blöcke
    256 Addressen pro Block.

    Finde Block: 	        BlockNR / 256
    Finde pos im Block:     (BlockNR % 256) * 2

### Rootverzeichnis:

Pro Datei:

    Name: 255 byte
    Größe: 8 byte
    BenutzerID: 4 byte (st_uid)
    GruppenID: 4 byte (st_gid)
    Berechtigung: 5 byte (st_mode)
    Zeitpunkte:
        letzter Zugriff: 10 byte (st_atime)
        letzte Änderung: 10 byte (st_mtime)
        Inode Änderung: 10 byte (st_ctime)
    Zeiger auf ersten Datenblock: 5 byte

    Gesamt: 311
    
Es ist wahrscheinlich geschickt, pro Datei einen Block zu verwenden.

    64 Dateien * 1 Block = 64 Blöcke

### Datenblöcke:

Es soll Platz für mindestens 30 MB sein:

    33.554.432 byte / 512 byte = 65536 Blöcke
    65536 Blöcke - 1 Block - 128 Blöcke - 256 Blöcke - 64 Blöcke = 65087 Blöcke
    65087 Blöcke × 512 byte = 33324544 byte / (1024^2) byte =~ 31,78 MB