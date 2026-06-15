/*
 * ast.h - Définition des structures de l'Arbre Syntaxique Abstrait (AST)
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Ce fichier définit les structures de données utilisées
 * pour représenter l'AST du langage .sgn. L'AST est construit par
 * l'analyseur syntaxique (Bison) et utilisé pour les vérifications
 * sémantiques, les calculs et la génération JSON.
 */

#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================
 * Constantes
 * ============================ */

#define MAX_ERREURS 100   /* Nombre maximal d'erreurs sémantiques */

/* ============================
 * Structures de l'AST
 * ============================ */

/* Structure représentant un module d'enseignement */
typedef struct Module {
    char *nom;           /* Nom du module (ex: "Algorithmique") */
    int coef;            /* Coefficient du module (>= 1) */
    float note;          /* Note obtenue (0.0 - 20.0) */
    struct Module *next; /* Pointeur vers le module suivant */
} Module;

/* Structure représentant un semestre */
typedef struct Semestre {
    char *id;              /* Identifiant du semestre ("S1".."S6") */
    Module *modules;       /* Liste chaînée des modules */
    float moyenne;         /* Moyenne pondérée calculée */
    struct Semestre *next; /* Pointeur vers le semestre suivant */
} Semestre;

/* Structure représentant un étudiant */
typedef struct Etudiant {
    char *matricule;        /* Matricule unique (ex: "L1-2025-001") */
    char *nom;              /* Nom complet */
    char *prenom;           /* Prénom */
    Semestre *semestres;    /* Liste chaînée des semestres */
    float moyenne_annuelle; /* Moyenne annuelle calculée */
    const char *mention;    /* Mention obtenue (pointeur vers littéral) */
    const char *decision;   /* Décision : "Admis" ou "Ajourne" */
    int rang;               /* Rang dans le niveau */
    struct Etudiant *next;  /* Pointeur vers l'étudiant suivant */
} Etudiant;

/* Structure représentant un niveau d'études */
typedef struct Niveau {
    char *id;              /* Identifiant du niveau ("L1", "L2", "L3") */
    Etudiant *etudiants;   /* Liste chaînée des étudiants */
    struct Niveau *next;   /* Pointeur vers le niveau suivant */
} Niveau;

/* Structure représentant le programme complet (racine de l'AST) */
typedef struct Programme {
    char *annee;           /* Année académique (ex: "2025-2026") */
    Niveau *niveaux;       /* Liste chaînée des niveaux */
} Programme;

/* Structure pour collecter les erreurs sémantiques */
typedef struct {
    char messages[MAX_ERREURS][256]; /* Messages d'erreur */
    int count;                       /* Nombre d'erreurs collectées */
} ListeErreurs;

/* ============================
 * Fonctions de gestion des erreurs
 * ============================ */

/* Ajouter une erreur sémantique à la liste */
void ajouter_erreur(ListeErreurs *liste, const char *msg);

/* ============================
 * Fonctions de création de nœuds AST
 * ============================ */

/* Création d'un nœud module */
Module *creer_module(char *nom, int coef, float note);

/* Création d'un nœud semestre */
Semestre *creer_semestre(char *id, Module *modules);

/* Création d'un nœud étudiant */
Etudiant *creer_etudiant(char *matricule, char *nom, char *prenom,
                          Semestre *semestres);

/* Création d'un nœud niveau */
Niveau *creer_niveau(char *id, Etudiant *etudiants);

/* Création du programme racine */
Programme *creer_programme(char *annee, Niveau *niveaux);

/* ============================
 * Fonctions d'ajout aux listes chaînées
 * (ajout en fin de liste pour préserver l'ordre d'apparition)
 * ============================ */

Module *ajouter_module(Module *liste, Module *nouveau);
Semestre *ajouter_semestre(Semestre *liste, Semestre *nouveau);
Etudiant *ajouter_etudiant(Etudiant *liste, Etudiant *nouveau);
Niveau *ajouter_niveau(Niveau *liste, Niveau *nouveau);

/* ============================
 * Fonctions de calcul
 * ============================ */

/* Calcul de la moyenne pondérée d'un semestre :
 * M_s = sum(c_i * n_i) / sum(c_i) */
void calculer_moyenne_semestre(Semestre *sem);

/* Calcul de la moyenne annuelle d'un étudiant :
 * M_a = (1/S) * sum(M_s) */
void calculer_moyenne_annuelle(Etudiant *etu);

/* Détermination de la mention et de la décision de passage */
void determiner_mention(Etudiant *etu);

/* Calcul des rangs des étudiants dans un niveau
 * (tri par moyenne annuelle décroissante) */
void calculer_rangs(Niveau *niv);

/* Appliquer tous les calculs sur l'ensemble du programme */
void calculer_tout(Programme *prog);

/* ============================
 * Fonctions de sortie JSON
 * ============================ */

/* Générer la sortie JSON complète sur stdout
 * Format conforme à la spécification (section 3.3.2 du sujet) */
void generer_json(Programme *prog, ListeErreurs *erreurs);

/* ============================
 * Fonctions de libération mémoire
 * ============================ */

/* Libérer le programme complet et toutes ses sous-structures */
void liberer_programme(Programme *prog);

#endif /* AST_H */
