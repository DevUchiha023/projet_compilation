/*
 * main.c - Point d'entrée du programme SGN
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Ce fichier contient la fonction main() qui orchestre
 * l'ensemble du processus d'analyse :
 *   1. Ouverture du fichier d'entrée (.sgn)
 *   2. Lancement de l'analyse lexicale et syntaxique (yyparse)
 *   3. Calcul des moyennes, mentions et rangs
 *   4. Génération de la sortie JSON sur stdout
 *   5. Libération de la mémoire
 *
 * Usage : ./sgn_parser [fichier.sgn]
 * Si aucun fichier n'est spécifié, lit depuis stdin.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "symboles.h"

/* ============================
 * Déclarations externes
 * ============================ */

/* Fonction d'analyse syntaxique générée par Bison */
extern int yyparse(void);

/* Fichier d'entrée pour Flex */
extern FILE *yyin;

/* Résultat de l'analyse : programme racine de l'AST */
extern Programme *programme_racine;

/* Liste des erreurs sémantiques (définie dans parser.y) */
extern ListeErreurs erreurs_sem;

/* Compteurs d'erreurs (définis dans lexer.l et parser.y) */
extern int erreurs_lexicales;
extern int erreurs_syntaxiques;

/* ============================
 * Fonction principale
 * ============================ */

int main(int argc, char *argv[]) {
    /* Initialisation de la table de symboles (doublons matricules) */
    init_table_symboles();

    /* Initialisation de la liste d'erreurs sémantiques */
    erreurs_sem.count = 0;

    /* --- Ouverture du fichier d'entrée --- */
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Erreur : impossible d'ouvrir le fichier '%s'\n",
                    argv[1]);
            return 1;
        }
    } else {
        /* Lecture depuis l'entrée standard */
        yyin = stdin;
    }

    /* --- Lancement de l'analyse lexicale et syntaxique --- */
    yyparse();

    /* --- Fermeture du fichier d'entrée --- */
    if (argc > 1 && yyin) {
        fclose(yyin);
    }

    /* --- Traitement des résultats --- */
    if (programme_racine != NULL &&
        erreurs_lexicales == 0 &&
        erreurs_syntaxiques == 0) {
        /*
         * Analyse réussie (pas d'erreurs lexicales ni syntaxiques).
         * On procède aux calculs même s'il y a des erreurs sémantiques,
         * car l'AST est tout de même construit.
         */

        /* Calcul des moyennes pondérées, moyennes annuelles,
         * mentions et rangs pour chaque étudiant */
        calculer_tout(programme_racine);

        /* Génération de la sortie JSON sur stdout */
        generer_json(programme_racine, &erreurs_sem);

    } else {
        /*
         * Erreurs lexicales ou syntaxiques : l'AST n'est pas fiable.
         * On génère un JSON minimal avec uniquement les erreurs.
         */
        printf("{\n");
        printf("  \"annee\": \"\",\n");
        printf("  \"niveaux\": [],\n");
        printf("  \"erreurs\": [\n");

        int first = 1;

        /* Signaler les erreurs lexicales */
        if (erreurs_lexicales > 0) {
            printf("    \"%d erreur(s) lexicale(s) detectee(s)\"", erreurs_lexicales);
            first = 0;
        }

        /* Signaler les erreurs syntaxiques */
        if (erreurs_syntaxiques > 0) {
            if (!first) printf(",\n");
            printf("    \"%d erreur(s) syntaxique(s) detectee(s)\"", erreurs_syntaxiques);
            first = 0;
        }

        /* Ajouter les erreurs sémantiques éventuelles */
        for (int i = 0; i < erreurs_sem.count; i++) {
            if (!first) printf(",\n");
            printf("    \"%s\"", erreurs_sem.messages[i]);
            first = 0;
        }

        printf("\n  ]\n");
        printf("}\n");
    }

    /* --- Libération de la mémoire --- */
    if (programme_racine) {
        liberer_programme(programme_racine);
    }
    liberer_table_symboles();

    /* Code de retour : 0 si aucune erreur, 1 sinon */
    return (erreurs_lexicales + erreurs_syntaxiques + erreurs_sem.count > 0)
           ? 1 : 0;
}
