# Répertoire de travail

```text
travail/
├── include/
│   ├── common.h             # Constantes et fonctions partagées
│   ├── msg_struct.h         # Types et en-tête des messages
│   ├── command.h            # Commandes côté client
│   ├── file_transfer.h      # Transfert de fichiers entre clients
│   ├── client_list.h        # Liste des clients du serveur
│   └── server_command.h     # Commandes côté serveur
├── src/
│   ├── common.c             # Code partagé entre client et serveur
│   ├── client/
│   │   ├── client.c         # Connexion, boucle poll, réception
│   │   ├── command.c        # Lecture et envoi des commandes
│   │   └── file_transfer.c  # Transfert de fichiers entre clients
│   └── server/
│       ├── server.c         # Écoute, boucle poll, réception
│       ├── client_list.c    # Gestion de la liste des clients
│       └── command.c        # Traitement des commandes
├── Makefile
└── README.md
```

Compilation avec `make`, les exécutables sont produits dans `bin/`. Nettoyage avec `make clean`.

## Transfert de fichiers

Sur la connexion directe, l'émetteur envoie le fichier en plusieurs trames `FILE_SEND` d'au plus `MSG_LEN` octets, puis une trame `FILE_SEND` vide qui marque la fin. Le récepteur répond `FILE_ACK`. Les fichiers reçus sont enregistrés dans `.resa1/inbox/`.

Les décisions de protocole sont décrites dans [../protocole.md](../protocole.md).
