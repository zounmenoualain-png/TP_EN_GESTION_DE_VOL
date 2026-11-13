// ============================================================================
//  Agence de voyage de Cotonou - Gestion des passagers, vols, reservations,
//  paiements et annulations (application console, C++17)
//
//  Compilation :  g++ -std=c++17 -Wall -Wextra -o agence main.cpp
//  Execution   :  ./agence
// ============================================================================
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ----------------------------------------------------------------------------
//  Enumerations
// ----------------------------------------------------------------------------
enum class StatutReservation { EN_ATTENTE, PAYEE, ANNULEE };
enum class StatutRemboursement { AUCUN, REMBOURSE };
enum class ModePaiement { MOBILE_MONEY, CARTE_BANCAIRE, ESPECES };

static string toString(StatutReservation s) {
    switch (s) {
        case StatutReservation::EN_ATTENTE: return "EN ATTENTE DE PAIEMENT";
        case StatutReservation::PAYEE:      return "PAYEE (place definitivement reservee)";
        case StatutReservation::ANNULEE:    return "ANNULEE";
    }
    return "?";
}
static string toString(StatutRemboursement s) {
    return s == StatutRemboursement::REMBOURSE ? "REMBOURSE" : "Aucun remboursement";
}
static string toString(ModePaiement m) {
    switch (m) {
        case ModePaiement::MOBILE_MONEY:   return "Mobile Money";
        case ModePaiement::CARTE_BANCAIRE: return "Carte bancaire";
        case ModePaiement::ESPECES:        return "Especes";
    }
    return "?";
}

// ----------------------------------------------------------------------------
//  Utilitaires de saisie / affichage
// ----------------------------------------------------------------------------
static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static string upper(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return toupper(c); });
    return s;
}
static string lower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
    return s;
}
static string fcfa(double v) {
    ostringstream o;
    long long n = static_cast<long long>(v);
    string digits = to_string(n), res;
    int c = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        res.insert(res.begin(), digits[i]);
        if (++c % 3 == 0 && i > 0) res.insert(res.begin(), ' ');
    }
    return res + " FCFA";
}
static void ligne(char c = '-', int n = 64) { cout << string(n, c) << "\n"; }
static void titre(const string& t) {
    cout << "\n";
    ligne('=');
    cout << "  " << t << "\n";
    ligne('=');
}

[[noreturn]] static void finEntree() {
    cout << "\nFin de la saisie. Au revoir !\n";
    exit(0);
}

static string lireTexte(const string& invite) {
    while (true) {
        cout << invite;
        string l;
        if (!getline(cin, l)) finEntree();
        l = trim(l);
        if (!l.empty()) return l;
        cout << "  ! Ce champ est obligatoire.\n";
    }
}
static int lireEntier(const string& invite, int mn, int mx) {
    while (true) {
        string l = lireTexte(invite);
        try {
            size_t pos = 0;
            int v = stoi(l, &pos);
            if (pos == l.size() && v >= mn && v <= mx) return v;
        } catch (...) {}
        cout << "  ! Entrez un nombre entier entre " << mn << " et " << mx << ".\n";
    }
}
static bool lireOuiNon(const string& invite) {
    while (true) {
        string l = lower(lireTexte(invite + " (o/n) : "));
        if (l == "o" || l == "oui") return true;
        if (l == "n" || l == "non") return false;
        cout << "  ! Repondez par 'o' ou 'n'.\n";
    }
}
static bool tousChiffres(const string& s, size_t from, size_t to) {
    for (size_t i = from; i < to; ++i)
        if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}
