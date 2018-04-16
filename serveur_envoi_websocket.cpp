//
// Created by Hugo Boisselle on 2018-04-12.
//

#include "serveur_envoi_websocket.h"

ServeurEnvoiWebSocket::ServeurEnvoiWebSocket(const string& adresseServeur):
        adresseServeur(adresseServeur),
        th(&ServeurEnvoiWebSocket::agir, this),
        ws(WebSocket::from_url(adresseServeur))
{
}

void ServeurEnvoiWebSocket::arreter()
{
    termine = true;
}

void ServeurEnvoiWebSocket::ajouter(const string& message)
{
    lock_guard<mutex> _(m);
    buffer.ajouter(message);
}

void ServeurEnvoiWebSocket::ajouter(string&& message)
{
    lock_guard<mutex> _(m);
    buffer.ajouter(&message, &message+1);
}

template <class It>
void ServeurEnvoiWebSocket::ajouter(const It debut, const It fin)
{
    lock_guard<mutex> _(m);
    buffer.ajouter(debut, fin);
}

ServeurEnvoiWebSocket::~ServeurEnvoiWebSocket()
{
    arreter();
    th.join();
    delete ws;
}

bool ServeurEnvoiWebSocket::doitTerminer() const
{
    return termine;
}

bool ServeurEnvoiWebSocket::estDeconnecte()
{
    return ws == nullptr ||
           ws->getReadyState() == WebSocket::CLOSED ||
           ws->getReadyState() == WebSocket::CLOSING;
}

void ServeurEnvoiWebSocket::reconnecter()
{
    int tempsAttente{tempsAttenteReconnexionParDefaut};
    while(estDeconnecte()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tempsAttente));
        tempsAttente*=2;
        connecter();
    }
}

void ServeurEnvoiWebSocket::connecter()
{
    if(ws != nullptr)
        ws->close();
    ws = WebSocket::from_url(adresseServeur);
}


void ServeurEnvoiWebSocket::envoyer()
{
    auto donnees = buffer.extraire();
    for(auto it = begin(donnees); it != end(donnees); it = next(it)){
        if(estDeconnecte())
            reconnecter();
        ws->send(*it);
        ws->poll(0);
    }
}

void ServeurEnvoiWebSocket::agir()
{
    connecter();
    while (!doitTerminer()) {
        envoyer();
    }
    ws->close();
}