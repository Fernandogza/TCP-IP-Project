/*
	Cliente TCP que lee un string del teclado y lo manda al servidor, después debe recibir
	el mismo string en mayúsculas.
	La dirección IP del servidor se debe de especificar como parámetro en al línea de
	comandos y el puerto es el 5000
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#define PUERTO 5000

void tree(char *name, int level) {
    DIR *dir;
    struct dirent *entry;

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
            printf("%*s[%s]\n", level*2, "", entry->d_name);
            tree(path, level + 1);
        }
        else
            printf("%*s- %s\n", level*2, "", entry->d_name);
    } while (entry = readdir(dir));
    closedir(dir);
}

void copiar() {
	int fd_origen;
	int fd_destino;
	int nbytes;
	char buffer [100];
	/*Apertura del archivo en modo solo lectura*/
         if ((fd_origen=open("Cliente.txt",O_RDONLY))== -1) {
		exit(-1);
	}
	/* Apertura o creacion de archivos en modo solo escritura*/
	if ((fd_destino=open("ClienteCopia.txt",O_WRONLY|O_TRUNC|O_CREAT, 0666))== -1) {
		exit(-1);
	}
 	/* copiamos el archivo origen en el archivo destino. */
	while ((nbytes=read(fd_origen, buffer, sizeof buffer))> 0) {
		write(fd_destino, buffer, nbytes);
	}
	close(fd_origen);
	close(fd_destino);	
}

