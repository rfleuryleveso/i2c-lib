#include "VB_I2C.hpp"
#include <Wire.h>
#include <Arduino.h>

VbI2C::VbI2C()
{
#ifdef DEBUG
    Serial.println("Starting VBI2C server");
#endif
    Wire.begin(); // On démarre la transmission par Wire.

#ifdef DEBUG
    Serial.println("Setting memory pointers");
    Serial.println("Server data pointers");
#endif

    // Initialisation de la mémoire
    for (size_t i = 0; i < SERVER_DATA_ARRAY_SIZE; i++)
    {
        // On initialise l'array de pointeurs
        this->serverDataQueue[i] = (SERVER_DATA_T)malloc(sizeof(SERVER_DATA));

#ifdef DEBUG
        Serial.print("Server PTR #");
        Serial.print(i);
        Serial.print(" -> 0x");
        Serial.println((int)this->serverDataQueue[i], 16);
#endif
        // On définie la mémoire à 0 pour avoir un espace de travail propre. (Evite les données parasites lors des transmissions)
        memset(this->serverDataQueue[i], 0, sizeof(SERVER_DATA));
    }

#ifdef DEBUG
    Serial.println("Client data pointers");
#endif

    for (size_t i = 0; i < CLIENT_DATA_ARRAY_SIZE; i++)
    {
        // On initialise l'array de pointeurs
        this->clientDataQueue[i] = (CLIENT_DATA_T)malloc(sizeof(CLIENT_DATA));
        if (this->clientDataQueue[i] == NULL)
        {
            Serial.print("Client PTR #");
            Serial.print(i);
            Serial.print(" -> FAILED !");
        }
#ifdef DEBUG
        Serial.print("Client PTR #");
        Serial.print(i);
        Serial.print(" -> 0x");
        Serial.println((int)this->clientDataQueue[i], 16);
#endif
        // On définie la mémoire à 0 pour avoir un espace de travail propre. (Evite les données parasites lors des transmissions)
        memset(this->clientDataQueue[i], 0, sizeof(CLIENT_DATA));
    }
}

bool VbI2C::hasData()
{
    return this->clientDataAvailable > 0;
}

CLIENT_DATA_T VbI2C::getData()
{
    // On renvoie la donnée à l'index de la dernière donnée reçue (Donc, nombre de données reçues - 1). Ensuite on diminue le nombre de données disponibles
    return this->clientDataQueue[(this->clientDataAvailable--) - 1];
}

bool VbI2C::sendData(SERVER_DATA_T data)
{
    int index = this->serverDataAvailable;
    if (index > 15)
    {
        return false;
    }

#ifdef DEBUG
    Serial.print("Sending data, index: ");
    Serial.print(index);
    Serial.print(" -> 0x");
    Serial.println((int)this->serverDataQueue[index], 16);
#endif

    this->serverDataAvailable++;

    // On ajoute une donnée à la file.
    // J'utilise memcpy pour garder toujours les mêmes emplacements mémoires...
    memcpy(this->serverDataQueue[index], data, sizeof(SERVER_DATA));

#ifdef DEBUG
    Serial.print("Packet ID: ");
    Serial.println(this->serverDataQueue[index]->dataType);
#endif
    return true;
}

bool VbI2C::fastSendData(SERVER_DATA_T data)
{
    Wire.beginTransmission(data->clientId);
    Wire.write((uint8_t *)data, 32);
    Wire.endTransmission();
}

void VbI2C::clearClientData()
{
    for (size_t i = 0; i < CLIENT_DATA_ARRAY_SIZE; i++)
    {
        // On redéfinie la mémoire à 0 pour revenir à 0;
        memset(this->clientDataQueue[i], 0, sizeof(CLIENT_DATA));
    }
    this->clientDataAvailable = 0;
}

void VbI2C::clearServerData()
{
    for (size_t i = 0; i < SERVER_DATA_ARRAY_SIZE; i++)
    {
        // On redéfinie la mémoire à 0 pour revenir à 0;
        memset(this->serverDataQueue[i], 0, sizeof(SERVER_DATA));
    }
    this->serverDataAvailable = 0;
#ifdef DEBUG
    Serial.print("Cleared ServerData: ");
    Serial.println(this->serverDataAvailable);
#endif
}

