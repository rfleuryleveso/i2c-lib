#include "VB_I2C.hpp"
#include <Wire.h>
#include <Arduino.h>
#include <avr/wdt.h>

VbI2C::VbI2C(int address)
{
#ifdef DEBUG
    Serial.println("Starting VBI2C");
    Serial.print("Address is: ");
    Serial.println(address, 16);
#endif

    Wire.begin(address); // On démarre la transmission par Wire.
    this->clientId = address;

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
    return this->serverDataAvailable > 0;
}

SERVER_DATA_T VbI2C::getData()
{
    if (this->serverDataAvailable > 0)
    {
        int dataIndex = this->serverDataAvailable;
        // On renvoie la donnée à l'index de la dernière donnée reçue (Donc, nombre de données reçues - 1). Ensuite on diminue le nombre de données disponibles
        this->serverDataAvailable--;
        return this->serverDataQueue[dataIndex - 1];
    }
    else
    {
        return NULL;
    }
}

bool VbI2C::sendData(CLIENT_DATA_T data)
{
    noInterrupts();
    int index = this->clientDataAvailable;
    if (index > CLIENT_DATA_ARRAY_SIZE)
    {
        Serial.println("TOO MUCH IN QUEUE");
        return false;
    }

#ifdef DEBUG
    Serial.print("Sending data, index: ");
    Serial.print(index);
    Serial.print(" -> 0x");
    Serial.println((int)this->clientDataQueue[index], 16);
#endif

    // On ajoute une donnée à la file.
    // J'utilise memcpy pour garder toujours les mêmes emplacements mémoires...
    memcpy(this->clientDataQueue[index], data, 32);

    // On redéfinie le clientId.
    this->clientDataQueue[index]->clientId = this->clientId;

#ifdef DEBUG
    Serial.print("Packet ID: ");
    Serial.println(this->clientDataQueue[index]->dataType);
#endif
    interrupts();
    this->clientDataAvailable++;
    return true;
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
}

bool VbI2C::isSendingData()
{
    return this->clientSendingData;
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


void VbI2C::receiveEvent()
{
    // Si on reçoit des données
    if (Wire.available())
    {

        // On transfère les bytes reçues dans un emplacement mémoire. On caste en uint8_t (Au final, ce sont des uint8_t)
        byte buff[32];
        Wire.readBytes(buff, 32);

        this->serverDataAvailable++;

        memcpy(this->serverDataQueue[this->serverDataAvailable - 1], &buff, 32);

        // Serial.print("Writing to: ");
        // Serial.print("0x");
        // Serial.println((int)this->serverDataQueue[this->serverDataAvailable - 1], HEX);

        // Serial.print("IDX: ");
        // Serial.println(this->serverDataAvailable);

        // On récupère le pointeur pour vérifier les données
        SERVER_DATA_T receivedData = this->serverDataQueue[this->serverDataAvailable - 1];

#ifdef DEBUG
        Serial.println("RECEIVED SERVER PACKET OVER WIRE");
        Serial.print("Packet ID: ");
        Serial.println(receivedData->dataType, HEX);
#endif

        if (receivedData->dataType == SERVER_DATA_TYPE::START_TX)
        {

#ifdef DEBUG
            Serial.println("SERVER_DATA_TYPE::START_TX");
#endif
            this->clientSendingData = true;
            this->serverDataAvailable--;
        }
        else if (receivedData->dataType == SERVER_DATA_TYPE::STOP_TX)
        {

#ifdef DEBUG
            Serial.println("SERVER_DATA_TYPE::STOP_TX");
#endif
            this->clientSendingData = false;
            this->serverDataAvailable--;
        }
        else
        {
            if (this->hasCallback)
            {
                this->userDataReceivedCallback();
            }
        }
        
    }
}

void VbI2C::sendAvailablePacketsToServer()
{

    // On indique au serveur combien de packets sont disponibles
    CLIENT_DATA availablePacketsPacket;
    availablePacketsPacket.clientId = this->clientId;
    availablePacketsPacket.dataType = CLIENT_DATA_TYPE::START_ACK;
    memset(&availablePacketsPacket.data, 0, sizeof(availablePacketsPacket.data));
    memcpy(&availablePacketsPacket.data, &this->clientDataAvailable, sizeof(this->clientDataAvailable));
#ifdef DEBUG
    // Serial.println("Sending available packets to server for further processing");
    Serial.print("Available packets: ");
    Serial.println(this->clientDataAvailable);
#endif
    int sent = Wire.write((uint8_t *)&availablePacketsPacket, sizeof(CLIENT_DATA));

#ifdef DEBUG
    Serial.print("Sent: ");
    Serial.println(sent);
#endif
}

void VbI2C::requestEvent()
{
    // Si on est en phase d'envoie de données, on envoi un array de bytes.
    // Sinon, on renvoi le nombre de bytes disponibles

#ifdef DEBUG
    Serial.println("requestEvent()");
#endif

    if (!this->isSendingData())
    {
        this->sendAvailablePacketsToServer();
    }
    else
    {
#ifdef DEBUG
        Serial.print("Sending packet content #");
        Serial.println(this->clientDataAvailable - 1);
        for (size_t i = 0; i < 32; i++)
        {
            Serial.print(((uint8_t *)this->clientDataQueue[this->clientDataAvailable - 1])[i], HEX);
            Serial.print(' ');
        }
#endif
        Wire.write((uint8_t *)this->clientDataQueue[(this->clientDataAvailable - 1)], 32);
        this->clientDataAvailable--;
#ifdef DEBUG
        Serial.println("Sent packet");
#endif
    }
}

void VbI2C::setCallback(void (*user_func)())
{
    this->userDataReceivedCallback = user_func;
    this->hasCallback = true;
}