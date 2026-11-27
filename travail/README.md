# Répertoire de travail

```text
travail/
├── include/
│   ├── common.h             # Constantes et fonctions partagées
│   ├── msg_struct.h         # Types et en-tête des messages
│   ├── command.h            # Commandes côté client
│   ├── file_transfer.h      # Transfert de fichiers entre clients
│   ├── utils.h              # Liste des clients et salons du serveur
│   └── server_command.h     # Commandes côté serveur
├── src/
│   ├── common.c             # Code partagé entre client et serveur
│   ├── client/
│   │   ├── client.c         # Connexion, boucle poll, réception
│   │   ├── command.c        # Lecture et envoi des commandes
│   │   └── file_transfer.c  # Transfert de fichiers entre clients
│   └── server/
│       ├── server.c         # Écoute, boucle poll, réception
│       ├── utils.c          # Gestion des clients et des salons
│       └── command.c        # Traitement des commandes
├── Makefile
└── README.md
```

Compilation avec `make`, les exécutables sont produits dans `bin/`. Nettoyage avec `make clean`.

## Transfert de fichiers

Les fichiers reçus sont enregistrés dans `.resa1/inbox/`.
