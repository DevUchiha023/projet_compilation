# ==============================================================
# Makefile - Compilation du projet SGN
# Projet : Système de Gestion des Notes (SGN)
# Module : Compilation - Master 1 Informatique
# Date   : Juin 2026
#
# Description : Compile automatiquement l'analyseur lexical (Flex),
# l'analyseur syntaxique (Bison) et les sources C en un exécutable.
#
# Usage :
#   make          - Compile le projet
#   make clean    - Supprime les fichiers générés
#   make test     - Lance les tests
# ==============================================================

# Compilateur et options
CC      = gcc
CFLAGS  = -Wall -g

# Outils Flex et Bison
LEX     = flex
YACC    = bison

# Nom de l'exécutable
TARGET  = sgn_parser

# Fichiers sources
SRCS    = lex.yy.c parser.tab.c ast.c main.c
HEADERS = ast.h symboles.h parser.tab.h

# ==============================================================
# Règle principale : compilation de l'exécutable
# ==============================================================
all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) lex.yy.c parser.tab.c ast.c main.c -lm

# ==============================================================
# Génération de l'analyseur syntaxique (Bison)
# -d : génère le fichier d'en-tête parser.tab.h
# -v : génère le fichier de diagnostic parser.output
# ==============================================================
parser.tab.c parser.tab.h: parser.y
	$(YACC) -d -v parser.y

# ==============================================================
# Génération de l'analyseur lexical (Flex)
# Dépend de parser.tab.h pour les définitions de tokens
# ==============================================================
lex.yy.c: lexer.l parser.tab.h
	$(LEX) lexer.l

# ==============================================================
# Nettoyage des fichiers générés
# ==============================================================
clean:
	rm -f lex.yy.c parser.tab.c parser.tab.h parser.output $(TARGET)

# ==============================================================
# Lancement des tests
# ==============================================================
test: $(TARGET)
	@echo "=== Test fichier valide ==="
	./$(TARGET) ../tests/test_valide.sgn
	@echo ""
	@echo "=== Test erreurs lexicales ==="
	-./$(TARGET) ../tests/test_erreur_lex.sgn
	@echo ""
	@echo "=== Test erreurs syntaxiques ==="
	-./$(TARGET) ../tests/test_erreur_syn.sgn
	@echo ""
	@echo "=== Test erreurs semantiques ==="
	-./$(TARGET) ../tests/test_erreur_sem.sgn

# ==============================================================
# Phony targets
# ==============================================================
.PHONY: all clean test