static string lireDate(const string& invite) {
    while (true) {
        string s = lireTexte(invite);
        if (s.size() == 10 && s[2] == '/' && s[5] == '/' && tousChiffres(s, 0, 2) &&
            tousChiffres(s, 3, 5) && tousChiffres(s, 6, 10)) {
            int j = stoi(s.substr(0, 2)), m = stoi(s.substr(3, 2));
            if (j >= 1 && j <= 31 && m >= 1 && m <= 12) return s;
        }
        cout << "  ! Format attendu : JJ/MM/AAAA (ex : 25/12/2026).\n";
    }
}
static string lireHeure(const string& invite) {
    while (true) {
        string s = lireTexte(invite);
        if (s.size() == 5 && s[2] == ':' && tousChiffres(s, 0, 2) && tousChiffres(s, 3, 5)) {
            int h = stoi(s.substr(0, 2)), m = stoi(s.substr(3, 2));
            if (h < 24 && m < 60) return s;
        }
        cout << "  ! Format attendu : HH:MM (ex : 14:30).\n";
    }
}
static string lireTelephone(const string& invite) {
    while (true) {
        string s = lireTexte(invite);
        int nb = static_cast<int>(count_if(s.begin(), s.end(),
                    [](unsigned char c) { return isdigit(c); }));
        bool ok = nb >= 8 && all_of(s.begin(), s.end(), [](unsigned char c) {
            return isdigit(c) || c == '+' || c == ' ';
        });
        if (ok) return s;
        cout << "  ! Numero invalide (au moins 8 chiffres, ex : +229 97 00 00 00).\n";
    }
}

// ----------------------------------------------------------------------------
//  Classe Passager
// ----------------------------------------------------------------------------
class Passager {
    int id_;
    string nom_, prenom_, telephone_, passeport_;

public:
    Passager(int id, string nom, string prenom, string telephone, string passeport)
        : id_(id), nom_(move(nom)), prenom_(move(prenom)),
          telephone_(move(telephone)), passeport_(move(passeport)) {}

    int getId() const { return id_; }
    const string& getNom() const { return nom_; }
    const string& getPrenom() const { return prenom_; }
    const string& getTelephone() const { return telephone_; }
    const string& getPasseport() const { return passeport_; }
    string nomComplet() const { return prenom_ + " " + nom_; }
};

class Reservation;  // declaration anticipee (association Vol <-> Reservation)

// ----------------------------------------------------------------------------
//  Classe Vol
// ----------------------------------------------------------------------------
class Vol {
    string numero_, depart_, destination_, date_, heure_;
    int capacite_;
    double prix_;
    vector<Reservation*> reservations_;  // association 1 vol -> 0..* reservations

public:
    Vol(string numero, string depart, string destination, string date, string heure,
        int capacite, double prix)
        : numero_(move(numero)), depart_(move(depart)), destination_(move(destination)),
          date_(move(date)), heure_(move(heure)), capacite_(capacite), prix_(prix) {}

    const string& getNumero() const { return numero_; }
    const string& getDepart() const { return depart_; }
    const string& getDestination() const { return destination_; }
    const string& getDate() const { return date_; }
    const string& getHeure() const { return heure_; }
    int getCapacite() const { return capacite_; }
    double getPrix() const { return prix_; }
    const vector<Reservation*>& getReservations() const { return reservations_; }

    void ajouterReservation(Reservation* r) { reservations_.push_back(r); }

    int placesReservees() const;   // reservations non annulees (en attente + payees)
    int placesConfirmees() const;  // reservations payees (definitives)
    int placesLibres() const { return capacite_ - placesReservees(); }
    bool estComplet() const { return placesLibres() <= 0; }
};

// ----------------------------------------------------------------------------
//  Classe Paiement
// ----------------------------------------------------------------------------
class Paiement {
    int id_;
    double montant_;
    ModePaiement mode_;

public:
    Paiement(int id, double montant, ModePaiement mode)
        : id_(id), montant_(montant), mode_(mode) {}
    int getId() const { return id_; }
    double getMontant() const { return montant_; }
    ModePaiement getMode() const { return mode_; }
};

// ----------------------------------------------------------------------------
//  Classe Reservation (1 passager + 1 vol)
// ----------------------------------------------------------------------------
class Reservation {
    int id_;
    Passager* passager_;
    Vol* vol_;
    StatutReservation statut_;
    StatutRemboursement remboursement_;
    unique_ptr<Paiement> paiement_;

public:
    Reservation(int id, Passager* passager, Vol* vol)
        : id_(id), passager_(passager), vol_(vol),
          statut_(StatutReservation::EN_ATTENTE),
          remboursement_(StatutRemboursement::AUCUN) {
        if (!passager_ || !vol_)
            throw invalid_argument("Une reservation exige un passager et un vol.");
    }

    int getId() const { return id_; }
    Passager* getPassager() const { return passager_; }
    Vol* getVol() const { return vol_; }
    StatutReservation getStatut() const { return statut_; }
    StatutRemboursement getRemboursement() const { return remboursement_; }
    const Paiement* getPaiement() const { return paiement_.get(); }
    bool estActive() const { return statut_ != StatutReservation::ANNULEE; }

