Pour compiler:
`make clean`
`make build`

Pour lancer le client:
`./build/client [Server IPv6 address] [Server port]`

Pour lancer le serveur:
`./build/server [Server port]`

Lancé sans argument, notre serveur Megaphone tourne sur le port 7777 (configurable depuis server.h et client.h)

Les fichiers uploadé dans le serveur seront placés dans ./fichiers_fil

Les fichiers téléchargés depuis le client seront placés dans ./downloaded_files
