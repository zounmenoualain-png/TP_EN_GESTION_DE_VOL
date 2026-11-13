#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

struct Couleur { double r, g, b; };
static const Couleur NOIR{0, 0, 0}, BLANC{1, 1, 1}, MARINE{0.12, 0.23, 0.37},
                     CLAIR{0.92, 0.94, 0.97}, GRIS{0.35, 0.35, 0.35};
enum class Police { NORMALE = 1, GRAS = 2, ITALIQUE = 3 };
enum class Align { GAUCHE, CENTRE };
struct Point { double x, y; };

class Pdf {
    ostringstream c_;
    double larg_, haut_;
    double Y(double y) const { return haut_ - y; }
    void trait(const Couleur& k) { c_ << k.r << ' ' << k.g << ' ' << k.b << " RG\n"; }
    void rempl(const Couleur& k) { c_ << k.r << ' ' << k.g << ' ' << k.b << " rg\n"; }
    void style(double ep, bool tirets) { c_ << ep << " w\n" << (tirets ? "[5 4] 0 d\n" : "[] 0 d\n"); }
    static string echapper(const string& s) {
        string r;
        for (char ch : s) { if (ch == '(' || ch == ')' || ch == '\\') r += '\\'; r += ch; }
        return r;
    }
public:
    Pdf(double l, double h) : larg_(l), haut_(h) { c_ << fixed << setprecision(2); }
    void rect(double x, double y, double w, double h, const Couleur* fond, const Couleur* bord,
              double ep = 1.5, bool tirets = false) {
        c_ << "q\n";
        if (fond) rempl(*fond);
        if (bord) { trait(*bord); style(ep, tirets); }
        c_ << x << ' ' << Y(y + h) << ' ' << w << ' ' << h << " re ";
        c_ << (fond && bord ? "B" : (fond ? "f" : "S")) << "\nQ\n";
    }
    void ligne(double x1, double y1, double x2, double y2, const Couleur& k, double ep = 1.5, bool tirets = false) {
        c_ << "q\n"; trait(k); style(ep, tirets);
        c_ << x1 << ' ' << Y(y1) << " m " << x2 << ' ' << Y(y2) << " l S\nQ\n";
    }
    void polyligne(const vector<Point>& p, const Couleur& k, double ep = 1.5) {
        c_ << "q\n"; trait(k); style(ep, false);
        for (size_t i = 0; i < p.size(); ++i) c_ << p[i].x << ' ' << Y(p[i].y) << (i == 0 ? " m " : " l ");
        c_ << "S\nQ\n";
    }
    void polygone(const vector<Point>& p, const Couleur* fond, const Couleur* bord, double ep = 1.5) {
        c_ << "q\n";
        if (fond) rempl(*fond);
        if (bord) { trait(*bord); style(ep, false); }
        for (size_t i = 0; i < p.size(); ++i) c_ << p[i].x << ' ' << Y(p[i].y) << (i == 0 ? " m " : " l ");
        c_ << "h " << (fond && bord ? "B" : (fond ? "f" : "S")) << "\nQ\n";
    }
    void fleche(double x1, double y1, double x2, double y2, const Couleur& k, bool tirets) {
        ligne(x1, y1, x2, y2, k, 1.1, tirets);
        double a = atan2(y2 - y1, x2 - x1), t = 9;
        polygone({{x2, y2}, {x2 - t * cos(a - 0.4), y2 - t * sin(a - 0.4)},
                  {x2 - t * cos(a + 0.4), y2 - t * sin(a + 0.4)}}, &k, nullptr);
    }
    void texte(double x, double y, const string& s, double taille, Police p = Police::NORMALE,
               const Couleur& k = NOIR, Align al = Align::GAUCHE) {
        double w = s.size() * taille * (p == Police::GRAS ? 0.56 : 0.50);
        if (al == Align::CENTRE) x -= w / 2;
        c_ << "BT\n/F" << static_cast<int>(p) << ' ' << taille << " Tf\n";
        rempl(k);
        c_ << x << ' ' << Y(y) << " Td (" << echapper(s) << ") Tj\nET\n";
    }
    void enregistrer(const string& fichier) const {
        string contenu = c_.str();
        vector<string> obj;
        ostringstream mb; mb << fixed << setprecision(0) << "[0 0 " << larg_ << ' ' << haut_ << ']';
        obj.push_back("<< /Type /Catalog /Pages 2 0 R >>");
        obj.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
        obj.push_back("<< /Type /Page /Parent 2 0 R /MediaBox " + mb.str() +
                      " /Resources << /Font << /F1 5 0 R /F2 6 0 R /F3 7 0 R >> >> /Contents 4 0 R >>");
        obj.push_back("<< /Length " + to_string(contenu.size()) + " >>\nstream\n" + contenu + "endstream");
        obj.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>");
        obj.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>");
        obj.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Oblique /Encoding /WinAnsiEncoding >>");
        string pdf = "%PDF-1.4\n";
        vector<size_t> pos;
        for (size_t i = 0; i < obj.size(); ++i) {
            pos.push_back(pdf.size());
            pdf += to_string(i + 1) + " 0 obj\n" + obj[i] + "\nendobj\n";
        }
        size_t xref = pdf.size();
        pdf += "xref\n0 " + to_string(obj.size() + 1) + "\n0000000000 65535 f \n";
        for (size_t p : pos) { ostringstream l; l << setw(10) << setfill('0') << p << " 00000 n \n"; pdf += l.str(); }
        pdf += "trailer\n<< /Size " + to_string(obj.size() + 1) + " /Root 1 0 R >>\nstartxref\n" +
               to_string(xref) + "\n%%EOF\n";
        ofstream f(fichier, ios::binary);
        if (!f) throw runtime_error("impossible d'ecrire " + fichier);
        f.write(pdf.data(), static_cast<streamsize>(pdf.size()));
    }
};

