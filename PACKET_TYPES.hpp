#ifndef VB_I2C_PACKET_TYPES
#define VB_I2C_PACKET_TYPES

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

#endif