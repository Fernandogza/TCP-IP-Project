/*
	Servidor TCP que recibirá petición de MULTIPLES clientes, De cada cliente recibirá
	strings. Se responderá con el String transformado a mayúsculas al cliente.
	Atenderá a MULTIPLES CLIENTES. La aplicación escucha en el puerto 5000
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PUERTO 5000
#define BACKLOG 50

void tree(char *name, int level, int sockFdCliente, int numbytes) {
  char line[80];
  DIR *dir;
  struct dirent *entry;
  char buffer[80] = "";

  if (!(dir = opendir(name)))
    return;
  if (!(entry = readdir(dir)))
    return;

  do {
    if (entry->d_type == DT_DIR) {
      char path[1024];
      int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
      path[len] = 0;
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      sprintf(line, "%*s[%s]\n", level*2, "", entry->d_name);
      if ((numbytes=send(sockFdCliente, line, strlen(line), 0)) == -1) {
        perror("SEND");
        exit(1);
      }

      tree(path, level + 1, sockFdCliente, numbytes);
    }
    else {
      sprintf(line, "%*s|- %s\n", level*2, "", entry->d_name);
      if ((numbytes=send(sockFdCliente, line, strlen(line), 0)) == -1) {
        perror("SEND");
        exit(1);
      }

    }
     } while (entry = readdir(dir));
  closedir(dir);
}
void copiar() {
	int fd_origen;
	int fd_destino;
	int nbytes;
	char buffer [100];
	/*Apertura del archivo en modo solo lectura*/
         if ((fd_origen=open("Servidor.txt",O_RDONLY))== -1) {
		exit(-1);
	}
	/* Apertura o creacion de archivos en modo solo escritura*/
	if ((fd_destino=open("ServidorCopia.txt",O_WRONLY|O_TRUNC|O_CREAT, 0666))== -1) {
		exit(-1);
	}
 	/* copiamos el archivo origen en el archivo destino. */
	while ((nbytes=read(fd_origen, buffer, sizeof buffer))> 0) {
		write(fd_destino, buffer, nbytes);
	}
	close(fd_origen);
	close(fd_destino);	
}
pthread_t c;
struct socket{
	int socketFdCli;
	struct sockaddr_in cli;	
};

