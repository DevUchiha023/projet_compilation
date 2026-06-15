/*
 * ast.c - Implémentation des fonctions de l'AST
 * Projet : Système de Gestion des Notes (SGN)
 * Module : Compilation - Master 1 Informatique
 * Date   : Juin 2026
 *
 * Description : Ce fichier implémente les fonctions de création des nœuds
 * de l'AST, les fonctions d'ajout aux listes chaînées, les calculs
 * (moyennes, mentions, rangs), la génération JSON, et la libération mémoire.
 */

#include "ast.h"

/* ====================================================================
 * SECTION 1 : Gestion des erreurs sémantiques
 * ==================================================================== */

/*
 * ajouter_erreur - Ajoute un message d'erreur à la liste
 * @liste : pointeur vers la liste d'erreurs
 * @msg   : message d'erreur à ajouter
 *
 * Si la liste est pleine (MAX_ERREURS atteint), l'erreur est ignorée.
 */
void ajouter_erreur(ListeErreurs *liste, const char *msg) {
    if (liste->count < MAX_ERREURS) {
        strncpy(liste->messages[liste->count], msg, 255);
        liste->messages[liste->count][255] = '\0';
        liste->count++;
    }
}

/* ====================================================================
 * SECTION 2 : Fonctions de création de nœuds AST
 * ==================================================================== */

/*
 * creer_module - Crée un nœud module
 * @nom  : nom du module (chaîne déjà allouée par strdup dans le lexer)
 * @coef : coefficient du module
 * @note : note obtenue
 * Retourne un pointeur vers le module créé.
 */
Module *creer_module(char *nom, int coef, float note) {
    Module *m = (Module *)malloc(sizeof(Module));
    if (!m) {
        perror("malloc module");
        exit(1);
    }
    m->nom = nom;    /* La chaîne est déjà dupliquée par le lexer */
    m->coef = coef;
    m->note = note;
    m->next = NULL;
    return m;
}

/*
 * creer_semestre - Crée un nœud semestre
 * @id      : identifiant du semestre ("S1".."S6")
 * @modules : liste chaînée des modules du semestre
 * Retourne un pointeur vers le semestre créé.
 */
Semestre *creer_semestre(char *id, Module *modules) {
    Semestre *s = (Semestre *)malloc(sizeof(Semestre));
    if (!s) {
        perror("malloc semestre");
        exit(1);
    }
    s->id = id;
    s->modules = modules;
    s->moyenne = 0.0;
    s->next = NULL;
    return s;
}

/*
 * creer_etudiant - Crée un nœud étudiant
 * @matricule  : matricule unique de l'étudiant
 * @nom        : nom complet
 * @prenom     : prénom
 * @semestres  : liste chaînée des semestres
 * Retourne un pointeur vers l'étudiant créé.
 */
Etudiant *creer_etudiant(char *matricule, char *nom, char *prenom,
                          Semestre *semestres) {
    Etudiant *e = (Etudiant *)malloc(sizeof(Etudiant));
    if (!e) {
        perror("malloc etudiant");
        exit(1);
    }
    e->matricule = matricule;
    e->nom = nom;
    e->prenom = prenom;
    e->semestres = semestres;
    e->moyenne_annuelle = 0.0;
    e->mention = NULL;
    e->decision = NULL;
    e->rang = 0;
    e->next = NULL;
    return e;
}

/*
 * creer_niveau - Crée un nœud niveau
 * @id        : identifiant du niveau ("L1", "L2", "L3")
 * @etudiants : liste chaînée des étudiants du niveau
 * Retourne un pointeur vers le niveau créé.
 */
Niveau *creer_niveau(char *id, Etudiant *etudiants) {
    Niveau *n = (Niveau *)malloc(sizeof(Niveau));
    if (!n) {
        perror("malloc niveau");
        exit(1);
    }
    n->id = id;
    n->etudiants = etudiants;
    n->next = NULL;
    return n;
}

/*
 * creer_programme - Crée le nœud racine du programme
 * @annee   : année académique (ex: "2025-2026")
 * @niveaux : liste chaînée des niveaux
 * Retourne un pointeur vers le programme créé.
 */