    // Regle 5 : une reservation payee ou annulee ne peut plus servir a payer.
    // Regle 6 : la place est definitive apres validation (paiement).
    void payer(int idPaiement, ModePaiement mode) {
        if (statut_ == StatutReservation::PAYEE)
            throw runtime_error("Cette reservation est deja payee.");
        if (statut_ == StatutReservation::ANNULEE)
            throw runtime_error("Cette reservation est annulee : paiement impossible.");
        paiement_ = make_unique<Paiement>(idPaiement, vol_->getPrix(), mode);
        statut_ = StatutReservation::PAYEE;
    }

    // Regle 7 : une reservation payee puis annulee est traitee selon le statut
    // de remboursement prevu par l'application.
    void annuler() {
        if (statut_ == StatutReservation::ANNULEE)
            throw runtime_error("Cette reservation est deja annulee.");
        if (statut_ == StatutReservation::PAYEE)
            remboursement_ = StatutRemboursement::REMBOURSE;
        statut_ = StatutReservation::ANNULEE;
    }

    string numeroBillet() const {
        ostringstream o;
        o << "TKT-" << vol_->getNumero() << "-" << setw(4) << setfill('0') << id_;
        return o.str();
    }

    void afficherBillet() const {
        cout << "\n";
        ligne('*');
        cout << "*" << string(18, ' ') << "BILLET D'AVION ELECTRONIQUE" << string(17, ' ') << "*\n";
        ligne('*');
        cout << "  Billet N°   : " << numeroBillet() << "\n";
        cout << "  Passager    : " << passager_->nomComplet() << "\n";
        cout << "  Passeport   : " << passager_->getPasseport() << "\n";
        cout << "  Vol         : " << vol_->getNumero() << "\n";
        cout << "  Trajet      : " << vol_->getDepart() << " -> " << vol_->getDestination() << "\n";
        cout << "  Depart      : " << vol_->getDate() << " a " << vol_->getHeure() << "\n";
        cout << "  Montant paye: " << fcfa(paiement_->getMontant())
             << " (" << toString(paiement_->getMode()) << ")\n";
        cout << "  Statut      : CONFIRME - bon voyage !\n";
        ligne('*');
    }

    void afficherDetails() const {
        ligne();
        cout << "  Reservation N°  : " << id_ << "\n";
        cout << "  Passager        : " << passager_->nomComplet()
             << " (id " << passager_->getId() << ", tel " << passager_->getTelephone() << ")\n";
        cout << "  Vol             : " << vol_->getNumero() << "  "
             << vol_->getDepart() << " -> " << vol_->getDestination() << "\n";
        cout << "  Depart          : " << vol_->getDate() << " a " << vol_->getHeure() << "\n";
        cout << "  Prix            : " << fcfa(vol_->getPrix()) << "\n";
        cout << "  Statut          : " << toString(statut_) << "\n";
        if (paiement_)
            cout << "  Paiement        : " << fcfa(paiement_->getMontant()) << " via "
                 << toString(paiement_->getMode()) << " (N°" << paiement_->getId() << ")\n";
        if (statut_ == StatutReservation::ANNULEE)
            cout << "  Remboursement   : " << toString(remboursement_) << "\n";
        ligne();
    }
};

int Vol::placesReservees() const {
    return static_cast<int>(count_if(reservations_.begin(), reservations_.end(),
                                     [](const Reservation* r) { return r->estActive(); }));
}
int Vol::placesConfirmees() const {
    return static_cast<int>(count_if(reservations_.begin(), reservations_.end(),
        [](const Reservation* r) { return r->getStatut() == StatutReservation::PAYEE; }));
}

// ----------------------------------------------------------------------------
//  Classe Agence : orchestre toutes les operations
// ----------------------------------------------------------------------------
class Agence {
    string nom_;
    vector<unique_ptr<Passager>> passagers_;
    vector<unique_ptr<Vol>> vols_;
    vector<unique_ptr<Reservation>> reservations_;
    int prochainPassager_ = 1, prochaineReservation_ = 1, prochainPaiement_ = 1;