void *atiende(void *cliente){
	printf("En el hilo atiende\n");
	struct socket* sock = (struct socket*) cliente;
	int sockFdCliente = sock->socketFdCli;
	struct sockaddr_in cli = sock->cli;
	int numbytes;			// Para almacenar la cantidad de bytes recibidos en "recv" y trasmitidos en "send"
	char buffer[80];		// Buffer para recibir los bytes y para transmitir la respuesta
	char tUser[80];			// Buffer para recibir el string "log <usuario>".
	char *tempUser;			// pointer al token que contiene el username.
	char user[80];			// Buffer que almacena el nombre del usuario escrito.
	char pass[80];			// Buffer que almacena el password del usuario escrito.
	char Fuser[80];			// Buffer que almacena el usuario cargado del archivo.
	char Fpass[80];			// Buffer que almacena el password cargado del archivo.
	char commands[80];		// Buffer que almacena el comando escrito por el cliente con su fecha y hora.
	char connectionTime[80];
	char disconnectionTime[80];
	char usuarioActual[80];
	FILE * pFile;
	FILE * logFile;
	
	int auth = 0;			// Booleano para saber si ya se establecio una autorizacion
	int intentos = 3;		// Numero predeterminado de intentos para autentificarze.

	printf("Conexión de %s/%d\n",inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
	memset(&buffer[0], 0, sizeof(buffer));
	do{
		while(auth == 0 && intentos > 0) {
			pFile = fopen("usuarios.txt", "r");	// Conexion al archivo que contiene los usuarios/passwords.
			memset(&tUser[0], 0, sizeof(tUser)); 	
			memset(&pass[0], 0, sizeof(pass)); 	
			// Recibir el usuario de un cliente
			if ((numbytes=recv(sockFdCliente, tUser, 80-1 , 0)) == -1) {
				perror("Error en el RECV");
				exit(1);
			}
			tUser[numbytes] = '\0';
			// Se tokeniza el string recibido "log <user>"
			tempUser = strtok(tUser, " ");
			tempUser = strtok(NULL, " ");
			// Se guarda el token que contiene el nombre del usuario en su buffer correspondiente.
			strcpy(user, tempUser);
			// Recibir el password de un cliente
			if ((numbytes=recv(sockFdCliente, pass, 80-1 , 0)) == -1) {
				perror("Error en el RECV");
				exit(1);
			}
			pass[numbytes] = '\0';
			// Checar si el usuario y la password coinciden con los datos guardados en el archivo.
			// Se comparan hasta que se acabe el archivo o si se autentifican antes.
			while(fgets(Fuser, 80, pFile) != NULL && auth == 0) {
				size_t ln = strlen(Fuser) - 1;
				// Quitar el end-line que carga el fgets
				if (Fuser[ln] == '\n')
				    Fuser[ln] = '\0';
				fgets(Fpass, 80, pFile);
				ln = strlen(Fpass) - 1;
				if (Fpass[ln] == '\n')
				    Fpass[ln] = '\0';
				//Verificar la auteticidad.
				if(strcmp(Fuser, user) == 0 && strcmp(Fpass, pass) == 0) {
					auth = 1;
					strcpy(buffer, "Autorizacion exitosa.");
					// Se envia el msj de Autorizacion exitosa al cliente.
					if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
						perror("SEND");
						exit(1);
					}
				}
			}
			fclose(pFile);
			// Si se llega al final del archivo y las credenciales no fueron correctas, se le manda un msj al cliente.
			if(auth == 0) {
				strcpy(buffer, "Credenciales incorrectas.");
				if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
					perror("SEND");
					exit(1);
				}
				intentos-= 1;			
			}
		}
		// Si se acaban los intentos para establecer una conexion completa, esta se cierra.
		if(intentos == 0) {
			strcpy(buffer, "FIN*");		
		}
     // se abre el archivo log donde se guarda quien se conecto y desconecto
    logFile = fopen("log.txt", "a+");
    strcat(connectionTime, "Conexion: ");
    strcat(connectionTime, user);
    strcat(connectionTime, " ");
    time_t now;
		time(&now);
    strcat(connectionTime, ctime(&now));
    fputs(connectionTime, logFile);
    strcpy(usuarioActual, user);

		strcat(user, ".txt");
		// se abre el archivo Log donde se guardaran los comandos que hace el usuario
		pFile = fopen(user, "a+");
   		// se estaran recibiendo comandos hasta que el cliente mande el msj de FIN*
		while(strcmp(buffer, "FIN*") != 0) {
			memset(&buffer[0], 0, sizeof(buffer)); 
			// se recibe el comando del usuario
			if ((numbytes=recv(sockFdCliente, buffer, 80-1 , 0)) == -1) {
				perror("Error en el RECV");
				exit(1);
			}	
			buffer[numbytes] = '\0';
			time_t now;
			time(&now);
			printf("%s, %s", buffer, ctime(&now));
			// se agrega el comando al log, con su fecha y hora.
			strcat(commands, buffer);
			strcat(commands, " ");
			strcat(commands, ctime(&now));
			fputs(commands, pFile);
			// se checa que comando emitio el cliente.
			if(strcmp(buffer, "dirR") == 0) {
				// se manda
        tree(".", 0, sockFdCliente, numbytes);
        //create a delay 
        sleep(1);
        if ((numbytes=send(sockFdCliente, "-*-", 3, 0)) == -1) {
          perror("SEND");
          exit(1);
        }
      }
			else if (strcmp(buffer, "rmR") == 0) {
				// se borra un archivo predeterminado del servidor.
				if(remove("borrarRemoto.txt") != 0) {
					strcpy(buffer, "Error al borrar el archivo.");			
				}
				else {
					strcpy(buffer, "Archivo borrado satisfactoriamente.");
				}
				// se envia un msj al cliente, satisfactoria o no.
				if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
					perror("SEND");
					exit(1);
				}
			}
			else if (strcmp(buffer, "cpR") == 0) {
				// se copia un archivo predeterminado del servidor.				
				copiar();
				strcpy(buffer, "Archivo copiado exitosamente.");
				if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
					perror("SEND");
					exit(1);
				}
			}
			else if (strcmp(buffer, "ipinfoL") == 0) {
				memset(&buffer[0], 0, sizeof(buffer));
				// se guarda el ip del cliente en el buffer y se le reenvia el msj.
				strcat(buffer, ("IP de la maquina local: "));
				strcat(buffer, inet_ntoa(cli.sin_addr));
				if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
					perror("SEND");
					exit(1);
				}
			}
			else if (strcmp(buffer, "ipinfoR") == 0) {
				// se obtiene el ip del servidor.
				int fd;
				struct ifreq ifr;
				fd = socket(AF_INET, SOCK_DGRAM, 0);
				ifr.ifr_addr.sa_family = AF_INET;
	 			strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);  //cambiar eth0 por la interfaz a la que este conectada el servidor.
				ioctl(fd, SIOCGIFADDR, &ifr);
				close(fd);
				memset(&buffer[0], 0, sizeof(buffer));
				strcat(buffer, ("IP de la maquina remota: "));
				strcat(buffer, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
				if ((numbytes=send(sockFdCliente, buffer, strlen(buffer), 0)) == -1) {
					perror("SEND");
					exit(1);
				}
			}
		}	
	}while(strcmp(buffer, "FIN*") != 0);
  time_t now;
  time(&now);
  strcat(disconnectionTime, "Desconexion: ");
  strcat(disconnectionTime, usuarioActual);
  strcat(disconnectionTime, " ");
  strcat(disconnectionTime, ctime(&now));
  fputs(disconnectionTime, logFile);

  fclose(logFile);
	fclose(pFile);
	close(sockFdCliente);
}

