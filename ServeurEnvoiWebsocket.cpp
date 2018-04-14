//
// Created by Hugo Boisselle on 2018-04-12.
//

#include "ServeurEnvoiWebsocket.h"
#include <iostream>
ServeurEnvoiWebSocket::ServeurEnvoiWebSocket(const string& adresseServeur):
        adresseServeur(adresseServeur),
        th(&ServeurEnvoiWebSocket::agir, this),
        ws(WebSocket::from_url(adresseServeur))
{
}

void ServeurEnvoiWebSocket::arreter(){
    termine = true;
}

void ServeurEnvoiWebSocket::ajouter(const string& message){
    buffers[bufferAjout].push_back(message);
}

void ServeurEnvoiWebSocket::ajouter(string&& message){
    buffers[bufferAjout].emplace_back(message);
}

template <class It>
void ServeurEnvoiWebSocket::ajouter(const It debut, const It fin){
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
    while(estDeconnecte()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tempsAttente));
        tempsAttente*=2;
        connecter();
    }
}

void ServeurEnvoiWebSocket::connecter(){
    ws = WebSocket::from_url(adresseServeur);
}

int ServeurEnvoiWebSocket::getBufferEnvoi() const noexcept{
    return bufferAjout == 0 ? 1 : 0;
}

int ServeurEnvoiWebSocket::getBufferAjout() const noexcept{
    return bufferAjout;
}

void ServeurEnvoiWebSocket::swapBuffers(){
    bufferAjout = bufferAjout == 1 ? 0 : 1;

}

void ServeurEnvoiWebSocket::envoyer(){
    auto bufferEnvoi = getBufferEnvoi();
    for(auto it = begin(buffers[bufferEnvoi]); it != end(buffers[bufferEnvoi]); it = next(it)){
        if(estDeconnecte())
            reconnecter();
        ws->send(it->c_str());
        ws->poll(0);
    }
}

void ServeurEnvoiWebSocket::agir(){
    connecter();
    while (!doitTerminer()) {
        swapBuffers();
        envoyer();
        buffers[getBufferEnvoi()].clear();
    }
    ws->close();
}