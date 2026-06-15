%{
/*
 * parser.y - Analyseur syntaxique et sémantique pour le langage SGN
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Ce fichier implémente la grammaire BNF du langage .sgn
 * conforme à la spécification (section 2.2 du sujet). Il construit
 * l'AST via les actions sémantiques et effectue les vérifications :
 *   - Notes dans l'intervalle [0.0, 20.0]
 *   - Coefficients strictement positifs (>= 1)
 *   - Absence de doublon de matricule dans un même niveau
 *   - Cohérence des semestres par rapport au niveau
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symboles.h"

/* ============================
 * Déclarations externes (Flex)
 * ============================ */
extern int yylex(void);
extern int yylineno;
extern FILE *yyin;
extern int erreurs_lexicales;

/* Fonction de gestion des erreurs syntaxiques */
void yyerror(const char *msg);

/* ============================
 * Variables globales
 * ============================ */

/* Programme racine : résultat final de l'analyse */
Programme *programme_racine = NULL;

/* Niveau courant : utilisé pour la vérification sémantique
 * de cohérence semestre/niveau et des doublons de matricule */
char *niveau_courant = NULL;

/* Liste des erreurs sémantiques collectées pendant l'analyse */
ListeErreurs erreurs_sem;

/* Compteur d'erreurs syntaxiques */
int erreurs_syntaxiques = 0;
%}

/* ============================
 * Union des types sémantiques
 * Définit les types possibles pour les valeurs des tokens
 * et des non-terminaux.
 * ============================ */
%union {
    int entier;            /* Valeur d'un nombre entier */
    float reel;            /* Valeur d'un nombre réel */
    char *str;             /* Valeur d'une chaîne de caractères */
    Module *module;        /* Pointeur vers un nœud Module */
    Semestre *semestre;    /* Pointeur vers un nœud Semestre */
    Etudiant *etudiant;    /* Pointeur vers un nœud Etudiant */
    Niveau *niveau;        /* Pointeur vers un nœud Niveau */
    Programme *programme;  /* Pointeur vers le nœud Programme */
}

/* ============================
 * Déclaration des tokens
 * ============================ */

/* Tokens sans valeur sémantique (mots-clés) */
%token TOK_ANNEE TOK_NIVEAU TOK_ETUDIANT TOK_MATRICULE
%token TOK_NOM TOK_PRENOM TOK_SEMESTRE TOK_MODULE TOK_COEF TOK_NOTE

/* Tokens avec valeur sémantique */
%token <str> TOK_ID_NIVEAU   /* "L1", "L2", "L3" */
%token <str> TOK_ID_SEM      /* "S1".."S6" */
%token <str> TOK_STRING       /* Chaîne entre guillemets */
%token <entier> TOK_ENTIER    /* Nombre entier */
%token <reel> TOK_REEL        /* Nombre réel */

/* ============================
 * Types des non-terminaux
 * ============================ */
%type <programme> programme
%type <niveau> liste_niveaux niveau
%type <etudiant> liste_etudiants etudiant
%type <semestre> liste_semestres semestre
%type <module> liste_modules module
%type <reel> valeur_note

/* Point d'entrée de la grammaire */
%start programme

%%

/* ====================================================================
 * RÈGLES DE PRODUCTION
 * Transcription directe de la grammaire BNF (section 2.2 du sujet)
 * avec actions sémantiques pour la construction de l'AST.
 * ==================================================================== */

/*
 * Programme principal
 * <programme> ::= ANNEE STRING <liste_niveaux>
 *
 * Crée le nœud racine de l'AST contenant l'année académique
 * et la liste de tous les niveaux.
 */
programme
    : TOK_ANNEE TOK_STRING liste_niveaux
        {
            $$ = creer_programme($2, $3);
            programme_racine = $$;
        }
    ;

/*
 * Liste des niveaux (au moins un)
 * <liste_niveaux> ::= <niveau> | <liste_niveaux> <niveau>
 */
liste_niveaux
    : niveau
        {
            $$ = $1;
        }
    | liste_niveaux niveau
        {
            $$ = ajouter_niveau($1, $2);
        }
    ;

/*
 * Bloc de niveau
 * <niveau> ::= NIVEAU <id_niveau> '{' <liste_etudiants> '}'
 *
 * Action intermédiaire : sauvegarde du niveau courant pour les
 * vérifications sémantiques des semestres et matricules.
 */
niveau
    : TOK_NIVEAU TOK_ID_NIVEAU
        {
            /* Sauvegarder le niveau courant pour les vérifications
             * sémantiques (cohérence semestre, doublons matricule) */
            niveau_courant = $2;
        }
      '{' liste_etudiants '}'
        {
            /* $2 = TOK_ID_NIVEAU, $5 = liste_etudiants
             * (décalé de 1 à cause de l'action intermédiaire) */
            $$ = creer_niveau($2, $5);
        }
    ;

/*
 * Liste d'étudiants (au moins un)
 * <liste_etudiants> ::= <etudiant> | <liste_etudiants> <etudiant>
 */
liste_etudiants
    : etudiant
        {
            $$ = $1;
        }
    | liste_etudiants etudiant
        {
            $$ = ajouter_etudiant($1, $2);
        }
    ;

/*
 * Bloc étudiant
 * <etudiant> ::= ETUDIANT '{' <champs_etudiant> <liste_semestres> '}'
 *
 * Les champs sont inlinés dans la règle pour simplifier l'accès
 * aux valeurs sémantiques ($5 = matricule, $8 = nom, $11 = prénom).
 *
 * Vérification sémantique : détection des doublons de matricule
 * dans le même niveau via la table de symboles.
 */
