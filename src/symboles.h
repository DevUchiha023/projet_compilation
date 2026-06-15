/*
 * symboles.h - Table de symboles pour la vérification des doublons
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Implémente une table de hachage simple pour vérifier
 * l'unicité des matricules dans un même niveau. Utilisée par l'analyseur
 * syntaxique (Bison) lors des vérifications sémantiques.
 */

#ifndef SYMBOLES_H
#define SYMBOLES_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Taille de la table de hachage */
#define TABLE_SIZE 256

/* ============================
 * Structure d'une entrée de la table
 * ============================ */

typedef struct SymEntry {
    char *matricule;       /* Matricule de l'étudiant */
    char *niveau;          /* Niveau associé (L1, L2, L3) */
    struct SymEntry *next; /* Chaînage pour gestion des collisions */
} SymEntry;

/* Table de hachage globale */
static SymEntry *table_matricules[TABLE_SIZE];

/* ============================
 * Fonction de hachage (DJB2)
 * ============================ */

static unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % TABLE_SIZE;
}

/* ============================
 * Initialisation de la table
 * ============================ */

static void init_table_symboles(void) {
    for (int i = 0; i < TABLE_SIZE; i++)
        table_matricules[i] = NULL;
}

/* ============================
 * Vérification de doublon de matricule
 * Retourne 1 si le matricule existe déjà dans le même niveau, 0 sinon
 * ============================ */

static int verifier_doublon_matricule(const char *matricule, const char *niveau) {
    unsigned int idx = hash_string(matricule);
    SymEntry *entry = table_matricules[idx];

    while (entry) {
        if (strcmp(entry->matricule, matricule) == 0 &&
            strcmp(entry->niveau, niveau) == 0) {
            return 1; /* Doublon trouvé dans le même niveau */
        }
        entry = entry->next;
    }
    return 0; /* Pas de doublon */
}

/* ============================
 * Ajout d'un matricule dans la table
 * ============================ */

static void ajouter_matricule(const char *matricule, const char *niveau) {
    unsigned int idx = hash_string(matricule);

    SymEntry *entry = (SymEntry *)malloc(sizeof(SymEntry));
    if (!entry) {
        perror("malloc");
        exit(1);
    }

    entry->matricule = strdup(matricule);
    entry->niveau = strdup(niveau);
    entry->next = table_matricules[idx]; /* Insertion en tête */
    table_matricules[idx] = entry;
}

/* ============================
 * Libération de la table de symboles
 * ============================ */

static void liberer_table_symboles(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        SymEntry *entry = table_matricules[i];
        while (entry) {
            SymEntry *next = entry->next;
            free(entry->matricule);
            free(entry->niveau);
            free(entry);
            entry = next;
        }
        table_matricules[i] = NULL;
    }
}

#endif /* SYMBOLES_H */
