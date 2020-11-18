#ifndef VB_I2C_HPP
#define VB_I2C_HPP

#define CLIENT_DATA_ARRAY_SIZE 8
#define SERVER_DATA_ARRAY_SIZE 8

#include <stdint.h>

// Tu peux rajouter des types de paquets
enum SERVER_DATA_TYPE : uint8_t
{
    // On déclare chaque type de paquet envoyé par le serveur
    INIT = 0x0,
    START = 0x1,

    START_TX = 0x2, // Démarrage de la phase de transmission de donnée
    STOP_TX = 0x3,   // Fin de la phase de transmission de donnée

    ABORT_GAME = 0x4,
};

enum CLIENT_DATA_TYPE : uint8_t
{
    // On déclare chaque type de paquet envoyé par le client
    START_ACK = 0x0, // Répond présent à la requête START. N'est pas envoyé ni géré automatiquement. Peut-être une idée pour vérifier que tout marche correctement

    SUCCESS = 0x1,  // Le joueur à réussi l'énigme.
    GAMEOVER = 0x2, // Le joueur à raté l'énigme.

    RUNTIME_ERROR = 0x3, // Une erreur est survenue. Par exemple une erreur de transmission avec un écran etc.

    // ...
};

typedef struct // Données envoyées par le serveur au client
{
    enum SERVER_DATA_TYPE dataType; // Type de paquet
    uint8_t data[31];               // Données, 31 bytes. On peut considérer ce tableau comme un tableau de byte (ce qu'il est, en réalité.)
} SERVER_DATA, *SERVER_DATA_T;

typedef struct // Données à envoyer au serveur
{
    enum CLIENT_DATA_TYPE dataType; // Type de paquet
    uint8_t clientId;
    uint8_t data[30];               // Données, 31 bytes
} CLIENT_DATA, *CLIENT_DATA_T;

class VbI2C
{
public:
    VbI2C(int); // Chaque partie à son identifiant unique.

    bool hasData();          // Renvoi true si le serveur à envoyé des paquets.
    SERVER_DATA_T getData(); // Retourne les données et les enlèves de la file d'attente. Attention: file LIFO.

    // Ajoute des infos pour la prochaine fois que le serveur demande des infos
    bool sendData(CLIENT_DATA_T);

    void clearClientData(); // Remet le compteur de données à envoyer à 0;
    void clearServerData(); // Remet le compteur de données reçues à 0;

    bool isSendingData(); // Renvoi true si clientSendingData == true

    // NB, les deux méthodes ci dessous doivent être proxy. Cf I2C.ino (exemple)
    void receiveEvent(); // Handler pour les paquets.
    void requestEvent(); // Handler pour les requêtes de données du serveur.

    // L'utilisateur peut définir un callback qui sera appelé à chaque fois que le serveur envoi un paquet.
    void setCallback(void (*)());

    // Sert à vérifier le contenu de la mémoire
    void dump(); 

private:
    SERVER_DATA_T serverDataQueue[SERVER_DATA_ARRAY_SIZE]; // Array de pointeurs vers les données du serveur en attente d'être lues
    CLIENT_DATA_T clientDataQueue[CLIENT_DATA_ARRAY_SIZE]; // Array de pointeurs vers les données du client en attente d'être envoyées

    int serverDataAvailable = 0; // Le nombre de paquets disponibles
    int clientDataAvailable = 0; // Le nombre de paquets à envoyer

    bool clientSendingData = false; // Défini par les paquets START_TX / STOP_TX.

    bool hasCallback = false;
    void (*userDataReceivedCallback)();

    uint8_t clientId = 0;

    void sendAvailablePacketsToServer();
};

#endif

/*
A propos du fonctionnement du serveur.
La librairie Wire impose un maximum de 32 bytes (Qu'il est possible d'augmenter.)
Ici, je stock les messages en mémoire (maximum 16 messages) en attendant l'envoie.

Pour envoyer le serveur execute pour chaque client une séquence spécifique:

    Serveur =(Demande d'information avec .requestFrom())=> Client
    Serveur <=(Renvoi le nombre n de données prêtes à être envoyées)= Client
    Serveur =(Envoi un paquet de type START_TX)=> Client

    Pour chaque n de 0 à 1:
        Serveur =(Demande d'information avec .requestFrom())=>Client
        Serveur <=(Paquet CLIENT_DATA de 32 bytes)= Client

    Serveur =(Envoi un paquet de type STOP_TX)=> Client

Cette séquence est executée à interval défini


Note: Cette librairie va utiliser au minimum 1024 bytes de SRAM (mémoire).
Pour réduire l'utilisation mémoire, il faut passer CLIENT_DATA_ARRAY_SIZE / SERVER_DATA_ARRAY_SIZE à 8 bytes. Cela relachera 512 bytes de mémoire (8*16*2)

Si l'utilisation est vraiment trop grande, je peux changer en allocation dynamique (Mais c'est moins facile...) ou même stocker les données en EEPROM (Mais c'est moins rapide)

*/