int main(int argc, char *argv[]){
	int sockfdServidor;			// Socket para identificar al servidor para mandar y recibir los datos
	struct sockaddr_in servidor;	// Estructura para identificar al servidor
	struct hostent *he;			// Para leer la dirección IP del servidor
	int numbytes;				// Para almacenar la cantidad de bytes recibidos en "recv" y trasmitidos en "send"
	char buffer[80];			// Buffer para recibir los bytes y para transmitir la respuesta
	char user[80];
  char received[80];
  char *command;
	char pass[80];
	int auth = 0;
	int intentos = 3;
	// Verificar que la llamada al programa sea correcta clienteTCP dirIPServidor
	if (argc != 2){
		fprintf(stderr,"uso: clienteTCP dirIPServidor\n");
		exit(1);
	}
	// Obtener la dirección IP del servidor
	if ((he=gethostbyname(argv[1])) == NULL){
		perror("gethostbyname");
		exit(1);
	}
	// Crear el socket tipo TCP/IP y TCP (Stream)
	if ((sockfdServidor = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}
	// Crear la estructura para recibir los datos
	servidor.sin_family = AF_INET;         // TCP/IP
	servidor.sin_port = htons(PUERTO);     // Puerto para recepción en formato de red. htons = Host TO Network Short
	servidor.sin_addr = *((struct in_addr *)he->h_addr); // Dirección del servidor
	memset(&(servidor.sin_zero), '\0', 8); // llenar el resto de la estructura con ceros

	// Petición de conexión al servidor
	if (connect(sockfdServidor, (struct sockaddr *)&servidor, sizeof servidor) == -1) {
		perror("connect");
		exit(1);
	}
	//Leer del teclado, enviar al servidor y obtener respuesta
	do{
		// Leer string del teclado
		printf("Bienvenido\n");
		while(auth == 0 && intentos > 0) {
			fgets(received, 80, stdin);
			if (strlen(received) == 1)
				strncpy(buffer, "FIN* ", 80);
			received[strlen(received)-1] = '\0';
			strcpy(user, received);
      command = strtok(received, " ");
      if (strcmp(command, "log") == 0 && strlen(user) >= 5) {
          // Enviar al servidor el usuario tecleado
          if ((numbytes=send(sockfdServidor, user, strlen(user), 0)) == -1){
          perror("SEND");
          exit(1);
          }
          printf("Contraseña: \n");
          fgets(pass, 80, stdin);
          pass[strlen(pass)-1] = '\0';
          // Enviar al servidor el password tecleado
          if ((numbytes=send(sockfdServidor, pass, strlen(pass), 0)) == -1){
          perror("SEND");
          exit(1);
          }
          // Recibir respuesta del servidor
          if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
          perror("Error en el RECV");
          exit(1);
          }
          buffer[numbytes] = '\0';
          // Se despliega el mensaje recibido.
          printf("%s\n", buffer);
          // Se verifica si la autorizacion fue exitosa.
          if(strcmp(buffer, "Autorizacion exitosa.") == 0) {
            auth = 1;	
          }
          // Si falla la autorizacion, se reduce en 1 los intentos
          else {
            intentos -=1;
            printf("Intente nuevamente.\n");			
          }
      }
      else if (strcmp(command,"?") == 0) {
        printf("Comandos disponibles:\n - log <username>\n");
      }
      else {
        printf("Comando incorrecto. Intente '?' para la lista de comandos\n");
      }
    }
		// Si se acaban los 3 intentos, se termina la conexion.
		if(intentos == 0) {
			strcpy(buffer, "FIN*");
			printf("Numero de intentos excedido. Cerrando conexion.\n");		
		}
		while(strcmp(buffer, "FIN*") != 0) {
			memset(&buffer[0], 0, sizeof(buffer));
			printf("Escriba el comando que quiera utilizar:\n");
			fgets(buffer, 80, stdin);
			buffer[strlen(buffer)-1] = '\0';
			if(strcmp(buffer, "?") == 0) {
				printf("Comandos disponibles\n");
				printf("- dirR\n");
				printf("- rmR\n");
				printf("- cpR\n");
				printf("- dirL\n");
				printf("- rmL\n");
				printf("- cpL\n");
				printf("- ipinfoL\n");
				printf("- ipinfoR\n");
			} else if (strcmp(buffer, "dirR") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
        if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
            perror("Error en el RECV");
            exit(1);
          }
        while(1) {
          buffer[numbytes] = '\0';
          if (strcmp(buffer, "-*-") == 0){
            break;
          }
          printf("%s\n", buffer);	
          if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) <= 0) {
            perror("Error en el RECV");
            exit(1);
          }
        }
      }
			else if (strcmp(buffer, "dirL") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
				tree(".", 0);
			}
			else if (strcmp(buffer, "rmR") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
				if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
					perror("Error en el RECV");
					exit(1);
				}
				buffer[numbytes] = '\0';
				printf("%s\n", buffer);				
			}
			else if (strcmp(buffer, "rmL") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
				if(remove("borrarLocal.txt") != 0) {
					printf("Error al borrar el archivo.");			
				}
				else {
					printf("Archivo borrado satisfactoriamente.");
				}
			}
			else if (strcmp(buffer, "cpR") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
				if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
					perror("Error en el RECV");
					exit(1);
				}
				buffer[numbytes] = '\0';
				printf("%s\n", buffer);			
			}
			else if (strcmp(buffer, "cpL") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
				copiar();
				printf("Archivo copiado exitosamente.\n");
			} else if (strcmp(buffer, "ipinfoL") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}	
				if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
					perror("Error en el RECV");
					exit(1);
				}
				buffer[numbytes] = '\0';
				printf("%s\n", buffer);		
			}
			else if (strcmp(buffer, "ipinfoR") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}	
				if ((numbytes=recv(sockfdServidor, buffer, 80-1 , 0)) == -1) {
					perror("Error en el RECV");
					exit(1);
				}
				buffer[numbytes] = '\0';
				printf("%s\n", buffer);		
			}
			else if (strcmp(buffer, "FIN*") == 0) {
				if ((numbytes=send(sockfdServidor, buffer, strlen(buffer), 0)) == -1){
					perror("SEND");
					exit(1);
				}
			} 	
			else {
				printf("Comando incorrecto\n"); 			
			}
		}
	}while(strcmp(buffer, "FIN*") != 0);
	close(sockfdServidor);
	return 0;
}
