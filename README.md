# Sodium Tracker
Le projet ci-présent vise le tracker GPS RaspberryPi.

##Pour le compiler le projet
Nécessite cmake pour la génération des fichiers make

`mkdir build`

`cd build`

`cmake ..`

`./Sodium.Tracker <adresse_serveur> < <fichier>`

##Pour le tester
Il est possible de tester le projet sans avoir à le rouler sur le RaspberryPi.

`./Sodium.Tracker <adresse_serveur> -t < ../gps_log`

Le fichier gps_log contient une capture de 2 minutes de données réelles provenant directement du module GPS.