Programme *creer_programme(char *annee, Niveau *niveaux) {
    Programme *p = (Programme *)malloc(sizeof(Programme));
    if (!p) {
        perror("malloc programme");
        exit(1);
    }
    p->annee = annee;
    p->niveaux = niveaux;
    return p;
}

/* ====================================================================
 * SECTION 3 : Fonctions d'ajout aux listes chaînées
 * Les éléments sont ajoutés en fin de liste pour préserver l'ordre
 * d'apparition dans le fichier source.
 * ==================================================================== */

Module *ajouter_module(Module *liste, Module *nouveau) {
    if (!liste) return nouveau;
    Module *courant = liste;
    while (courant->next) courant = courant->next;
    courant->next = nouveau;
    return liste;
}

Semestre *ajouter_semestre(Semestre *liste, Semestre *nouveau) {
    if (!liste) return nouveau;
    Semestre *courant = liste;
    while (courant->next) courant = courant->next;
    courant->next = nouveau;
    return liste;
}

Etudiant *ajouter_etudiant(Etudiant *liste, Etudiant *nouveau) {
    if (!nouveau) return liste;  /* Protection contre les nœuds NULL (erreurs) */
    if (!liste) return nouveau;
    Etudiant *courant = liste;
    while (courant->next) courant = courant->next;
    courant->next = nouveau;
    return liste;
}

Niveau *ajouter_niveau(Niveau *liste, Niveau *nouveau) {
    if (!liste) return nouveau;
    Niveau *courant = liste;
    while (courant->next) courant = courant->next;
    courant->next = nouveau;
    return liste;
}

/* ====================================================================
 * SECTION 4 : Fonctions de calcul
 * ==================================================================== */

/*
 * calculer_moyenne_semestre - Calcule la moyenne pondérée d'un semestre
 * Formule : M_s = sum(c_i * n_i) / sum(c_i)
 */
void calculer_moyenne_semestre(Semestre *sem) {
    if (!sem || !sem->modules) return;

    float somme_ponderee = 0.0;
    int somme_coefs = 0;

    Module *m = sem->modules;
    while (m) {
        somme_ponderee += (float)m->coef * m->note;
        somme_coefs += m->coef;
        m = m->next;
    }

    if (somme_coefs > 0) {
        sem->moyenne = somme_ponderee / (float)somme_coefs;
    } else {
        sem->moyenne = 0.0;
    }
}

/*
 * calculer_moyenne_annuelle - Calcule la moyenne annuelle d'un étudiant
 * Formule : M_a = (1/S) * sum(M_s) où S = nombre de semestres
 */
void calculer_moyenne_annuelle(Etudiant *etu) {
    if (!etu || !etu->semestres) return;

    float somme_moyennes = 0.0;
    int nb_semestres = 0;

    Semestre *s = etu->semestres;
    while (s) {
        calculer_moyenne_semestre(s);
        somme_moyennes += s->moyenne;
        nb_semestres++;
        s = s->next;
    }

    if (nb_semestres > 0) {
        etu->moyenne_annuelle = somme_moyennes / (float)nb_semestres;
    } else {
        etu->moyenne_annuelle = 0.0;
    }
}

/*
 * determiner_mention - Détermine la mention et la décision de passage
 * Barème :
 *   >= 16.0 : Admis, Très Bien
 *   >= 14.0 : Admis, Bien
 *   >= 12.0 : Admis, Assez Bien
 *   >= 10.0 : Admis, Passable
 *   <  10.0 : Ajourné, —
 */
void determiner_mention(Etudiant *etu) {
    if (!etu) return;

    float moy = etu->moyenne_annuelle;

    if (moy >= 16.0) {
        etu->mention = "Tres Bien";
        etu->decision = "Admis";
    } else if (moy >= 14.0) {
        etu->mention = "Bien";
        etu->decision = "Admis";
    } else if (moy >= 12.0) {
        etu->mention = "Assez Bien";
        etu->decision = "Admis";
    } else if (moy >= 10.0) {
        etu->mention = "Passable";
        etu->decision = "Admis";
    } else {
        etu->mention = "Insuffisant";
        etu->decision = "Ajourne";
    }
}

