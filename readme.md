## Projet  

Le protocole Mégaphone est un protocole de communication de type client/serveur. Des utilisateurs se connectent à un serveur avec un client et peuvent ensuite suivre des fils de discussions, en créer de nouveaux ou enrichir des fils existants en postant un nouveau message (billet), s’abonner à un fil pour recevoir des notifications.  

Plus précisément, un utilisateur peut s’inscrire auprès du serveur et une fois inscrit, il doit pouvoir faire les actions suivantes :  
  – poster un message ou un fichier sur un nouveau fil,  
  – poster un message ou un fichier sur un fil existant,  
  – demander la liste des n derniers billets sur chaque fil,  
  – demander les n derniers billets d’un fil donné,  
  – s’abonner à un fil et recevoir les notifications de ce fil.  
  
Le serveur doit répondre aux demandes du client et également envoyer des notifications.  


## Instructions  

Pour compiler:
`make clean`
`make build`

Pour lancer le client:
`./build/client [Server IPv6 address] [Server port]`

Pour lancer le serveur:
`./build/server [Server port]`

Les fichiers uploadé dans le serveur seront placés dans ./fichiers_fil
Les fichiers téléchargés depuis le client seront placés dans ./downloaded_files

Lancé sans argument, notre serveur Megaphone tourne sur le port 7777 (configurable depuis `server.h` et `client.h`)
