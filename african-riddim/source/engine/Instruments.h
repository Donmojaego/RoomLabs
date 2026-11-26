#pragma once
// =============================================================================
// African Riddim — les 54 instruments (un par pays de l'index des planches).
// Chaque instrument est une recette de synthèse physique / modale.
// Ce sont des interprétations sonores : plusieurs de ces instruments existent dans plusieurs pays
// et aucune correspondance n'est présentée comme exclusive.
// L'ordre est FIGÉ (paramètre « instrument » des sessions).
// =============================================================================
namespace afr
{
enum class Model { Pluck, Lamella, Bar, Drum, Wind, Bow, MouthBow };
enum class Cat { Cordes, Percussions, Lamellophones, Vents };

struct Inst
{
    const char* id; const char* country; const char* name; Cat cat; Model model;
    int octave;          // décalage d'octave pour que le do central sonne dans le bon registre
    float decay;         // secondes (T60 approx. au do central)
    float bright;        // 0..1 : éclat de l'excitation / perte des aigus
    float buzz;          // 0..1 : grésillement (chevalet, membrane de mirliton, anneaux de la sanza)
    float body;          // fréquence de la résonance de caisse (Hz)
    float bodyQ, bodyGain;
    float pos;           // position d'attaque (cordes) ou mélange des modes
    float detune;        // cents : 2e corde / 2e tuyau
    float special;       // usage selon le modèle (glissé du tambour parlant, bourdon de la cornemuse…)
};

// id, pays, instrument, catégorie, modèle, octave, decay, bright, buzz, body, Q, gain, pos, detune, special
inline constexpr Inst kInst[54] = {
 { "ma", "Maroc", "Guembri", Cat::Cordes, Model::Pluck, -2, 1.4f, .35f, .55f, 110, 1.2f, .9f, .18f, 0, 0 },
 { "dz", "Algérie", "Gasba", Cat::Vents, Model::Wind, 0, 0, .25f, 0, 900, 1.5f, .5f, .9f, 0, 0 },
 { "tn", "Tunisie", "Mezoued", Cat::Vents, Model::Wind, 0, 0, .8f, .2f, 1400, 2, .6f, .1f, 6, 1 },
 { "ly", "Libye", "Zummara", Cat::Vents, Model::Wind, 0, 0, .7f, .1f, 1100, 2, .5f, .15f, 9, 0 },
 { "eg", "Égypte", "Oud", Cat::Cordes, Model::Pluck, -1, 2.2f, .45f, .05f, 160, 1.4f, .8f, .14f, 4, 0 },
 { "sd", "Soudan", "Tanbura", Cat::Cordes, Model::Pluck, -1, 2.4f, .55f, .6f, 140, 1.2f, .8f, .1f, 0, 0 },
 { "mr", "Mauritanie", "Ardin", Cat::Cordes, Model::Pluck, 0, 3.0f, .6f, .15f, 220, 1.3f, .6f, .3f, 0, 0 },
 { "ml", "Mali", "Ngoni", Cat::Cordes, Model::Pluck, 0, 1.6f, .6f, .35f, 260, 1.5f, .7f, .12f, 0, 0 },
 { "sn", "Sénégal", "Sabar", Cat::Percussions, Model::Drum, 0, .5f, .8f, 0, 0, 0, 0, .45f, 0, 0 },
 { "gm", "Gambie", "Kora", Cat::Cordes, Model::Pluck, 0, 3.2f, .65f, .25f, 180, 1.2f, .8f, .2f, 0, 0 },
 { "gn", "Guinée", "Balafon", Cat::Percussions, Model::Bar, 0, .9f, .7f, .55f, 0, 0, 0, .5f, 0, 0 },
 { "gw", "Guinée-Bissau", "Tina", Cat::Percussions, Model::Drum, -1, .6f, .3f, 0, 0, 0, 0, .3f, 0, 3 },
 { "sl", "Sierra Leone", "Kondi", Cat::Lamellophones, Model::Lamella, 0, 1.8f, .55f, .35f, 330, 2, .5f, .5f, 0, 0 },
 { "lr", "Liberia", "Gbi", Cat::Percussions, Model::Bar, -1, .45f, .4f, 0, 0, 0, 0, .2f, 0, 1 },
 { "ci", "Côte d’Ivoire", "Attoungblan", Cat::Percussions, Model::Drum, 0, .5f, .6f, 0, 0, 0, 0, .5f, 0, 1 },
 { "bf", "Burkina Faso", "Bendré", Cat::Percussions, Model::Drum, -1, .8f, .35f, .4f, 0, 0, 0, .4f, 0, 2 },
 { "gh", "Ghana", "Seperewa", Cat::Cordes, Model::Pluck, 0, 2.6f, .6f, .1f, 240, 1.3f, .7f, .2f, 0, 0 },
 { "tg", "Togo", "Kologo", Cat::Cordes, Model::Pluck, -1, 1.2f, .5f, .45f, 200, 1.4f, .8f, .12f, 0, 0 },
 { "bj", "Bénin", "Agogo", Cat::Percussions, Model::Bar, 0, 1.4f, .9f, 0, 0, 0, 0, .8f, 0, 2 },
 { "ne", "Niger", "Goge", Cat::Cordes, Model::Bow, 0, 0, .55f, .3f, 500, 2, .7f, 0, 7, 0 },
 { "ng", "Nigeria", "Dùndún", Cat::Percussions, Model::Drum, 0, .55f, .6f, .1f, 0, 0, 0, .5f, 0, 1 },
 { "td", "Tchad", "Kakaki", Cat::Vents, Model::Wind, -1, 0, .9f, .3f, 700, 1.5f, .7f, .0f, 4, 2 },
 { "cm", "Cameroun", "Mvet", Cat::Cordes, Model::Pluck, 0, 2.0f, .5f, .5f, 200, 1.3f, .8f, .22f, 0, 0 },
 { "cf", "Centrafrique", "Sanza", Cat::Lamellophones, Model::Lamella, 0, 1.5f, .5f, .55f, 280, 2, .6f, .45f, 0, 0 },
 { "cg", "République du Congo", "Ngoma", Cat::Percussions, Model::Drum, -1, .45f, .55f, 0, 0, 0, 0, .55f, 0, 0 },
 { "cd", "RDC", "Likembe", Cat::Lamellophones, Model::Lamella, 0, 1.9f, .6f, .65f, 300, 2, .6f, .5f, 0, 0 },
 { "ga", "Gabon", "Ngombi", Cat::Cordes, Model::Pluck, 0, 2.8f, .55f, .1f, 230, 1.3f, .7f, .25f, 0, 0 },
 { "gq", "Guinée équatoriale", "Mvet", Cat::Cordes, Model::Pluck, 0, 2.2f, .45f, .45f, 180, 1.3f, .8f, .25f, 3, 0 },
 { "st", "São Tomé-et-Príncipe", "Puita", Cat::Percussions, Model::Drum, -1, .5f, .4f, .2f, 0, 0, 0, .3f, 0, 4 },
 { "ao", "Angola", "Kissanje", Cat::Lamellophones, Model::Lamella, 0, 2.2f, .65f, .3f, 320, 2, .5f, .55f, 0, 0 },
 { "cv", "Cabo Verde", "Cavaquinho", Cat::Cordes, Model::Pluck, 1, 1.6f, .85f, 0, 400, 1.4f, .6f, .16f, 6, 0 },
 { "km", "Comores", "Gabusi", Cat::Cordes, Model::Pluck, 0, 1.6f, .5f, .2f, 230, 1.5f, .8f, .2f, 3, 0 },
 { "mg", "Madagascar", "Valiha", Cat::Cordes, Model::Pluck, 1, 3.4f, .8f, .1f, 600, 1.2f, .5f, .2f, 0, 0 },
 { "mu", "Maurice", "Ravanne", Cat::Percussions, Model::Drum, -1, .4f, .55f, .3f, 0, 0, 0, .6f, 0, 5 },
 { "sc", "Seychelles", "Moutya", Cat::Percussions, Model::Drum, -1, .6f, .35f, .2f, 0, 0, 0, .4f, 0, 5 },
 { "mz", "Mozambique", "Timbila", Cat::Percussions, Model::Bar, 0, 1.0f, .6f, .6f, 0, 0, 0, .45f, 0, 0 },
 { "mw", "Malawi", "Bangwe", Cat::Cordes, Model::Pluck, 0, 2.6f, .7f, .35f, 300, 1.3f, .6f, .22f, 0, 0 },
 { "zm", "Zambie", "Silimba", Cat::Percussions, Model::Bar, 0, 1.1f, .65f, .5f, 0, 0, 0, .5f, 0, 0 },
 { "zw", "Zimbabwe", "Mbira", Cat::Lamellophones, Model::Lamella, 0, 2.6f, .6f, .5f, 260, 2, .6f, .5f, 0, 0 },
 { "bw", "Botswana", "Segaba", Cat::Cordes, Model::Bow, -1, 0, .45f, .2f, 400, 2, .8f, 0, 0, 0 },
 { "na", "Namibie", "!Gais-tsab", Cat::Cordes, Model::MouthBow, -1, 2.2f, .5f, .1f, 0, 0, 0, .3f, 0, 0 },
 { "za", "Afrique du Sud", "Uhadi", Cat::Cordes, Model::MouthBow, -1, 2.6f, .45f, .05f, 0, 0, 0, .4f, 0, 1 },
 { "ls", "Lesotho", "Lesiba", Cat::Vents, Model::Wind, 0, 0, .75f, .5f, 900, 3, .7f, .3f, 0, 3 },
 { "sz", "Eswatini", "Makhweyane", Cat::Cordes, Model::MouthBow, -1, 2.4f, .5f, .1f, 0, 0, 0, .35f, 0, 1 },
 { "bi", "Burundi", "Inanga", Cat::Cordes, Model::Pluck, -1, 2.6f, .4f, .3f, 150, 1.2f, .8f, .3f, 0, 0 },
 { "rw", "Rwanda", "Inanga", Cat::Cordes, Model::Pluck, -1, 2.8f, .45f, .25f, 160, 1.2f, .8f, .28f, 0, 0 },
 { "ug", "Ouganda", "Adungu", Cat::Cordes, Model::Pluck, 0, 2.4f, .5f, .1f, 200, 1.3f, .7f, .2f, 0, 0 },
 { "ke", "Kenya", "Nyatiti", Cat::Cordes, Model::Pluck, -1, 2.4f, .6f, .55f, 170, 1.3f, .8f, .15f, 0, 0 },
 { "tz", "Tanzanie", "Zeze", Cat::Cordes, Model::Pluck, 0, 1.4f, .6f, .4f, 280, 1.6f, .8f, .15f, 0, 0 },
 { "et", "Éthiopie", "Krar", Cat::Cordes, Model::Pluck, 0, 2.2f, .7f, .2f, 260, 1.3f, .6f, .18f, 0, 0 },
 { "er", "Érythrée", "Krar", Cat::Cordes, Model::Pluck, 0, 2.0f, .65f, .3f, 240, 1.3f, .6f, .2f, 0, 0 },
 { "dj", "Djibouti", "Kaban", Cat::Cordes, Model::Pluck, -1, 1.9f, .5f, .05f, 170, 1.4f, .8f, .14f, 5, 0 },
 { "so", "Somalie", "Kaban", Cat::Cordes, Model::Pluck, -1, 2.1f, .55f, .05f, 180, 1.4f, .8f, .16f, 4, 0 },
 { "ss", "Soudan du Sud", "Adungu", Cat::Cordes, Model::Pluck, 0, 2.6f, .5f, .15f, 210, 1.3f, .7f, .22f, 0, 0 },
};
inline const char* catName (Cat c) { switch (c) { case Cat::Cordes: return "Cordes"; case Cat::Percussions: return "Percussions"; case Cat::Lamellophones: return "Lamellophones"; default: return "Vents"; } }
} // namespace afr