int main(void){
	int sockfdConexion;		// Socket del servidor para recibir peticiones de conexión
	struct sockaddr_in yo;    	// Datos del servidor para recibir datagramas

	// Crear el socket tipo TCP/IP y TCP (Stream)
	if ((sockfdConexion = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}
	// Crear la estructura para recibir los datos
	yo.sin_family = AF_INET;         // TCP/IP
	yo.sin_port = htons(PUERTO);     // Puerto para recepción en formato de red. htons = Host TO Network Short
	yo.sin_addr.s_addr = INADDR_ANY; // Dirección del servidor (local)
	memset(&(yo.sin_zero), '\0', 8); // llenar el resto de la estructura con ceros

	// Enlace del socket sockfd con la estructura para recibir (dirIP, protocolo y puerto
	if (bind(sockfdConexion, (struct sockaddr *)&yo, sizeof(struct sockaddr)) == -1){
		perror("Error en el bind");
		exit(1);
	}
	
	// Establecer el tamaño (BACKLOG) de la cola para escuchar a peticiones de conexión simultáneas
	if (listen(sockfdConexion, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	// Esperar por una petición de conexión
	printf("Servidor escuchando en el puerto 5000\n"); // Esperando por peticiones de conexión

	do{
		int sockFdCliente;
		struct sockaddr_in cliente; 
		int addr_len = sizeof(struct sockaddr);	// Solamente para ponerlo como parámetro variable
		struct socket cliN;

		if ((sockFdCliente = accept(sockfdConexion, (struct sockaddr *)&cliente, &addr_len)) == -1) {
			perror("accept");
			exit(1);
		}
		cliN.socketFdCli = sockFdCliente;
		cliN.cli = cliente;
		printf("Conexión de %s/%d\n",inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));
		pthread_create(&c, NULL, atiende, &cliN);
	}while(1);
	close(sockfdConexion);
	pthread_exit(0);
	return 0;
}
