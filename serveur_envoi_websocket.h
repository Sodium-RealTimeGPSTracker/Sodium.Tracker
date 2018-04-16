//
// Created by Hugo Boisselle on 2018-04-12.
//

#ifndef SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H
#define SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H

#include <mutex>
#include <string>
#include <atomic>

#include "easywsclient/easywsclient.hpp"
#include "ntuple_buffer.h"

using easywsclient::WebSocket;
using namespace std;

class ServeurEnvoiWebSocket {
private:
    const string adresseServeur;
    enum {tempsAttenteReconnexionParDefaut = 50 };

    atomic<bool> termine{false};
    ntuple_buffer<string, 200, 1> buffer{}; // Moyenne crasse, mais fonctionne

    thread th;
    mutex m;
    WebSocket::pointer ws;

public:
    explicit ServeurEnvoiWebSocket(const string& adresseServeur);
    ~ServeurEnvoiWebSocket();

    void arreter();

    void ajouter(const string& message);
    void ajouter(string&& message);
    template <class It>
    void ajouter(const It debut, const It fin);

private:
    bool doitTerminer() const;
    bool estDeconnecte();
    void reconnecter();
    void connecter();
    void envoyer();
    void agir();
};


#endif //SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H
