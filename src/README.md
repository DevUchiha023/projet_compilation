# Projet Compilation M1 — Système de Gestion des Notes (SGN)
Université Gaston Berger — IPSL — Année 2025-2026

## Compilation
```bash
cd src && make
```
## Utilisation
```bash
./src/sgn_parser fichier.sgn
```
## Structure
- `src/` — Analyseur lexical (Flex), syntaxique (Bison), AST
- `tests/` — Fichiers .sgn de test
- `gui/` — Interface graphique Python
- `rapport/` — Rapport technique PDF
