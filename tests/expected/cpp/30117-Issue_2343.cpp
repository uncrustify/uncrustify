class Capteur {
public:
    Capteur ();
public:
    float  val_num;
    float  val_num_prec;    // La valeur précédente pour la comparaison
    String tendance;        // La variable text récupérée du flux Internet
    String val_texte;       // La variable text récupérée du flux Internet
};

class Capteur_CO2
    : public Capteur {
public:
    Capteur_CO2() :
                  un_membre_en_plus ( 0 ) {}
public:
    int un_membre_en_plus;
};


class Salon {
public:
    Capteur     temperature;
    Capteur     humidite;
    Capteur     pression;
    Capteur_CO2 CO2;
};


typedef struct Exterieur Exterieur;
struct Exterieur {  // Structure qui regroupe toutes les variables de la station météo
    float  temp_num;
    float  temp_num_prec;   // La valeur précédente pour la comparaison
    int    humidite;
    int    humidite_prec;   // La valeur précédente pour la comparaison
    String temp_tendance;   // La variable text récupérée du flux Internet
    String temp_texte;      // La variable text récupérée du flux Internet
    String humidite_texte;  // La variable text récupérée du flux Internet
    Exterieur () :
                 temp_num ( -99.9 ),
                 temp_num_prec ( -99.9 ),
                 humidite ( 0 ),
                 humidite_prec ( 0 ),
                 temp_tendance ( "up" ),
                 temp_texte ( "" ),
                 humidite_texte ( "" ) {}
};



for ( int i = 3; i < 42; i++ ) {
    Serial.print ( "TEXTE(AC" );
    Serial.print ( i );
    Serial.print ( ";\"0\");\",\";" );
}
#define OLIVE     0x7BE0
#define LIGHTGREY 0xC618
#ifndef _NETATMO_FONCTIONS_WIFI_h
    #define _NETATMO_FONCTIONS_WIFI_h
    #if defined ( ARDUINO ) && ARDUINO >= 100
        #include "arduino.h"
        #if defined ( RORO )
            #define qsijnqsijdn 1323
            // asbdsqhbdsqibd
        #endif
        #define qsijnqsijdn 1323

    #else
        #define qsijnqsijdn 1323

        #include "WProgram.h"
    #endif  // if defined ( ARDUINO ) && ARDUINO >= 100
    #define qsijnqsijdn 1323

#endif  // ifndef _NETATMO_FONCTIONS_WIFI_h

// Essaie de signe=se+szde/szz-sszzd%zdzd
zzez  = { 1, 2, 3 };
toto += 1 + 2 / 9 - 3 / 2;

int fonction ( ( int *zeze ), ( ss ) ) { ksjbshjdbshjdb = 1;}
fonction ( ( &zeze ), ( ss ) );
fonction ();
// Définition des structures de données
typedef struct Exterieur Exterieur;
struct Exterieur {      // Structure qui regroupe toutes les variables de la station météo
    float  temp_num;
    float  temp_num_prec;   // La valeur précédente pour la comparaison
    int    humidite;
    int    humidite_prec;   // La valeur précédente pour la comparaison
    String temp_tendance;   // La variable text récupérée du flux Internet
};
Exterieur tototot = { -99, -99, -99, -99, 99 };

// Température Extérieure
float _Temp_Ext = -99.9;
float _Temp_Ext_Precedente = -99.9;     // La valeur précédente pour la comparaison
String _Temp_Ext_Tendance  = "up";

UTFT myGLCD ( SSD1963_800 = 1, 38, 39, 40, 41 );    // (byte model, int RS, int WR, int CS, int RST, int SER)
UTFT_Geometry geo_myGLCD ( &myGLCD );

const char *jour_semaine[[1], [2]] = {
    "\0",
    "Vendredi\0",
    "Dimanche\0"
};

void Centrer_Nombre_Int_dans_Zone ( int _nbr, int Y, int X1, int X2, int COULEUR );
void Centrer_Nombre_Float_dans_Zone ( float _nbr, int Y, int X1, int X2, int COULEUR );

void Centrer_Nombre_Int_dans_Zone ( int _nbr, int Y, int X1, int X2, int COULEUR ) {
    toto = 1 + 2 / 9 - 3 / 2;
    String _texte = String ( _nbr, 1 );
    if ( X2 > X1 ) {
        X = X1 + ( X2 - X1 + 1 - _texte.length () * myGLCD.getFontXsize () ) / 2;
    }
    else {
        X = X2 + ( X1 - X2 - myGLCD.getFontXsize () ) / 2;
    }
    if ( X <= 0 ) {
        Serial.print ( F ( "-- Erreur dans le fonction Centrer_Nombre_Int_dans_Zone : la valeur calculée de X est négative ou nulle, elle vaut :" ) );
        Serial.println ( X );
        Serial.print ( F ( "Le texte qui génère cette erreur est : " ) );
        Serial.println ( _texte );
    }
    else {
        myGLCD.setColor ( COULEUR );
        myGLCD.printNumI ( _nbr, X, Y );
    }
}

void Texte_Bonjour () {
    myGLCD.setColor ( VGA_AQUA );
    myGLCD.setBackColor ( VGA_TRANSPARENT );



    myGLCD.setFont ( Grotesk32x64 );
    myGLCD.print ( F ( "BONJOUR" ), CENTER, 20 );
    myGLCD.setFont ( BigFont );
    myGLCD.print ( F ( "*** NETATMO AFFICHAGE DEPORTE ***" ), CENTER, 100 );
    myGLCD.print ( F ( "Debut : Mai 2019 / MAJ : Juillet 2019" ), CENTER, 120 );
}



