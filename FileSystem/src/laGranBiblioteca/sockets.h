typedef struct{
	uint32_t tipo;
	uint32_t tamano;
} __attribute__((packed))
Header;
void sigchld_handler(int);

void *getSin_Addr(struct sockaddr*);

int conexionConKernel(char*, char*);

int crearSocketYBindeo(char*);

void escuchar(int);

void recibirMensaje(int ,void* ); // Toda esta funcion deberá ccambiar en el momento qeu defininamos el protocolo de paquetes de mensajes :)

int enviarMensaje(int , int , void* , int );

int handshakeCliente(int, int );

int handshakeServidor(int,int , int[]);

void *deserializador(Header, int);

void* serializar (int, void* , int);
