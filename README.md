# Sodium Tracker
Le projet ci-présent vise le tracker GPS RaspberryPi.

##Pour le compiler le projet
Nécessite cmake pour la génération des fichiers make

`mkdir build`

`cd build`

`cmake ..`

##Pour l'exécuter
`./Sodium.Tracker {adresse_serveur} < {fichier}`


##Pour le tester
Il est possible de simuler le comportement du RaspberryPi et rouler le tout sur une même machine.
- Il faut d'abord démarrer le serveur, **Sodium.WebApp**.
- Ensuite, on peut démarrer le programme du tracker avec le fanion -t
`./Sodium.Tracker ws://localhost:8080 -t < ../gps_log`

Le fichier gps_log contient une capture de 2 minutes de données réelles provenant directement du module GPS.