etudiant
    : TOK_ETUDIANT '{'
        TOK_MATRICULE ':' TOK_STRING
        TOK_NOM ':' TOK_STRING
        TOK_PRENOM ':' TOK_STRING
        liste_semestres '}'
        {
            /* === Vérification sémantique : doublon de matricule === */
            if (verifier_doublon_matricule($5, niveau_courant)) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "Ligne %d : Doublon de matricule '%s' dans le niveau %s",
                    yylineno, $5, niveau_courant);
                ajouter_erreur(&erreurs_sem, msg);
                fprintf(stderr, "%s\n", msg);
            } else {
                /* Enregistrer le matricule dans la table de symboles */
                ajouter_matricule($5, niveau_courant);
            }

            /* Construction du nœud étudiant */
            /* $5=matricule, $8=nom, $11=prenom, $12=liste_semestres */
            $$ = creer_etudiant($5, $8, $11, $12);
        }
    | TOK_ETUDIANT '{' error '}'
        {
            /* Récupération d'erreur : bloc étudiant mal formé */
            fprintf(stderr,
                "Erreur syntaxique ligne %d : bloc etudiant mal forme\n",
                yylineno);
            erreurs_syntaxiques++;
            $$ = NULL;
        }
    ;

/*
 * Liste de semestres (au moins un)
 * <liste_semestres> ::= <semestre> | <liste_semestres> <semestre>
 */
liste_semestres
    : semestre
        {
            $$ = $1;
        }
    | liste_semestres semestre
        {
            $$ = ajouter_semestre($1, $2);
        }
    ;

/*
 * Bloc semestre
 * <semestre> ::= SEMESTRE <id_sem> '{' <liste_modules> '}'
 *
 * Vérification sémantique : cohérence entre le semestre et le niveau
 *   L1 → S1, S2
 *   L2 → S3, S4
 *   L3 → S5, S6
 */
semestre
    : TOK_SEMESTRE TOK_ID_SEM '{' liste_modules '}'
        {
            /* === Vérification sémantique : cohérence semestre/niveau === */
            if (niveau_courant != NULL) {
                int sem_num = $2[1] - '0'; /* Extraire le numéro (S1→1) */
                int valide = 0;

                if (strcmp(niveau_courant, "L1") == 0 &&
                    (sem_num == 1 || sem_num == 2))
                    valide = 1;
                else if (strcmp(niveau_courant, "L2") == 0 &&
                         (sem_num == 3 || sem_num == 4))
                    valide = 1;
                else if (strcmp(niveau_courant, "L3") == 0 &&
                         (sem_num == 5 || sem_num == 6))
                    valide = 1;

                if (!valide) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                        "Ligne %d : Semestre %s incompatible avec le niveau %s",
                        yylineno, $2, niveau_courant);
                    ajouter_erreur(&erreurs_sem, msg);
                    fprintf(stderr, "%s\n", msg);
                }
            }

            /* Construction du nœud semestre */
            /* $2 = id_sem, $4 = liste_modules */
            $$ = creer_semestre($2, $4);
        }
    ;

/*
 * Liste de modules (au moins un)
 * <liste_modules> ::= <module> | <liste_modules> <module>
 */
liste_modules
    : module
        {
            $$ = $1;
        }
    | liste_modules module
        {
            $$ = ajouter_module($1, $2);
        }
    ;

/*
 * Déclaration d'un module
 * <module> ::= MODULE STRING COEF ENTIER NOTE REEL
 *
 * Vérifications sémantiques :
 *   1. Coefficient >= 1 (entier strictement positif)
 *   2. Note dans l'intervalle [0.0, 20.0]
 *
 * Note : la règle valeur_note accepte TOK_REEL et TOK_ENTIER
 * pour plus de robustesse (ex: "NOTE 14" au lieu de "NOTE 14.0").
 */
module
    : TOK_MODULE TOK_STRING TOK_COEF TOK_ENTIER TOK_NOTE valeur_note
        {
            /* === Vérification sémantique : coefficient >= 1 === */
            if ($4 < 1) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "Ligne %d : Coefficient invalide (%d) pour le module '%s'"
                    " (doit etre >= 1)",
                    yylineno, $4, $2);
                ajouter_erreur(&erreurs_sem, msg);
                fprintf(stderr, "%s\n", msg);
            }

            /* === Vérification sémantique : note dans [0.0, 20.0] === */
            if ($6 < 0.0 || $6 > 20.0) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "Ligne %d : Note invalide (%.2f) pour le module '%s'"
                    " (doit etre entre 0 et 20)",
                    yylineno, $6, $2);
                ajouter_erreur(&erreurs_sem, msg);
                fprintf(stderr, "%s\n", msg);
            }

            /* Construction du nœud module */
            /* $2 = nom, $4 = coef, $6 = note */
            $$ = creer_module($2, $4, $6);
        }
    ;

/*
 * Valeur de note : accepte un réel ou un entier
 * Cela rend l'analyseur plus robuste : "NOTE 14" est accepté
 * au même titre que "NOTE 14.0".
 */
valeur_note
    : TOK_REEL
        {
            $$ = $1;
        }
    | TOK_ENTIER
        {
            $$ = (float)$1;
        }
    ;

%%

/* ====================================================================
 * Fonction de gestion des erreurs syntaxiques
 * Appelée automatiquement par Bison lors d'une erreur de syntaxe.
 * Affiche le message d'erreur avec le numéro de ligne sur stderr.
 * ==================================================================== */

void yyerror(const char *msg) {
    fprintf(stderr, "Erreur syntaxique ligne %d : %s\n", yylineno, msg);
    erreurs_syntaxiques++;
}
