# Tools Folder

**DO NOT EDIT** — Dieser Ordner enthält interne Build- und Verschlüsselungswerkzeuge, die für die Integrität des Plugins und des Copyright-Schutzes notwendig sind.

Änderungen an Dateien in diesem Ordner können dazu führen, dass das Plugin nicht mehr startet oder der API-Key-/Copyright-Schutz ungültig wird.

## Hinweis zur Sicherheit

Das Generator-Skript `gen_crypto.py` ist **nicht** Teil des öffentlichen Repositories. Es wurde einmalig verwendet, um die XOR-verschlüsselten Byte-Arrays zu erzeugen, die jetzt **hartcodiert** in `src/copyright_crypto.cpp` eingebaut sind.

Das Skript enthält die Klartext-Versionen von API-Key, Copyright, Produktname und externer URL und darf nicht veröffentlicht oder an Benutzer weitergegeben werden.
