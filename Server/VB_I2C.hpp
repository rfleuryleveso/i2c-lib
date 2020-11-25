#ifndef VB_I2C_HPP
#define VB_I2C_HPP

#define CLIENT_DATA_ARRAY_SIZE 9
#define SERVER_DATA_ARRAY_SIZE 8

#include <stdint.h>
#include "../PACKET_TYPES.hpp"


typedef struct // Données envoyées par le serveur au client
{
    enum SERVER_DATA_TYPE dataType; // Type de paquet
    uint8_t data[31];               // Données, 31 bytes. On peut considérer ce tableau comme un tableau de byte (ce qu'il est, en réalité.)
    uint8_t clientId;               // ID de la cible. 255 = Broadcast. Attention, le broadcast va cycler à tous les IDs de 0 à 127
} SERVER_DATA, *SERVER_DATA_T;

typedef struct // Données envoyées par le client
{
    enum CLIENT_DATA_TYPE dataType; // Type de paquet
    uint8_t clientId;               // Emetteur du message
    uint8_t data[30];               // Données, 30 bytes
} CLIENT_DATA, *CLIENT_DATA_T;

class VbI2C
{
public:
    VbI2C();

    bool hasData();          // Renvoi true si le serveur à envoyé des paquets.
    CLIENT_DATA_T getData(); // Retourne les données et les enlèves de la file d'attente. Attention: file LIFO.

    // Ajoute des infos pour le prochain envoi
    bool sendData(SERVER_DATA_T);
    
    // Envoi instantanément des données
    bool fastSendData(SERVER_DATA_T);

    void clearClientData(); // Remet le compteur de données reçues à 0;
    void clearServerData(); // Remet le compteur de données à envoyer à 0;

    // NB, la méthode ci dessous doit être proxy. Cf I2C.ino (exemple)
    void receiveEvent(); // Handler pour les paquets.

    // L'utilisateur peut définir un callback qui sera appelé à chaque fois que le serveur envoi un paquet.
    void setCallback(void (*)());

    // Sert à envoyer les paquets. 
    void tick(); 

    // Définie le nombre de clients connectés.
    void registerClient(int);

     // Sert à vérifier le contenu de la mémoire
    void dump(); 


private:
    SERVER_DATA_T serverDataQueue[SERVER_DATA_ARRAY_SIZE]; // Array de pointeurs vers les données du serveur en attente d'être lues
    CLIENT_DATA_T clientDataQueue[CLIENT_DATA_ARRAY_SIZE]; // Array de pointeurs vers les données du client en attente d'être envoyées

    int clients[8];
int clientCount = 0;

    int serverDataAvailable = 0; // Le nombre de paquets disponibles
    int clientDataAvailable = 0; // Le nombre de paquets à envoyer

    bool hasCallback = false;
    void (*userDataReceivedCallback)();

    
};

#endif