void VbI2C::receiveEvent()
{
#ifdef DEBUG
    Serial.println("RECEIVED DATA");
#endif
    // Si on reçoit des données
    if (Wire.available())
    {

        // On transfère les bytes reçues dans un emplacement mémoire. On caste en uint8_t (Au final, ce sont des uint8_t)
        Wire.readBytes((uint8_t *)this->clientDataQueue[this->clientDataAvailable++], 32);
#ifdef DEBUG
        Serial.print("IDX: ");
        Serial.println(this->clientDataAvailable);
#endif
        // On récupère le pointeur pour vérifier les données
        CLIENT_DATA_T receivedData = this->clientDataQueue[this->clientDataAvailable - 1];

        if (receivedData->dataType == CLIENT_DATA_TYPE::START_ACK)
        {
#ifdef DEBUG
            Serial.println(">> START ACK RECEIVED <<");
#endif
            // On traite le paquet
            uint8_t packetsAvailable = 0;
            memcpy(&packetsAvailable, &receivedData->data[0], sizeof(uint8_t));
#ifdef DEBUG
            Serial.print(packetsAvailable);
            Serial.print(" packets available");
            Serial.print(" from ");
            Serial.println(receivedData->clientId, DEC);
#endif
            SERVER_DATA_T startPacket = new SERVER_DATA();
            startPacket->dataType = SERVER_DATA_TYPE::START_TX;
            memset(startPacket->data, 0, 31);
            startPacket->clientId = receivedData->clientId;
            this->fastSendData(startPacket);
            free(startPacket);

            // Et pour chaque paquet, on le demande à l'émetteur
            for (uint8_t packet = 0; packet < packetsAvailable; packet++)
            {
                Wire.requestFrom((int)receivedData->clientId, 32);
                this->receiveEvent();
            }

            // END OF TX PACKET
            SERVER_DATA_T endPacket = new SERVER_DATA();
            endPacket->dataType = SERVER_DATA_TYPE::STOP_TX;
            memset(endPacket->data, 0, 31);
            endPacket->clientId = receivedData->clientId;
            this->fastSendData(startPacket);
            free(endPacket);

            this->clientDataAvailable--;
        }
        else
        {
#ifdef DEBUG
            Serial.println(">> OTHER PACKET RECEIVED <<");
            for (size_t i = 0; i < 32; i++)
            {
                Serial.print(((uint8_t *)receivedData)[i], HEX);
                Serial.print(' ');
            }
            Serial.println();
#endif

            if (this->hasCallback)
            {
                this->userDataReceivedCallback();
            }
        }
    }
}

void VbI2C::setCallback(void (*user_func)())
{
    this->userDataReceivedCallback = user_func;
    this->hasCallback = true;
}

void VbI2C::dump()
{
    Serial.println("DUMPING VBi2C data.");
    Serial.print("serverDataAvailable: ");
    Serial.println(this->serverDataAvailable);

    for (int packet = 0; packet < SERVER_DATA_ARRAY_SIZE; packet++)
    {
        Serial.print("Packet #");
        Serial.print(packet);
        Serial.print(": ");
        for (size_t i = 0; i < 33; i++)
        {
            Serial.print(((uint8_t *)this->serverDataQueue[packet])[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }

    Serial.print("clientDataAvailable: ");
    Serial.println(this->clientDataAvailable);

    for (int packet = 0; packet < CLIENT_DATA_ARRAY_SIZE; packet++)
    {
        Serial.print("Packet #");
        Serial.print(packet);
        Serial.print(": ");
        for (size_t i = 0; i < 32; i++)
        {
            Serial.print(((uint8_t *)this->clientDataQueue[packet])[i], HEX);
            Serial.print(' ');
        }
        Serial.println();
    }
}

void VbI2C::tick()
{
// Pour chaque client
#ifdef DEBUG
    Serial.print(this->clientCount);
    Serial.print(" clients, ");
    Serial.print(this->serverDataAvailable);
    Serial.println(" packets available");
#endif

    for (uint8_t clientIndex = 0; clientIndex < this->clientCount; clientIndex++)
    {

        uint8_t clientId = this->clients[clientIndex]; // On récupère l'ID I2C à partir du tableau des clients

        // Pour chaque paquet
        for (uint8_t packetId = 0; packetId < this->serverDataAvailable; packetId++)
        {
            // Si le paquet est destiné au client
            uint8_t target = this->serverDataQueue[packetId]->clientId;
            if (target == clientId || target == -1)
            {
#ifdef DEBUG
                Serial.print("  Packet #");
                Serial.print(packetId);
                Serial.print(", packet type: ");
                Serial.print(this->serverDataQueue[packetId]->dataType);
#endif

                Wire.beginTransmission(clientId);
                // On envoie les données à la cible, moins l'ID de la cible.
                Wire.write((uint8_t *)this->serverDataQueue[packetId], 32);
                Wire.endTransmission();
#ifdef DEBUG
                Serial.println(" SENT !");
#endif
            }
        }
    }
    this->clearServerData();

    // Ensuite, on requiert les données.
    for (uint8_t clientIndex = 0; clientIndex < this->clientCount; clientIndex++)
    {

        uint8_t clientId = this->clients[clientIndex];
#ifdef DEBUG
        Serial.print("Client, ");
        Serial.print(clientId);
        Serial.println(" sending packet request... ");
#endif

        Wire.requestFrom(clientId, (uint8_t)32);
        this->receiveEvent();
    }
}

void VbI2C::registerClient(int clientId)
{
    this->clients[this->clientCount++] = clientId;
}