    Passager* trouverPassager(int id) const {
        for (auto& p : passagers_) if (p->getId() == id) return p.get();
        return nullptr;
    }
    Vol* trouverVol(const string& numero) const {
        string n = upper(numero);
        for (auto& v : vols_) if (v->getNumero() == n) return v.get();
        return nullptr;
    }
    Reservation* trouverReservation(int id) const {
        for (auto& r : reservations_) if (r->getId() == id) return r.get();
        return nullptr;
    }
    bool dejaReserve(const Passager* p, const Vol* v) const {
        for (auto& r : reservations_)
            if (r->estActive() && r->getPassager() == p && r->getVol() == v) return true;
        return false;
    }

    static void enteteVols() {
        cout << left << setw(8) << "Vol" << setw(20) << "Trajet" << setw(12) << "Date"
             << setw(7) << "Heure" << setw(14) << "Prix" << "Places libres\n";
        ligne();
    }
    static void ligneVol(const Vol& v) {
        cout << left << setw(8) << v.getNumero()
             << setw(20) << (v.getDepart() + "->" + v.getDestination())
             << setw(12) << v.getDate() << setw(7) << v.getHeure()
             << setw(14) << fcfa(v.getPrix())
             << v.placesLibres() << "/" << v.getCapacite()
             << " (" << v.placesConfirmees() << " confirmee(s))\n";
    }

    ModePaiement choisirModePaiement() const {
        cout << "  Mode de paiement :\n    1. Mobile Money\n    2. Carte bancaire\n    3. Especes\n";
        int c = lireEntier("  Votre choix (1-3) : ", 1, 3);
        return c == 1 ? ModePaiement::MOBILE_MONEY
             : c == 2 ? ModePaiement::CARTE_BANCAIRE : ModePaiement::ESPECES;
    }

    void payer(Reservation* r) {
        if (r->getStatut() == StatutReservation::PAYEE)
            throw runtime_error("Cette reservation est deja payee.");
        if (r->getStatut() == StatutReservation::ANNULEE)
            throw runtime_error("Cette reservation est annulee : paiement impossible.");
        cout << "\n  Montant a payer : " << fcfa(r->getVol()->getPrix()) << "\n";
        ModePaiement mode = choisirModePaiement();
        r->payer(prochainPaiement_++, mode);
        cout << "\n  >>> Paiement accepte. Votre place est DEFINITIVEMENT reservee.\n";
        r->afficherBillet();
    }

public:
    explicit Agence(string nom) : nom_(move(nom)) {
        // Quelques vols de demonstration au depart de Cotonou
        vols_.push_back(make_unique<Vol>("BJ101", "Cotonou", "Paris", "20/12/2026", "23:45", 3, 450000));
        vols_.push_back(make_unique<Vol>("BJ202", "Cotonou", "Abidjan", "18/12/2026", "09:30", 5, 120000));
        vols_.push_back(make_unique<Vol>("BJ303", "Cotonou", "Casablanca", "22/12/2026", "14:10", 2, 280000));
    }

    const string& getNom() const { return nom_; }

    // --- 1. Enregistrer un passager ---------------------------------------
    Passager* enregistrerPassager() {
        titre("ENREGISTRER UN PASSAGER");
        string nom = lireTexte("  Nom            : ");
        string prenom = lireTexte("  Prenom         : ");
        string tel = lireTelephone("  Telephone      : ");
        string passeport = upper(lireTexte("  N° passeport/CNI : "));
        for (auto& p : passagers_)
            if (p->getPasseport() == passeport)
                throw runtime_error("Un passager avec ce numero de document existe deja (id "
                                    + to_string(p->getId()) + ").");
        passagers_.push_back(make_unique<Passager>(prochainPassager_++, upper(nom), prenom, tel, passeport));
        Passager* p = passagers_.back().get();
        cout << "\n  >>> Passager enregistre. Votre IDENTIFIANT PASSAGER est : " << p->getId() << "\n";
        cout << "      (a conserver pour reserver un billet)\n";
        return p;
    }

