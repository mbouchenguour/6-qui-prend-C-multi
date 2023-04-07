#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#define TAILLE_BUF 128




int descClient; //Le descripteur
int nbJoueur; //Le numéro du client envoyé par le serveur

int cartes[10] = {0}; //Stocke les cartes du joueurs

//Affiche le message d'erreur et ferme l'application
void panic (const char *msg) {
  perror(msg);
  exit(1);
}

//Fin de la partie lors de ctrl+c
void fin (int sig) {
  if (close (descClient) != -1)
    printf("Descripteur bien fermé\n");
  exit(0);
}

//Vide le buffer de saisie du clavier
void clean_stdin(void)
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}
//Calcule la valeur de têtes de boeufs d'une carte
int nbTete(int carte){
  int valeur = 0;
  if(carte == 55)
    valeur = 7;
  else if(!(carte % 10))
    valeur = 3;
  else if(!(carte % 5))
    valeur = 2;
  else if(!(carte % 11))
    valeur = 5;
  else
    valeur = 1;
  return valeur;
}

//Permet de traiter unn message envoyé par le serveur
void traitement(char reponse[]){
  char buf[TAILLE_BUF] =""; //Buffer qui va contenir les messages recus et envoyés par/au le serveur
  bool valide = false;    //Booléen qui permet de continuer a demander à l'utilisateur de mettre une saisie si elle n'est pas correcte
  int carte = 0; //Var qui recoit les cartes du joueurs et la carte que le joueur veut joueur
  int emplacement = 0;  //Var qui stocke la valeur de l'emplacement souhaité par le joueur
  char delim[] = " ";   //Delimitateur qui permet de découper une chaine de caractères
  char * ptr; //Permet de découper une chaîne
  
  
  //Si le serveur demande de choisir une carte
  if(strcmp(reponse, "Choisir carte") == 0)
  {
    //On affiche les cartes du joueur
    printf("Voici vos cartes : \n\n");
    
    for(int i = 0; i < 10; i++){
      if(cartes[i] != 0)                     //On affiche pas les valeurs du tableau ayant 0
        printf("(%d)\t", nbTete(cartes[i]));
    }
    printf("\n");

    //On affiche les valeurs de têtes de boeufs
    for(int i = 0; i < 10; i++){
      if(cartes[i] != 0)
        printf("%d\t", cartes[i]);
    }

    //On demande au joueur de choisir une carte tant que la saisie n'est pas valide
    while(!valide){
      
      printf("\n\nChoississez une carte (mettre valeur) : ");
      scanf("%d", &carte);

      for(int i = 0; i < 10; i++){
        if((cartes[i] != 0) && (cartes[i] == carte)){
            valide = true;
            cartes[i] = 0;
        }
      }

      if(valide){
        printf("Saisie correct, en attente des autres joueurs\n");
        write(descClient, &carte, sizeof(carte)); //On envoie la réponse au serveur
      }
      else{
        clean_stdin();
        printf("Saisie incorrect, veuillez recommencer\n");
      }

    }
  }
  //Si la carte donné poser ne peux pas être placé par le serveur, on demande un emplacement
  else if(strcmp(reponse, "Placement impossible") == 0){
    emplacement = 0;
    printf("\nLa carte choisie n'a pas pu être posé\n");

    while(emplacement < 1 || emplacement > 4){
      printf("Veuillez choisir un emplacement (entre 1 et 4) : ");
      scanf("%d", &emplacement);
      if( emplacement >= 1 && emplacement <= 4){
        sprintf(buf, "%d", emplacement);
        write(descClient, buf, sizeof(buf));
      }
      else{
      	clean_stdin();
        printf("Saisie incorrect, veuillez recommencer\n\n");
      }
    }
  }
  //Affichage du plateau avec les valeurs de têtes de boeufs
  else if(strcmp(reponse, "Plateau du jeu :") == 0){
    printf("\n%s\n\n", reponse);
    for(int i =0; i<4; i++){
      int bufSize = read(descClient,buf,TAILLE_BUF);

      if(bufSize < 0)
        panic("Erreur de lors de la lecture des données");
      else{
        char buf2[TAILLE_BUF];

        buf[bufSize] = '\0';
        strcpy(buf2, buf);

        ptr = strtok(buf, delim);
        while (ptr != NULL){
          printf("(%d)\t", nbTete(atoi(ptr)));
          ptr = strtok(NULL, delim);
        }
        
        printf("\n");

        ptr = strtok(buf2, delim);
        while (ptr != NULL){
          printf("%d\t", atoi(ptr));
          ptr = strtok(NULL, delim);
        }
        printf("\n\n");

      }
    }
    printf("\n");
  }
  //Reception des cartes par le serveur
  else if(strcmp(reponse, "Cartes") == 0){
    for(int i =0; i<10; i++){
      int bufSize = read(descClient,&carte,sizeof(carte));

      if(bufSize < 0)
        panic("Erreur de lors de la lecture des données");
      else
        cartes[i] = carte;
    }
  } 
  //Sinon on print le message envoyé par le serveur
  else
  {
    printf("\n%s\n", reponse);
  }
}

int main (int argc, char *argv[]) {
  struct sigaction act;
  struct sockaddr_in addrServ;
  struct sockaddr_in addrClient;
  bool partie = true;   //Booleen mis a vrai tant que la partie n'est pas finie

  //Quitter proprement par ^C
  act.sa_handler = fin;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act,0);


    
  char *buf = (char*) malloc (TAILLE_BUF);    //Buffer qui permet d'envoyer et de stocker les messages reçu
  struct hostent *hote = gethostbyname(argv[1]); 
  if (hote == NULL) 
    panic("Serveur not found");

  //obtention d'un desc socket valide
  descClient = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  
  //nommage du socket client tel qu'il le serait par defaut
  addrClient.sin_family = AF_INET;
  addrClient.sin_addr.s_addr = INADDR_ANY; //recupere l'adresse de la machine locale
  addrClient.sin_port =  htons(0); //affecte le premier numero de port libre
  bind (descClient,(struct sockaddr *) &addrClient,(sizeof(struct sockaddr_in)));

  //nom du socket du serveur
  addrServ.sin_family = AF_INET;
  addrServ.sin_addr.s_addr = inet_addr (inet_ntoa (*(struct in_addr *)(hote->h_addr_list[0])));
  addrServ.sin_port = htons (55555);

  system("clear");
  wait(0);
  if (connect(descClient,(struct sockaddr *) &addrServ,(sizeof(struct sockaddr_in))) == -1)
    panic ("Connexion ratée");

  //Le serveur nous envoie le numéro du joueur après la connexion
  read(descClient,&nbJoueur,sizeof(nbJoueur));
  printf("Vous êtes le joueur numéro %i\n", nbJoueur);

  //On envoie notre pseudo au serveur
  printf("Quel est votre pseudo : ");
  scanf("%29s", buf);
  write(descClient, buf, sizeof(buf));

  /**
   * Tant que le connexion n'est pas coupé, 
   * on se met en lecture et on traite le message dans la fonction traitement
   * 
   * */ 
  while(partie){
    int bufSize = read(descClient,buf,TAILLE_BUF);
    if(bufSize < 0)
      panic("Erreur de lors de la lecture des données");
    if(bufSize == 0)
      partie = false;
    else
          traitement(buf);
  }
  

  //quitter proprement
  close (descClient);
  free(buf);
  return 0;
}