struct ClasseUML {
    string nom; vector<string> attributs, methodes; double x, y, w;
    double hauteur() const { return 30 + 18 * attributs.size() + 10 + 18 * methodes.size() + 10; }
};
struct RelationUML { string a, b, multA, multB, libelle; bool agregation; };
struct EnumUML {
    string nom; vector<string> valeurs; double x, y;
    double hauteur() const { return 40 + 18 * valeurs.size(); }
};

class DiagrammeClasses {
    vector<ClasseUML> classes_; vector<RelationUML> relations_; vector<EnumUML> enums_;
    const ClasseUML& classe(const string& n) const {
        for (auto& c : classes_) if (c.nom == n) return c;
        throw runtime_error("classe inconnue : " + n);
    }
    void dessinerClasse(Pdf& p, const ClasseUML& c) const {
        p.rect(c.x, c.y, c.w, c.hauteur(), &BLANC, &MARINE, 1.8);
        p.rect(c.x, c.y, c.w, 30, &MARINE, &MARINE, 1.8);
        p.texte(c.x + c.w / 2, c.y + 21, c.nom, 15, Police::GRAS, BLANC, Align::CENTRE);
        double y = c.y + 30 + 18;
        for (auto& a : c.attributs) { p.texte(c.x + 8, y - 5, a, 11); y += 18; }
        double ySep = c.y + 30 + 18 * c.attributs.size() + 5;
        p.ligne(c.x, ySep, c.x + c.w, ySep, MARINE, 1.0);
        y = ySep + 18;
        for (auto& m : c.methodes) { p.texte(c.x + 8, y - 5, m, 11); y += 18; }
    }
    void dessinerRelation(Pdf& p, const RelationUML& r) const {
        const ClasseUML& A = classe(r.a);
        const ClasseUML& B = classe(r.b);
        if (r.agregation) {
            double ca = A.x + A.w / 2, cb = B.x + B.w / 2;
            double sx = ca + (cb < ca - 10 ? -90 : (cb > ca + 10 ? 90 : 0));
            double yA = A.y + A.hauteur(), yMid = yA + 40;
            p.polyligne({{sx, yA + 22}, {sx, yMid}, {cb, yMid}, {cb, B.y}}, MARINE);
            p.polygone({{sx, yA}, {sx + 8, yA + 11}, {sx, yA + 22}, {sx - 8, yA + 11}}, &BLANC, &MARINE);
            p.texte(cb + 6, B.y - 8, r.multB, 12, Police::GRAS);
        } else if (A.y == B.y) {
            bool aGauche = A.x < B.x;
            const ClasseUML& G = aGauche ? A : B;
            const ClasseUML& D = aGauche ? B : A;
            double y = A.y + 110, x1 = G.x + G.w, x2 = D.x;
            p.ligne(x1, y, x2, y, MARINE);
            p.texte(x1 + 6, y - 6, aGauche ? r.multA : r.multB, 12, Police::GRAS);
            p.texte(x2 - 32, y - 6, aGauche ? r.multB : r.multA, 12, Police::GRAS);
            p.texte((x1 + x2) / 2, y + 18, r.libelle, 12, Police::ITALIQUE, NOIR, Align::CENTRE);
        } else {
            double x = A.x + A.w / 2, y1 = A.y + A.hauteur(), y2 = B.y;
            p.ligne(x, y1, x, y2, MARINE);
            p.texte(x + 8, y1 + 18, r.multA, 12, Police::GRAS);
            p.texte(x + 8, y2 - 8, r.multB, 12, Police::GRAS);
            p.texte(x + 40, (y1 + y2) / 2 + 4, r.libelle, 12, Police::ITALIQUE);
        }
    }
    void dessinerEnum(Pdf& p, const EnumUML& e) const {
        p.rect(e.x, e.y, 240, e.hauteur(), &CLAIR, &MARINE, 1.2, true);
        p.texte(e.x + 120, e.y + 16, "<<enumeration>>", 10, Police::ITALIQUE, NOIR, Align::CENTRE);
        p.texte(e.x + 120, e.y + 33, e.nom, 12, Police::GRAS, MARINE, Align::CENTRE);
        for (size_t k = 0; k < e.valeurs.size(); ++k) p.texte(e.x + 10, e.y + 54 + 18 * k, e.valeurs[k], 11);
    }
public:
    void ajouter(const ClasseUML& c) { classes_.push_back(c); }
    void ajouter(const RelationUML& r) { relations_.push_back(r); }
    void ajouter(const EnumUML& e) { enums_.push_back(e); }
    void genererPDF(const string& fichier) const {
        Pdf p(1120, 940);
        p.texte(560, 30, "Diagramme de classes UML - Gestion d'une agence de voyage (Cotonou)", 20,
                Police::GRAS, MARINE, Align::CENTRE);
        for (auto& r : relations_) dessinerRelation(p, r);
        for (auto& c : classes_) dessinerClasse(p, c);
        for (auto& e : enums_) dessinerEnum(p, e);
        const ClasseUML& res = classe("Reservation");
        const ClasseUML& pai = classe("Paiement");
        p.fleche(260, 700, res.x, res.y + 120, GRIS, true);
        p.fleche(260, 830, res.x, res.y + 140, GRIS, true);
        p.fleche(840, 777, pai.x + pai.w, 777, GRIS, true);
        p.texte(560, 915, "Regles : 1 reservation = 1 passager + 1 vol ; capacite du vol non depassable ; "
                          "pas de double reservation ; 1 seul paiement par reservation.",
                11, Police::NORMALE, GRIS, Align::CENTRE);
        p.enregistrer(fichier);
    }
};

