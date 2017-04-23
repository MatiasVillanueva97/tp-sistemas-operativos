typedef struct{
	uint32_t tipo;
	uint32_t tamano;
} __attribute__((packed))
Header;
void sigchld_handler(int s);

void *getSin_Addr(struct sockaddr*);

int conexionConKernel(char*, char*);

int crearSocketYBindeo(char*);

void escuchar(int);

void recibirMensaje(int ,void* ); // Toda esta funcion deberá ccambiar en el momento qeu defininamos el protocolo de paquetes de mensajes :)

int enviarMensaje(int , int , void* , int );

int handshakeCliente(int socket, int id);

int handshakeServidor(int socket,int id, int permitidos[]);


