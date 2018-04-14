//
// Created by Hugo Boisselle on 2018-04-12.
//

#ifndef SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H
#define SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H
#include <thread>

#include <list>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <atomic>

#include "easywsclient/easywsclient.hpp"

using easywsclient::WebSocket;
using namespace std;

class ServeurEnvoiWebSocket {
private:
    const string adresseServeur;
    static const auto tempsAttenteReconnexionParDefaut = 1000;

    atomic<bool> termine{false};
    atomic<int> bufferAjout{0};

    array<list<string>, 2> buffers{};

    thread th;

    WebSocket::pointer ws;

public:
    ServeurEnvoiWebSocket(const string& adresseServeur);
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
    void swapBuffers();
    int getBufferEnvoi() const noexcept;
    int getBufferAjout() const noexcept;
    void envoyer();
    void agir();
};


#endif //SODIUM_TRACKER_SERVEUR_ES_WEBSOCKET_H
