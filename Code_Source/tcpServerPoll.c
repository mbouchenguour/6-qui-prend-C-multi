#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#define PORT 55555
#define TAILLE_BUF 128

int joueurs = 0; //Stocke le nombre de joueur
int robots; //Stocke le nombre de robots
int manches; //Stocke le nombre de manche
int tete; //Stocke le nombre de tete de boeufs pour mettre fin à la partie
int server_socket; //Socket du serveur



//Affiche le message d'erreur et ferme l'application
void panic (const char *msg) {
  perror(msg);
  exit(1);
}


//Fin de la partie lors de ctrl+c
void fin (int sig) {
    char *cmd = (char*) malloc (TAILLE_BUF);
    if (close (server_socket) != -1)
        printf("Descripteur bien fermé\n");
    sprintf(cmd,"./suppression.sh %d %d", joueurs, robots); //On supprime les fichiers générés par le serveur

    system(cmd);
    wait(0);
    free(cmd);
    exit(0);
}

//Envoie un message à tous les clients
void envoyerMessage(char * message, struct pollfd pollfds[]){
    int taille;
    for (int i = 1; i <= joueurs; i++){
        taille = write(pollfds[i].fd,message, TAILLE_BUF);
            if(taille < 0)
                panic("Erreur lors de l'envoie du message");
    }
}

//Créer un socket qui accueil un nombre de joueurs passé en paramètre de la fonction
int creat_socket(int nb)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    int bindResult = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (bindResult == -1)
        perror("bindResult");

    int listenResult = listen(server_socket, nb);
    if (listenResult == -1)
        perror("listenResult");

    printf("Serveur actif sur port %d\n", ntohs(addr.sin_port));
    return server_socket;
}

//Vide le buffer de saisie du clavier
void clean_stdin(void)
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}


