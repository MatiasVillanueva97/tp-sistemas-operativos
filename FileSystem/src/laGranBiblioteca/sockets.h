void sigchld_handler(int s);

void *getSin_Addr(struct sockaddr*);

int conexionConKernel(char*, char*);

int crearSocketYBindeo(char*);

void escuchar(int);

char* recibir(int); // Toda esta funcion deberá ccambiar en el momento qeu defininamos el protocolo de paquetes de mensajes :)

int enviarMensaje(char* mensaje, int socket);

int handshakeCliente(int socket, int id);

int handshakeServidor(int socket,int id, int permitidos[]);