/*
 * calculer_rangs - Calcule le rang de chaque étudiant dans un niveau
 * Les étudiants sont triés par moyenne annuelle décroissante.
 * Algorithme : tri par insertion sur un tableau de pointeurs.
 */
void calculer_rangs(Niveau *niv) {
    if (!niv || !niv->etudiants) return;

    /* Compter le nombre d'étudiants dans le niveau */
    int nb = 0;
    Etudiant *e = niv->etudiants;
    while (e) {
        nb++;
        e = e->next;
    }

    if (nb == 0) return;

    /* Créer un tableau de pointeurs vers les étudiants */
    Etudiant **tab = (Etudiant **)malloc(nb * sizeof(Etudiant *));
    if (!tab) {
        perror("malloc rangs");
        exit(1);
    }

    e = niv->etudiants;
    for (int i = 0; i < nb; i++) {
        tab[i] = e;
        e = e->next;
    }

    /* Tri par insertion : moyenne annuelle décroissante */
    for (int i = 1; i < nb; i++) {
        Etudiant *cle = tab[i];
        int j = i - 1;
        while (j >= 0 && tab[j]->moyenne_annuelle < cle->moyenne_annuelle) {
            tab[j + 1] = tab[j];
            j--;
        }
        tab[j + 1] = cle;
    }

    /* Assigner les rangs (1er = meilleure moyenne) */
    for (int i = 0; i < nb; i++) {
        tab[i]->rang = i + 1;
    }

    free(tab);
}

/*
 * calculer_tout - Applique tous les calculs sur le programme entier
 * Pour chaque niveau : calcule les moyennes de chaque étudiant,
 * détermine les mentions, puis calcule les rangs.
 */
void calculer_tout(Programme *prog) {
    if (!prog) return;

    Niveau *niv = prog->niveaux;
    while (niv) {
        /* Calculs individuels pour chaque étudiant */
        Etudiant *etu = niv->etudiants;
        while (etu) {
            calculer_moyenne_annuelle(etu);
            determiner_mention(etu);
            etu = etu->next;
        }

        /* Calcul des rangs dans le niveau */
        calculer_rangs(niv);

        niv = niv->next;
    }
}

/* ====================================================================
 * SECTION 5 : Génération de la sortie JSON
 * Format conforme à la spécification du sujet (section 3.3.2)
 * ==================================================================== */

/*
 * json_print_string - Affiche une chaîne échappée pour JSON
 * Échappe les caractères spéciaux : guillemets, backslash, newline, tab
 */
static void json_print_string(FILE *out, const char *str) {
    fputc('"', out);
    if (str) {
        while (*str) {
            switch (*str) {
                case '"':  fprintf(out, "\\\""); break;
                case '\\': fprintf(out, "\\\\"); break;
                case '\n': fprintf(out, "\\n"); break;
                case '\t': fprintf(out, "\\t"); break;
                default:   fputc(*str, out); break;
            }
            str++;
        }
    }
    fputc('"', out);
}

/*
 * generer_json - Génère la sortie JSON complète sur stdout
 * @prog    : programme racine de l'AST
 * @erreurs : liste des erreurs sémantiques collectées
 *
 * Le JSON est écrit sur stdout pour être capturé par l'interface Python
 * via subprocess.run(capture_output=True).
 */