int main()
{
    struct sigaction act;
    char saisie[128] = "";  //Stocke le message à envoyer aux clients
    char log[250] = ""; //Stocke une chaîne à mettre dans le log
    bool attenteJoueur = true; //Booléen qui permet l'attente de connexion de tous les joueurs
    bool attenteReponses; //Booléen qui permet d'attendre la réponse de tous les joueurs
    int nbReponse; //Permet d'avoir le nombre de client qui ont répondu
    char *buf = (char*) malloc (TAILLE_BUF); //Buffer qui récupère les messages des clients et les sorties des scripts bash/awk
    FILE* fp; 
    char *cmd = (char*) malloc (TAILLE_BUF);    //Stocke la commande à exécuter
    int joueurTour; //Numéro du joueur qui doit jouer
    int nbTour = 1; //Compteur de tours
    bool recupererTete; //Booléen mis a vrai si un jour doit récupérer des têtes de boeufs
    int nbManche = 1;   //Compteur de manches
    bool finPartie = false; //Booleen qui met fin à une partie
    char delim[] = " "; //Delimitateur pour couper une chaine de caractères
    int carte;  //Stocke une carte
    int emplacement;    //Stocke un emplacement
    int nombreJoue;     //Stocke le nombre de joueur ayant joué
    int nbCarte;        //Stocke le nombre de carte sur une rangée
    pid_t pidRobot;     //Stocke le pid d'un processus fils
    int tube[2];        //Tube pour la communication entre le gestionnaire de jeu et les processus robots
    int taille;         //Taille lors d'un read/write
    int nbTete;         //Nombre de têtes de boeufs que foit récupèrer un joueur
    int minTemp;        //Nombre de robots min (2 robots si aucun joueur, 1 robot si un seul joueur)
    int nbJoueur;       //Numéro d'un joueur
    int i;              //Var de boucle

    //Demande du nombre de joueurs
    printf("Nombre de joueurs (min 0, max 10): ");
    while(scanf("%d", &joueurs) != 1 || !(joueurs >= 0 && joueurs <= 10)){
        clean_stdin();
        printf("Saisie incorrect\n");
        printf("Nombre de joueurs (min 0, max 10): ");
    }

    
    if (joueurs == 1)               //Il faut minimum un robot s'il n'y a qu'un joueur
        minTemp = 1;
    else if( joueurs == 0)          //Il faut minimum 2 robot s'il n'y a pas de joueur
        minTemp = 2;
    else
        minTemp = 0;        

    //Demande du nombre de robots
    printf("Nombre de robots (min %d, max %d): ", minTemp, 10 - joueurs);
    while(scanf("%d", &robots) != 1 || !( robots >= minTemp && robots <= (10 - joueurs) )){
        clean_stdin();
        printf("Saisie incorrect\n");
        printf("Nombre de robots (min %d, max %d): ", minTemp, 10 - joueurs);
    }

    //Demande du nombre de manche
    printf("Nombre de manche (0 si jusqu'à défaite) : ");

    while(scanf("%d", &manches) != 1 || (manches < 0)){
        clean_stdin();
        printf("Saisie incorrect\n");
        printf("Nombre de manche (0 si jusqu'à défaite) : ");
    }

    //Demande du nombre de têtes de boeufs
    printf("Nombre de tête de boeuf (minimum 1): ");

    while(scanf("%d", &tete) != 1 || (tete < 1)){
        clean_stdin();
        printf("Saisie incorrect\n");
        printf("Nombre de tête de boeuf (minimum 1): ");
    }

    //Initialise le score des joueurs
    int vie[joueurs + robots + 1];

    //Initialise le pseudo des joueurs et le score avec une chaine vide et 0
    char pseudo[joueurs + robots + 1][30];
    for(i = 0; i < joueurs + robots + 1; i++){
        vie[i] = 0;
        if(i <= joueurs){
            strcpy(pseudo[i], "");
        }
        else
            sprintf(pseudo[i], "Robot%d", i - joueurs);
    }

    server_socket = creat_socket(joueurs);

    struct pollfd pollfds[joueurs + 1];
    //Initialise le poll
    for(i = 0; i <= joueurs; i++){
        pollfds[i].fd = 0;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }

    pollfds[0].fd = server_socket;
    pollfds[0].events = POLLIN | POLLPRI;
    int useClient = 0;



    //Quitter proprement par ^C
    act.sa_handler = fin;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act,0);

    

    printf("Lancement de la partie\n");

    //Si on a que des robots, on attends pas
    if(joueurs == 0)
        attenteJoueur = false;
        
    //On attend la connexion des joueurs
    while(attenteJoueur){
        int pollResult = poll(pollfds, useClient + 1, 5000);
        if (pollResult > 0)
        {
            if (pollfds[0].revents & POLLIN)
            {
                struct sockaddr_in cliaddr;
                unsigned int addrlen = sizeof(cliaddr);
                int client_socket = accept(server_socket, (struct sockaddr *)&cliaddr, &addrlen);
                printf("Succes accept %s\n", inet_ntoa(cliaddr.sin_addr));
                for (i = 1; i <= joueurs; i++)
                {
                    if (pollfds[i].fd == 0)
                    {
                        pollfds[i].fd = client_socket;
                        pollfds[i].events = POLLIN | POLLPRI;
                        useClient++;
                        write(pollfds[i].fd,&i,sizeof(i));	 //On envoie aux clients le numéro du joueur
                        break;
                    }
                }
            }
        }
        if(useClient == joueurs){
            attenteJoueur = false;
        }
    }


    attenteReponses = true;
    nbReponse=0;
    if(joueurs == 0)
        attenteReponses = false;

    //On attend le pseudo des joueurs
    while(attenteReponses){
        int pollResult = poll(pollfds, useClient + 1, 5000);
        if (pollResult > 0)
        {
            for (i = 1; i <= joueurs; i++)
            {
                    if (pollfds[i].fd > 0 && pollfds[i].revents & POLLIN)
                    {
                        int bufSize = read(pollfds[i].fd, buf, sizeof(buf));
                        if (bufSize <= 0)
                        { 
                            printf("Au revoir\n");
                            pollfds[i].fd = 0;
                            pollfds[i].events = 0;
                            pollfds[i].revents = 0;
                            useClient--;
                        }
                        else
                        {
                            buf[bufSize] = '\0';
                            nbReponse++;
                            if(nbReponse == joueurs)
                                attenteReponses = false;
                            strcpy(pseudo[i], buf);
                        }
                    }
            }
        }
    }


    sleep(1);

    strcpy(saisie,"Bienvenue sur 6 qui prend, commencement de la partie");
    envoyerMessage(saisie, pollfds);
        
    sleep(1);


    while(((nbManche <= manches ) || manches == 0) && !finPartie){ //On joue tant que la partie n'est pas terminé et tant que le nombre de manche n'est pas joué
        //debut de la manche
        sprintf(saisie,"Debut de la manche %d", nbManche);
        envoyerMessage(saisie, pollfds);

        sprintf(log, "echo \"%s\n\" >> log", saisie);

        sprintf(saisie, "./scriptCartes.sh %d %d", joueurs, robots); //Génération des cartes, du fichier log, du fichier qui stocke l'orde des joueurs
        system(saisie);
        wait(0);

        system(log);
        wait(0);

        

        sprintf(saisie,"Cartes");
        envoyerMessage(saisie, pollfds);

        //On envoie les cartes aux joueurs
        for (i = 1; i <= joueurs; i++){
            sprintf(cmd,"cat carteJoueur%d", i);

            if ((fp = popen(cmd, "r")) == NULL)
                panic("Erreur lors de l'ouvertue du tube\n");

            while(fgets(buf, TAILLE_BUF, fp) != NULL){
                carte = atoi(buf);
                write(pollfds[i].fd, &carte,sizeof(carte));
            }
                    
            if (pclose(fp))
                panic("Erreur lors de la fermeture du tube\n");
        }

        nbTour = 1;
        while(nbTour <= 10 && !finPartie ){
            //Début du tour
            sleep(1);
            sprintf(saisie,"Debut du tour %d", nbTour);
            envoyerMessage(saisie, pollfds);

            sprintf(log, "echo \"%s\n\" >> log", saisie);
            system(log);
            wait(0);

            sleep(1);

            //On envoie le plateau
            strcpy(saisie, "Plateau du jeu :");
            envoyerMessage(saisie, pollfds);
            
            sprintf(cmd,"cat plateau");

            if ((fp = popen(cmd, "r")) == NULL)
                panic("Erreur lors de l'ouvertue du tube\n");

            while(fgets(buf, TAILLE_BUF, fp) != NULL){
                envoyerMessage(buf, pollfds);
            }

            if (pclose(fp))
                panic("Erreur lors de la fermeture du tube\n");

            //sauvegarde dans le log
            system("echo Plateau du jeu : >> log");
            wait(0);

            system("cat plateau >> log");
            wait(0);

            system("echo >> log");
            wait(0);
            
            sleep(1);

            //On demande aux joueurs de choisir une carte
            strcpy(saisie, "Choisir carte");
            envoyerMessage(saisie, pollfds);

            attenteReponses = true;
            nbReponse=0;

            if(joueurs == 0)
                attenteReponses = false;

            //On attend la réponse des joueurs
            while(attenteReponses){
                int pollResult = poll(pollfds, useClient + 1, 5000);
                if (pollResult > 0)
                {
                    for (i = 1; i <= joueurs; i++)
                    {
                            if (pollfds[i].fd > 0 && pollfds[i].revents & POLLIN)
                            {
                                int bufSize = read(pollfds[i].fd, &carte, sizeof(carte));
                                if (bufSize <= 0)
                                { 
                                    printf("Au revoir\n");
                                    pollfds[i].fd = 0;
                                    pollfds[i].events = 0;
                                    pollfds[i].revents = 0;
                                    useClient--;
                                }
                                else
                                {
                                    buf[bufSize] = '\0';
                                    nbReponse++;
                                    if(nbReponse == joueurs)
                                        attenteReponses = false;
                                    sprintf(saisie, "echo %d %d >> ordreJoueur", carte, i); // On stocke la saisie des joueurs
                                    system(saisie);
                                    wait(0);
                                }
                            }
                    }
                }
            }
            
            

            for(i = 1; i <= robots; i++){
                //On fait jouer les joueurs robotos
                
                int p = pipe(tube);
                pidRobot = fork();
                //Processus robot qui va choisir une carte à partir du script robotIntelligent.sh
                
                if(p < 0)
                    panic("Erreur lors de la création du tube robot");
                if(pidRobot < 0)
                    panic("Erreur lors de la création du processus robot");
                
                if(pidRobot == 0){
                    close(tube[0]);
                    sprintf(cmd, "./robotIntelligent.sh %d", i+joueurs); //commande qui récupère la première carte et la retire des cartes du robot

                    if ((fp = popen(cmd, "r")) == NULL)
                        panic("Erreur lors de l'ouvertue du tube\n");
                    
                    if(fgets(buf, TAILLE_BUF, fp) != NULL) {
                        carte = atoi(buf);
                        write(tube[1], &carte, sizeof(carte));
                    }

                    if (pclose(fp))
                        panic("Erreur lors de la fermeture du tube\n");
                    
                    close(tube[1]);
                    exit(0);
                }
                else
                {
                    close(tube[1]);
                    taille = read(tube[0], &carte, sizeof(carte));
                    if(taille == 0)
                        panic("Erreur lors du passe de valeur");

                    sprintf(saisie, "echo %d %d >> ordreJoueur", carte, joueurs + i); // On stocke la saisie des robots
                    system(saisie);
                    close(tube[0]);
                    wait(0);
                }
            }

            system("sort -n -o ordreJoueur ordreJoueur"); //Trie le fichier pour avoir l'ordre
            wait(0);
            
            carte = 0;
            emplacement = 0;
            nombreJoue = 0;
            nbCarte = 0;
            nbJoueur = 0;

            strcpy(cmd, "cat ordreJoueur"); //Commande qui récupere l'ordre des joueurs
            if ((fp = popen(cmd, "r")) == NULL) 
                panic("Erreur lors de l'ouvertue du tube\n");
            
            while(fgets(buf, TAILLE_BUF, fp) != NULL){     //On récupère la ligne
                carte = atoi(strtok(buf, delim));       //On récupère le premier élément (valeur de la carte)
                nbJoueur = atoi(strtok(NULL, delim));     //On récupère le deuxième element (nbJoueur)
                sprintf(saisie, "%s a joué la carte %d", pseudo[nbJoueur], carte);
                envoyerMessage(saisie, pollfds);    //On envoie les cartes jouées à tous les joueurs
                sprintf(log, "echo %s >> log", saisie);
                system(log);
            }

            if (pclose(fp)) 
                panic("Erreur lors de la fermeture du tube\n");

            system("echo >> log");
            wait(0);

            sleep(1);

            system("sort -n -o ordreJoueur ordreJoueur"); //Trie le fichier pour avoir l'ordre
            wait(0);

            
            while((nombreJoue < joueurs + robots) && !finPartie)
            {
                nbCarte = 0;
                carte = 0;
                emplacement = 0;
                nbTete = 0;

                sprintf(cmd, "head -n 1 ordreJoueur | awk '{print $2}' "); //Commande qui récupere le numéro du joueur de la première ligne

                if ((fp = popen(cmd, "r")) == NULL) 
                    panic("Erreur lors de l'ouvertue du tube\n");
                
                if(fgets(buf, TAILLE_BUF, fp) != NULL)
                    joueurTour = atoi(buf);

                if (pclose(fp))
                    panic("Erreur lors de la fermeture du tube\n");

                sprintf(saisie,"Placement de la carte de %s", pseudo[joueurTour]);

                envoyerMessage(saisie, pollfds);

                sleep(1);


                sprintf(cmd, "head -n 1 ordreJoueur | awk '{print $1}' "); //Commande qui récupere le numéro de la carte de la première ligne

                if ((fp = popen(cmd, "r")) == NULL)
                    panic("Erreur lors de l'ouvertue du tube\n");
                
                if(fgets(buf, TAILLE_BUF, fp) != NULL)
                    carte = atoi(buf);

                if (pclose(fp))
                    panic("Erreur lors de la fermeture du tube\n");

                sprintf(cmd, "./cartePlacement.sh %d", carte);  //On place la carte

                if ((fp = popen(cmd, "r")) == NULL) 
                    panic("Erreur lors de l'ouvertue du tube\n");
                
                while(fgets(buf, TAILLE_BUF, fp) != NULL) {
                    if(emplacement == 0)
                        emplacement = atoi(buf);  //Emplacement de la carte qui est posé
                    else if(nbCarte == 0)
                        nbCarte = atoi(buf);   //Nombre de carte sur la rangée avant de poser la carte
                    else
                        nbTete = atoi(buf);    //Nombre de têtes de boeufs
                }

                if (pclose(fp))
                    panic("Erreur lors de la fermeture du tube\n");

                recupererTete = false;
                if (emplacement == 0) //Si on ne peux pas poser la carte
                {
                    recupererTete = true;
                    if(joueurTour <= joueurs){
                        //On demande au joueur de choisir un emplacement
                        strcpy(saisie,"Placement impossible");
                        write(pollfds[joueurTour].fd,saisie,sizeof(saisie));
                        attenteReponses = true;
                        if(joueurs == 0)
                            attenteReponses = false;
                        while(attenteReponses){
                            int pollResult = poll(pollfds, useClient + 1, 5000);
                            if (pollResult > 0)
                            { 
                                if (pollfds[joueurTour].fd > 0 && pollfds[joueurTour].revents & POLLIN)
                                {
                                    int bufSize = read(pollfds[joueurTour].fd, buf, TAILLE_BUF);
                                    if (bufSize <= 0)
                                    { 
                                        printf("Au revoir\n");
                                        pollfds[joueurTour].fd = 0;
                                        pollfds[joueurTour].events = 0;
                                        pollfds[joueurTour].revents = 0;
                                        useClient--;
                                    }
                                    else
                                    {
                                        buf[bufSize] = '\0';
                                        attenteReponses = false;
                                        emplacement = atoi(buf);
                                    }
                                }
                            }
                        }
                        
                    }
                    else
                    {
                        //On demande au robot de choisir un emplacement 
                        int p2 = pipe(tube);
                        pidRobot = fork();
                        if(p2 < 0)
                            panic("Erreur lors de la création du tube robot");
                        if(pidRobot < 0)
                            panic("Erreur lors de la création du processus robot");
                        
                        if(pidRobot == 0){
                            close(tube[0]);
                            strcpy(cmd, "awk -f robotEmplacement.awk plateau");      //On récupère l'emplacement qui a le moins de tête de boeufs
                            if ((fp = popen(cmd, "r")) == NULL)
                                panic("Erreur lors de l'ouvertue du tube\n");
                            
                            if(fgets(buf, TAILLE_BUF, fp) != NULL)
                                emplacement = atoi(buf);

                            if (pclose(fp))
                                panic("Erreur lors de la fermeture du tube\n");
                            write(tube[1], &emplacement, sizeof(emplacement));
                            close(tube[1]);
                            exit(0);
                        }
                        else
                        {
                            close(tube[1]);
                            taille = read(tube[0], &emplacement, sizeof(emplacement));
                            if(taille == 0)
                                panic("Erreur lors du passage de valeur");
                            close(tube[0]);
                            wait(0);
                        }   
                    }

                    
                    //On récupère le nombre de tête de boeuf a récupérer
                    sprintf(cmd, "awk -f nbTete.awk -v emplacement=%d plateau", emplacement);
                    if ((fp = popen(cmd, "r")) == NULL)
                        panic("Erreur lors de l'ouvertue du tube\n");
                    
                    if(fgets(buf, TAILLE_BUF, fp) != NULL)
                        nbTete = atoi(buf);

                    if (pclose(fp))
                        panic("Erreur lors de la fermeture du tube\n");

                    //On place la carte jouée sur le plateau
                    sprintf(saisie, "awk -f placementCarte.awk -v carte=%d -v emplacement=%d plateau > plateau2 && mv plateau2 plateau", carte, emplacement); //On place la carte
                    system(saisie);
                    wait(0);
                    
                }
                

                system("sed -i '1d' ordreJoueur"); //On supprime la premiere ligne
                wait(0);
                
                
                
                //On verifie si le joueur récupère des têtes de boeufs
                if(recupererTete){ 
                    sprintf(saisie,"%s ne pas poser de carte \nIl pose sa carte à l'emplacement %d et récupère %d têtes de boeufs", pseudo[joueurTour], emplacement, nbTete);
                    vie[joueurTour] = vie[joueurTour] + nbTete; //On ajoute le nombre de têtes de boeufs aux scores du joueur
                }
                else
                    sprintf(saisie,"Placement de la carte de %s de valeur %d à l'emplacement %d", pseudo[joueurTour], carte, emplacement);

                envoyerMessage(saisie, pollfds);
                sprintf(log, "echo \"%s\">> log", saisie);
                system(log);
                wait(0);
                // On verifie si le joueur à posé la sixieme carte
                if(nbCarte == 5 && !recupererTete){ 
                    sprintf(saisie,"Sixième carte posé, %s prend %d têtes de boeufs", pseudo[joueurTour], nbTete);
                    vie[joueurTour] = vie[joueurTour] + nbTete; //On ajoute le nombre de têtes de boeufs aux scores du joueur
                    envoyerMessage(saisie, pollfds);

                    sprintf(log, "echo \"%s\">> log", saisie);
                    system(log);
                    wait(0);
                }


                //On verifie si un joueur a eu le nombre de têtes de boeufs max
                if (vie[joueurTour] >= tete)
                        finPartie = true;

                sleep(2);

                //On affiche la vie aux joueurs
                if((nbCarte == 5 && !recupererTete) || recupererTete){
                    sprintf(saisie,"Vie :");
                    envoyerMessage(saisie, pollfds);
                    sleep(1);

                    system("echo >> log");
                    wait(0);

                    int nombreDeJoueurs = joueurs + robots;
                    for(i = 1; i<= nombreDeJoueurs; i++){
                        sprintf(saisie,"%s a %d têtes de boeufs", pseudo[i], vie[i]);
                        envoyerMessage(saisie, pollfds);
                        sprintf(log, "echo \"%s\">> log", saisie);
                        system(log);
                        wait(0);
                    }

                    system("echo >> log");
                    wait(0);
                }
                


                sleep(2);
                //Si ce n'est pas la fin de la partie, on affiche le plateau
                if(!finPartie){
                    
                    strcpy(saisie,"Plateau du jeu :");
                    envoyerMessage(saisie, pollfds);

                    sprintf(cmd,"cat plateau");

                    if ((fp = popen(cmd, "r")) == NULL)
                        panic("Erreur lors de l'ouvertue du tube\n");

                    while(fgets(buf, TAILLE_BUF, fp) != NULL)
                        envoyerMessage(buf, pollfds);
                    
                    if (pclose(fp))
                        panic("Erreur lors de la fermeture du tube\n");
                    
                    system("echo Plateau du jeu : >> log");
                    wait(0);

                    system("cat plateau >> log");
                    wait(0);

                    system("echo >> log");
                    wait(0);

                    sleep(2);
                }
                //Incrémentaion du nombre de carte posé sur le plateau durant le tour
                nombreJoue++;
            }
            nbTour++;
        };
        nbManche++;
    }

    sleep(2);

    int gagnant = 0; 
    int perdant = 0;
    int min = 99; 
    int max = 0;

    for(i = 1; i <= joueurs + robots; i++){      //On cherche les joueurs ayant le moins et le plus de tête de boeufs
        if(vie[i] < min){                   // Gagnant
            min = vie[i];
            gagnant = i;
        }
        if(vie[i] > max){                   // Perdant
            max = vie[i];
            perdant = i;
        }
    }

    if(((nbManche-1) == manches) && !finPartie){                            //Fin de partie car toute les manches ont été jouées
        sprintf(saisie,"%d manches jouées, fin de la partie", manches);
    }
    else 
    if(finPartie){         //Fin de partie car un joueur a obtenu n têtes de boeufs     
        sprintf(saisie,"Fin de la partie, %s a eu %d têtes de boeufs", pseudo[perdant], max);
    }


    envoyerMessage(saisie, pollfds);                            //On envoie le message de fin

    sprintf(log, "echo \"%s\">> log", saisie);
    system(log);
    wait(0);

    sprintf(saisie,"Le gagnant est %s avec %d tête de boeufs", pseudo[gagnant], min);
    envoyerMessage(saisie, pollfds);                            //On envoie le vainceur

    sprintf(log, "echo \"%s\">> log", saisie);
    system(log);
    wait(0);


    printf("Terminaison du Serveur\n");

    sprintf(cmd,"./suppression.sh %d %d", joueurs, robots); //On supprime les fichiers générés par le serveur et génère fichier log en pdf
    system(cmd);
    wait(0);

    for(i = 1; i <= joueurs + robots; i++){
        if(i != gagnant)
            sprintf(cmd,"./ajoutStat.sh %s %d %d", pseudo[i], 0, vie[i]); //On ajoute le perdant 
        else
            sprintf(cmd,"./ajoutStat.sh %s %d %d", pseudo[i], 1, vie[i]); //On ajoute le gagnant
        system(cmd);
        wait(0);  
    }

    for(i = 1; i <= joueurs; i++){      //On ferme les descripteurs clients
        close(pollfds[i].fd);    
    }
    close(server_socket);               //On ferme le serveur
    free(cmd);
    free(buf);

    return 0;
}