int main() {
    DiagrammeClasses d;
    d.ajouter(ClasseUML{"Agence",
        {"-nom : string", "-passagers : vector<Passager>", "-vols : vector<Vol>", "-reservations : vector<Reservation>"},
        {"+enregistrerPassager()", "+ajouterVol()", "+afficherVolsDisponibles()", "+rechercherVol()",
         "+effectuerReservation()", "+enregistrerPaiement()", "+annulerReservation()",
         "+consulterReservations()", "+afficherInfosReservation()"}, 380, 55, 320});
    d.ajouter(ClasseUML{"Passager",
        {"-id : int", "-nom : string", "-prenom : string", "-telephone : string", "-passeport : string"},
        {"+getId() : int", "+nomComplet() : string"}, 20, 410, 240});
    d.ajouter(ClasseUML{"Reservation",
        {"-id : int", "-passager : Passager*", "-vol : Vol*", "-statut : StatutReservation",
         "-remboursement : StatutRemboursement"},
        {"+payer(mode)", "+annuler()", "+numeroBillet() : string", "+afficherBillet()"}, 400, 410, 280});
    d.ajouter(ClasseUML{"Vol",
        {"-numero : string", "-depart : string", "-destination : string", "-date : string",
         "-heure : string", "-capacite : int", "-prix : double"},
        {"+placesLibres() : int", "+placesConfirmees() : int", "+estComplet() : bool",
         "+ajouterReservation(r)"}, 840, 410, 240});
    d.ajouter(ClasseUML{"Paiement",
        {"-id : int", "-montant : double", "-mode : ModePaiement"}, {}, 400, 710, 280});

    d.ajouter(EnumUML{"StatutReservation", {"EN_ATTENTE", "PAYEE", "ANNULEE"}, 20, 660});
    d.ajouter(EnumUML{"StatutRemboursement", {"AUCUN", "REMBOURSE"}, 20, 790});
    d.ajouter(EnumUML{"ModePaiement", {"MOBILE_MONEY", "CARTE_BANCAIRE", "ESPECES"}, 840, 730});

    d.ajouter(RelationUML{"Passager", "Reservation", "1", "0..*", "effectue", false});
    d.ajouter(RelationUML{"Vol", "Reservation", "1", "0..*", "concerne", false});
    d.ajouter(RelationUML{"Reservation", "Paiement", "1", "0..1", "regle", false});
    d.ajouter(RelationUML{"Agence", "Passager", "1", "0..*", "", true});
    d.ajouter(RelationUML{"Agence", "Reservation", "1", "0..*", "", true});
    d.ajouter(RelationUML{"Agence", "Vol", "1", "0..*", "", true});

    try {
        d.genererPDF("diagramme_classes.pdf");
        cout << "PDF genere : diagramme_classes.pdf\n";
    } catch (const exception& e) {
        cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}