    // --- 2. Ajouter un vol -------------------------------------------------
    void ajouterVol() {
        titre("AJOUTER UN VOL");
        string numero = upper(lireTexte("  Numero du vol (ex : BJ404) : "));
        if (trouverVol(numero)) throw runtime_error("Le vol " + numero + " existe deja.");
        string depart = lireTexte("  Ville de depart (ex : Cotonou) : ");
        string dest = lireTexte("  Destination    : ");
        string date = lireDate("  Date (JJ/MM/AAAA) : ");
        string heure = lireHeure("  Heure (HH:MM)  : ");
        int capacite = lireEntier("  Capacite (places) : ", 1, 1000);
        int prix = lireEntier("  Prix du billet (FCFA) : ", 1, 100000000);
        vols_.push_back(make_unique<Vol>(numero, depart, dest, date, heure, capacite, prix));
        cout << "\n  >>> Vol " << numero << " ajoute avec succes.\n";
    }

    // --- 3. Afficher les vols disponibles ---------------------------------
    bool afficherVolsDisponibles() const {
        titre("VOLS DISPONIBLES");
        bool any = false;
        for (auto& v : vols_) {
            if (v->estComplet()) continue;
            if (!any) enteteVols();
            ligneVol(*v);
            any = true;
        }
        if (!any) cout << "  Aucun vol disponible pour le moment.\n";
        return any;
    }

    // --- 4. Rechercher un vol ---------------------------------------------
    void rechercherVol() const {
        titre("RECHERCHER UN VOL");
        string mot = lower(lireTexte("  Numero, destination ou date : "));
        bool any = false;
        for (auto& v : vols_) {
            string cible = lower(v->getNumero() + " " + v->getDepart() + " " +
                                 v->getDestination() + " " + v->getDate());
            if (cible.find(mot) != string::npos) {
                if (!any) enteteVols();
                ligneVol(*v);
                any = true;
            }
        }
        if (!any) cout << "  Aucun vol ne correspond a \"" << mot << "\".\n";
    }

    // --- 5. Effectuer une reservation -------------------------------------
    void effectuerReservation() {
        titre("EFFECTUER UNE RESERVATION");
        int idP = lireEntier("  Votre identifiant passager : ", 1, 1000000);
        Passager* p = trouverPassager(idP);
        if (!p) {
            // Regle 1 : il faut etre enregistre avant de reserver
            cout << "  ! Aucun passager n'a cet identifiant. Il faut d'abord vous enregistrer.\n";
            if (!lireOuiNon("  Voulez-vous vous enregistrer maintenant ?")) return;
            p = enregistrerPassager();
            titre("SUITE DE LA RESERVATION");
        }
        cout << "  Bienvenue " << p->nomComplet() << " !\n";
        if (!afficherVolsDisponibles()) return;

        string numero = lireTexte("\n  Numero du vol a reserver : ");
        Vol* v = trouverVol(numero);
        if (!v) throw runtime_error("Vol introuvable : " + upper(numero));
        // Regle 2 : capacite
        if (v->estComplet()) throw runtime_error("Le vol " + v->getNumero() + " est complet.");
        // Regle 3 : pas de double reservation
        if (dejaReserve(p, v))
            throw runtime_error("Vous avez deja une reservation active sur le vol " + v->getNumero() + ".");

        // Regle 4 : 1 reservation = 1 passager + 1 vol
        reservations_.push_back(make_unique<Reservation>(prochaineReservation_++, p, v));
        Reservation* r = reservations_.back().get();
        v->ajouterReservation(r);

        cout << "\n  >>> Reservation N°" << r->getId() << " creee (en attente de paiement).\n";
        cout << "      Prix a regler : " << fcfa(v->getPrix()) << "\n";
        if (lireOuiNon("\n  Payer maintenant pour obtenir votre billet ?")) payer(r);
        else cout << "  Pensez a payer (menu 6) avec la reservation N°" << r->getId()
                  << " pour obtenir votre billet.\n";
    }

    // --- 6. Enregistrer un paiement ---------------------------------------
    void enregistrerPaiement() {
        titre("ENREGISTRER UN PAIEMENT");
        int id = lireEntier("  N° de reservation : ", 1, 1000000);
        Reservation* r = trouverReservation(id);
        if (!r) throw runtime_error("Reservation introuvable.");
        r->afficherDetails();
        payer(r);  // leve une exception si deja payee / annulee (regle 5)
    }

