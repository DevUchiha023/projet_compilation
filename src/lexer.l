%{
/*
 * lexer.l - Analyseur lexical pour le langage SGN
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Ce fichier définit les règles lexicales pour reconnaître
 * les tokens du langage .sgn. Il gère les mots-clés réservés, les
 * littéraux (chaînes, entiers, réels), les commentaires, et signale
 * les erreurs lexicales avec le numéro de ligne.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

/* Compteur d'erreurs lexicales */
int erreurs_lexicales = 0;
%}

/* ============================
 * Options Flex
 * ============================ */
%option noyywrap
%option yylineno

%%

 /* ============================================================
  * Commentaires : lignes commençant par "--"
  * Les commentaires sont ignorés silencieusement.
  * ============================================================ */
"--".*          { /* Commentaire ignoré */ }

 /* ============================================================
  * Espaces, tabulations et retours à la ligne
  * Ignorés pour ne pas perturber l'analyse syntaxique.
  * ============================================================ */
[ \t\r\n]+      { /* Espaces et sauts de ligne ignorés */ }

 /* ============================================================
  * Mots-clés réservés du langage SGN
  * Chaque mot-clé retourne son token correspondant.
  * ============================================================ */
"ANNEE"         { return TOK_ANNEE; }
"NIVEAU"        { return TOK_NIVEAU; }
"ETUDIANT"      { return TOK_ETUDIANT; }
"MATRICULE"     { return TOK_MATRICULE; }
"NOM"           { return TOK_NOM; }
"PRENOM"        { return TOK_PRENOM; }
"SEMESTRE"      { return TOK_SEMESTRE; }
"MODULE"        { return TOK_MODULE; }
"COEF"          { return TOK_COEF; }
"NOTE"          { return TOK_NOTE; }

 /* ============================================================
  * Identifiants de niveau : L1, L2, L3
  * La valeur sémantique (yylval.str) contient l'identifiant.
  * ============================================================ */
"L1"            { yylval.str = strdup(yytext); return TOK_ID_NIVEAU; }
"L2"            { yylval.str = strdup(yytext); return TOK_ID_NIVEAU; }
"L3"            { yylval.str = strdup(yytext); return TOK_ID_NIVEAU; }

 /* ============================================================
  * Identifiants de semestre : S1, S2, S3, S4, S5, S6
  * Pattern générique "S" suivi d'un chiffre de 1 à 6.
  * ============================================================ */
"S"[1-6]        { yylval.str = strdup(yytext); return TOK_ID_SEM; }

 /* ============================================================
  * Nombres réels : nombres avec point décimal obligatoire
  * Exemples : 14.5, 9.0, 12.75
  * IMPORTANT : cette règle doit précéder la règle ENTIER
  * pour éviter que "14" dans "14.5" soit reconnu comme ENTIER.
  * ============================================================ */
[0-9]+\.[0-9]+  { yylval.reel = atof(yytext); return TOK_REEL; }

 /* ============================================================
  * Nombres entiers : séquence de chiffres sans point décimal
  * Exemples : 3, 1, 10
  * ============================================================ */
[0-9]+          { yylval.entier = atoi(yytext); return TOK_ENTIER; }

 /* ============================================================
  * Chaînes de caractères : texte entre guillemets doubles
  * Les guillemets sont supprimés de la valeur sémantique.
  * Exemples : "Algorithmique", "DIALLO Mamadou", "2025-2026"
  * ============================================================ */
\"[^\"]*\"      {
                    /* Dupliquer la chaîne sans les guillemets */
                    yylval.str = strdup(yytext + 1);
                    yylval.str[strlen(yylval.str) - 1] = '\0';
                    return TOK_STRING;
                }

 /* ============================================================
  * Symboles de ponctuation
  * Retournés directement comme caractères ASCII.
  * ============================================================ */
"{"             { return '{'; }
"}"             { return '}'; }
":"             { return ':'; }

 /* ============================================================
  * Gestion des erreurs lexicales
  * Tout caractère non reconnu par les règles précédentes
  * génère un message d'erreur avec le numéro de ligne.
  * ============================================================ */
.               {
                    fprintf(stderr,
                        "Erreur lexicale ligne %d : caractere inattendu '%s'\n",
                        yylineno, yytext);
                    erreurs_lexicales++;
                }

%%
