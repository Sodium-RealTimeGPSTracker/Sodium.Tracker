//
// Created by Hugo Boisselle on 2018-04-12.
//

#include "ServeurEnvoiWebsocket.h"
#include <iostream>

ServeurEnvoiWebSocket::ServeurEnvoiWebSocket(const string& adresseServeur):
        adresseServeur(adresseServeur) {
    connecter();
    th = thread([&] {
        while (!doitTerminer())
            agir();
        ws->close();
    });
}

void ServeurEnvoiWebSocket::arreter(){
    termine = true;
}

void ServeurEnvoiWebSocket::ajouter(const string& message){
    lock_guard l{mutexBufferSwap};
    buffers[bufferAjout].push_back(message);
}

void ServeurEnvoiWebSocket::ajouter(string&& message){
    lock_guard l{mutexBufferSwap};
    buffers[bufferAjout].emplace_back(message);
}

template <class It>
void ServeurEnvoiWebSocket::ajouter(const It debut, const It fin){
    lock_guard l{mutexBufferSwap};
    for(auto it = debut; it != fin; next(it))
        buffers[bufferAjout].push_back(it);
}

ServeurEnvoiWebSocket::~ServeurEnvoiWebSocket(){
    arreter();
    th.join();
    delete ws;
}

bool ServeurEnvoiWebSocket::doitTerminer() const{
    return termine;
}

bool ServeurEnvoiWebSocket::estDeconnecte(){
    return ws == nullptr ||
           ws->getReadyState() == WebSocket::CLOSED ||
           ws->getReadyState() == WebSocket::CLOSING;
}

void ServeurEnvoiWebSocket::reconnecter(){
    int tempsAttente{tempsAttenteReconnexionParDefaut};
    connecter();
    while(estDeconnecte()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tempsAttente));
        tempsAttente*=2;
        connecter();
    }
}

void ServeurEnvoiWebSocket::connecter(){
    delete ws; // Aucun effet avec nullptr
    ws = WebSocket::from_url(adresseServeur);
}

void ServeurEnvoiWebSocket::swapBuffers(){
    lock_guard l{mutexBufferSwap};
    std::swap(bufferAjout, bufferEnvoi);
}

void ServeurEnvoiWebSocket::envoyer(){
    for(auto it = begin(buffers[bufferEnvoi]); it != end(buffers[bufferEnvoi]); it = next(it)){
        if(estDeconnecte())
            reconnecter();
        ws->send(it->c_str());
        ws->poll(0);
    }
}

void ServeurEnvoiWebSocket::agir(){
    swapBuffers();
    envoyer();
    buffers[bufferEnvoi].clear();
}