    // --- 7. Annuler une reservation ---------------------------------------
    void annulerReservation() {
        titre("ANNULER UNE RESERVATION");
        int id = lireEntier("  N° de reservation : ", 1, 1000000);
        Reservation* r = trouverReservation(id);
        if (!r) throw runtime_error("Reservation introuvable.");
        r->afficherDetails();
        if (r->getStatut() == StatutReservation::ANNULEE)
            throw runtime_error("Cette reservation est deja annulee.");
        bool payee = r->getStatut() == StatutReservation::PAYEE;
        if (!lireOuiNon(string("  Confirmer l'annulation") +
                        (payee ? " (un remboursement sera effectue) ?" : " ?"))) {
            cout << "  Annulation abandonnee.\n";
            return;
        }
        r->annuler();
        cout << "\n  >>> Reservation N°" << r->getId() << " annulee. La place est liberee.\n";
        if (payee)
            cout << "  >>> Remboursement de " << fcfa(r->getPaiement()->getMontant())
                 << " : statut " << toString(r->getRemboursement()) << ".\n";
        else
            cout << "  >>> Aucun paiement effectue : rien a rembourser.\n";
    }

    // --- 8. Consulter les reservations ------------------------------------
    void consulterReservations() const {
        titre("LISTE DES RESERVATIONS");
        if (reservations_.empty()) { cout << "  Aucune reservation enregistree.\n"; return; }
        cout << left << setw(6) << "N°" << setw(24) << "Passager" << setw(8) << "Vol"
             << setw(32) << "Statut" << "Remb.\n";
        ligne();
        for (auto& r : reservations_) {
            string st = r->getStatut() == StatutReservation::EN_ATTENTE ? "En attente"
                      : r->getStatut() == StatutReservation::PAYEE ? "Payee" : "Annulee";
            cout << left << setw(6) << r->getId()
                 << setw(24) << r->getPassager()->nomComplet().substr(0, 22)
                 << setw(8) << r->getVol()->getNumero() << setw(32) << st
                 << (r->getRemboursement() == StatutRemboursement::REMBOURSE ? "Rembourse" : "-") << "\n";
        }
    }

    // --- 9. Afficher les informations d'une reservation -------------------
    void afficherInfosReservation() const {
        titre("INFORMATIONS D'UNE RESERVATION");
        int id = lireEntier("  N° de reservation : ", 1, 1000000);
        Reservation* r = trouverReservation(id);
        if (!r) throw runtime_error("Reservation introuvable.");
        r->afficherDetails();
        if (r->getStatut() == StatutReservation::PAYEE) r->afficherBillet();
    }
};

// ----------------------------------------------------------------------------
//  Menu principal
// ----------------------------------------------------------------------------
static void afficherMenu(const string& nom) {
    cout << "\n";
    ligne('#');
    cout << "   " << nom << "\n";
    ligne('#');
    cout << "   1.  Enregistrer un passager\n"
            "   2.  Ajouter un vol\n"
            "   3.  Afficher les vols disponibles\n"
            "   4.  Rechercher un vol\n"
            "   5.  Effectuer une reservation\n"
            "   6.  Enregistrer un paiement\n"
            "   7.  Annuler une reservation\n"
            "   8.  Consulter les reservations\n"
            "   9.  Afficher les informations d'une reservation\n"
            "   10. Quitter\n";
    ligne('#');
}

int main() {
    Agence agence("AGENCE DE VOYAGE DE COTONOU");
    cout << "Bienvenue ! Parcours : enregistrement -> reservation -> paiement -> BILLET.\n";

    while (true) {
        afficherMenu(agence.getNom());
        int choix = lireEntier("Votre choix (1-10) : ", 1, 10);
        try {
            switch (choix) {
                case 1:  agence.enregistrerPassager(); break;
                case 2:  agence.ajouterVol(); break;
                case 3:  agence.afficherVolsDisponibles(); break;
                case 4:  agence.rechercherVol(); break;
                case 5:  agence.effectuerReservation(); break;
                case 6:  agence.enregistrerPaiement(); break;
                case 7:  agence.annulerReservation(); break;
                case 8:  agence.consulterReservations(); break;
                case 9:  agence.afficherInfosReservation(); break;
                case 10: cout << "\nMerci d'avoir utilise l'application. A bientot !\n"; return 0;
            }
        } catch (const exception& e) {
            cout << "\n  [ERREUR] " << e.what() << "\n";
        }
    }
}
