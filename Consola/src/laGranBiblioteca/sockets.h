void sigchld_handler(int s);

void *get_in_addr(struct sockaddr*);

int conexionConKernel(char*, char*);

int crearSocketYBindeo(int);

void escuchar(int);

char* recibir(int); // Toda esta funcion deberá ccambiar en el momento qeu defininamos el protocolo de paquetes de mensajes :)

void enviarMensaje(char* mensaje, int socket);

int handshakeCliente(int socket, int id);

int handshakeServidor(int socket,int id, int permitidos[]);