void generer_json(Programme *prog, ListeErreurs *erreurs) {
    if (!prog) return;

    printf("{\n");

    /* Année académique */
    printf("  \"annee\": ");
    json_print_string(stdout, prog->annee);
    printf(",\n");

    /* Liste des niveaux */
    printf("  \"niveaux\": [\n");

    Niveau *niv = prog->niveaux;
    int first_niv = 1;
    while (niv) {
        if (!first_niv) printf(",\n");
        first_niv = 0;

        printf("    {\n");
        printf("      \"niveau\": ");
        json_print_string(stdout, niv->id);
        printf(",\n");

        /* Liste des étudiants du niveau */
        printf("      \"etudiants\": [\n");

        Etudiant *etu = niv->etudiants;
        int first_etu = 1;
        while (etu) {
            if (!first_etu) printf(",\n");
            first_etu = 0;

            printf("        {\n");

            /* Informations de l'étudiant */
            printf("          \"matricule\": ");
            json_print_string(stdout, etu->matricule);
            printf(",\n");

            printf("          \"nom\": ");
            json_print_string(stdout, etu->nom);
            printf(",\n");

            printf("          \"prenom\": ");
            json_print_string(stdout, etu->prenom);
            printf(",\n");

            /* Liste des semestres de l'étudiant */
            printf("          \"semestres\": [\n");

            Semestre *sem = etu->semestres;
            int first_sem = 1;
            while (sem) {
                if (!first_sem) printf(",\n");
                first_sem = 0;

                printf("            {\n");
                printf("              \"id\": ");
                json_print_string(stdout, sem->id);
                printf(",\n");

                /* Liste des modules du semestre */
                printf("              \"modules\": [\n");

                Module *mod = sem->modules;
                int first_mod = 1;
                while (mod) {
                    if (!first_mod) printf(",\n");
                    first_mod = 0;

                    printf("                {\"nom\": ");
                    json_print_string(stdout, mod->nom);
                    printf(", \"coef\": %d, \"note\": %.1f}",
                           mod->coef, mod->note);

                    mod = mod->next;
                }
                printf("\n");

                printf("              ],\n");
                printf("              \"moyenne\": %.2f\n", sem->moyenne);
                printf("            }");

                sem = sem->next;
            }
            printf("\n");

            printf("          ],\n");

            /* Résultats calculés */
            printf("          \"moyenne_annuelle\": %.2f,\n",
                   etu->moyenne_annuelle);
            printf("          \"mention\": ");
            json_print_string(stdout, etu->mention);
            printf(",\n");
            printf("          \"decision\": ");
            json_print_string(stdout, etu->decision);
            printf(",\n");
            printf("          \"rang\": %d\n", etu->rang);

            printf("        }");

            etu = etu->next;
        }
        printf("\n");

        printf("      ]\n");
        printf("    }");

        niv = niv->next;
    }
    printf("\n");

    printf("  ],\n");

    /* Erreurs sémantiques */
    printf("  \"erreurs\": [");
    if (erreurs && erreurs->count > 0) {
        printf("\n");
        for (int i = 0; i < erreurs->count; i++) {
            printf("    ");
            json_print_string(stdout, erreurs->messages[i]);
            if (i < erreurs->count - 1) printf(",");
            printf("\n");
        }
        printf("  ");
    }
    printf("]\n");

    printf("}\n");
}

/* ====================================================================
 * SECTION 6 : Libération mémoire
 * ==================================================================== */

/* Libère une liste chaînée de modules */
static void liberer_modules(Module *m) {
    while (m) {
        Module *next = m->next;
        free(m->nom);
        free(m);
        m = next;
    }
}

/* Libère une liste chaînée de semestres */
static void liberer_semestres(Semestre *s) {
    while (s) {
        Semestre *next = s->next;
        free(s->id);
        liberer_modules(s->modules);
        free(s);
        s = next;
    }
}

/* Libère une liste chaînée d'étudiants */
static void liberer_etudiants(Etudiant *e) {
    while (e) {
        Etudiant *next = e->next;
        free(e->matricule);
        free(e->nom);
        free(e->prenom);
        /* mention et decision sont des littéraux de chaîne, pas de free */
        liberer_semestres(e->semestres);
        free(e);
        e = next;
    }
}

/* Libère une liste chaînée de niveaux */
static void liberer_niveaux(Niveau *n) {
    while (n) {
        Niveau *next = n->next;
        free(n->id);
        liberer_etudiants(n->etudiants);
        free(n);
        n = next;
    }
}

/*
 * liberer_programme - Libère l'intégralité de l'AST
 * Parcourt récursivement toutes les sous-structures et les libère.
 */
void liberer_programme(Programme *prog) {
    if (!prog) return;
    free(prog->annee);
    liberer_niveaux(prog->niveaux);
    free(prog);
}
