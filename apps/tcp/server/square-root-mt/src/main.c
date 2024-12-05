#include "main.h"
#include "stdio.h"
#include "winsock2.h"
#include "threads.h" 
#include "net-utils.h"
#include "shared-data.h"

SOCKET server_socket = -1;
static int active_clients = 0;  // Лічильник активних клієнтів

// Функція для закриття сокету
void free_socket() {
	if (server_socket > 0) {
		closesocket(server_socket);
	}
}

// Функція для виведення правил використання
void usage(const char* exe_name) {
	printf("Usage:\n");
	printf("\t%s -p <port> -q <que_size>\n", exe_name);
}

// Функція старту сервера
int start(int argc, char* argv[]) {
	int port = DEFAULT_PORT;
	int queue_size = DEFAULT_QUEUE;

	if (argc >= 3) {
		char arg_line[128] = { 0 };

		combine_arg_line(arg_line, argv, 1, argc);

		int ret = sscanf(arg_line, "-p %d -q %d", &port, &queue_size);
		if (ret < 1) {
			usage(argv[0]);
			return -1;
		}
	}

	return init_server(port, queue_size);
}

// Функція ініціалізації сервера
int init_server(short port, int queue_size) {
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		printf("Cannot create socket\n");
		return -1;
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) != 0) {
		printf("Cannot bind socket to port %d\n", port);
		return -2;
	}

	if (listen(server_socket, queue_size) != 0) {
		printf("Cannot listen on port %d\n", port);
		return -3;
	}

	printf("Server is running on port %d\n", port);

	return process_connections();  // Виклик функції обробки підключень
}

// Основна функція для обробки підключень
int process_connections() {
	SOCKET client_socket = -1;

	// Основний цикл сервера
	while (1) {
		struct sockaddr_in client_addr;
		int len = sizeof(client_addr);

		// Приймаємо нове з'єднання
		client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len);
		if (client_socket == INVALID_SOCKET) {
			printf("Error accepting connection\n");
			continue;
		}

		// Збільшуємо кількість активних клієнтів
		active_clients++;

		// Виводимо кількість підключень
		printf("New connection established. Active clients: %d\n", active_clients);

		thrd_t trd;

		// Створюємо потік для обробки клієнта
		thrd_create(&trd, process_connection, (void*)client_socket);
		thrd_detach(trd);  // Окремо запускаємо потік, щоб не блокувати основний потік
	}

	return 0;
}

// Функція для обробки кожного з'єднання клієнта
void process_connection(void* arg) {
	SOCKET client_socket = (SOCKET)arg;

	if (client_socket <= 0) {
		printf("Invalid socket\n");
		return;
	}

	struct sockaddr_in client_addr;
	int len = sizeof(client_addr);

	getsockname(client_socket, (struct sockaddr*)&client_addr, &len);
	printf("Connection established from: %s\n", inet_ntoa(client_addr.sin_addr));

	char buffer[1024];

	// Основний цикл для обробки клієнтських запитів
	while (1) {
		int ret = recv(client_socket, buffer, sizeof(buffer), 0);
		if (ret <= 0) {
			printf("Connection closed\n");
			break;
		}

		printf("<==== Received: [%d bytes] Data: %s\n", ret, buffer);

		// Надсилаємо підтвердження клієнту
		ret = send(client_socket, buffer, ret, 0);
		if (ret <= 0) {
			printf("Error sending data to client\n");
			break;
		}

		printf("====> Sent: [%d bytes]\n", ret);
	}

	// Закриваємо сокет після обробки
	closesocket(client_socket);

	// Зменшуємо кількість активних клієнтів після закриття з'єднання
	active_clients--;
	printf("Connection closed. Active clients: %d\n", active_clients);
}




/*#include "main.h"

SOCKET server_socket = -1;

void free_socket()
{
	if (server_socket > 0)
	{
		closesocket(server_socket);
	}
}
void usage(const char* exe_name)
{
	printf("Usage:\n");
	printf("\t%s -p <port> -q <que_size>", exe_name);
}

int start(int argc, char* argv[])
{
	int port = DEFAULT_PORT;

	int queue_size = DEFAULT_QUEUE;

	if (argc >= 3)
	{
		char arg_line[128];
		
		memset(arg_line, 0, sizeof(arg_line));

		combine_arg_line(arg_line, argv, 1, argc);

		int ret = sscanf(arg_line, "-p %d -q %d", &port, &queue_size);

		if (ret < 1) {
			usage(argv[0]);
			return -1;
		}
	}

	return init_client(port, queue_size);
}

int init_client(short port, int queue_size)
{
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (server_socket <= 0)
	{
		printf("Cannot create socket\n");
		return -1;
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_socket, (struct sockaddr*)&address, sizeof(address))) {
		printf("Cannot bind socket to port %d\n", port);
		return -2;
	}
	
	if (listen(server_socket, queue_size))
	{
		printf("Cannot listen socket on port %d\n", port);
		return -3;
	}

	printf("Sever run on port %d\n", port);

	return process_connections();
}

int process_connections()
{
	SOCKET client_socket = -1;

	//main loop
	while (1)
	{
		struct sockaddr_in client_addr;

		int len = sizeof(client_addr);

		client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len);

		thrd_t trd;

		thrd_create(&trd, process_connection, client_socket);
	}

	return 0;
}

void process_connection(void* arg)
{
	SOCKET client_socket = (SOCKET)arg;

	if (client_socket <= 0)
	{
		printf("Error incomming connection\n");
		return;
	}

	struct sockaddr_in client_addr;

	int len = sizeof(client_addr);

	getsockname(client_socket, (struct sockaddr*)&client_addr, &len);

	printf("Establish connection from: %s\n", inet_ntoa(client_addr.sin_addr));

	// client loop
	while (1)
	{
		struct QuadraticEquation request;

		int ret = recv(client_socket, (char*)&request, sizeof(request), 0);

		if (ret <= 0)
		{
			printf("Close connection\n");
			return;
		}

		printf("<==== Received: [%d bytes]\n", ret);

		struct SquareRootData response;

		process_request(&request, &response);

		ret = send(client_socket, (char*)&response, sizeof(response), 0);

		if (ret <= 0)
		{
			printf("Close connection\n");
			return;
		}

		printf("====> Sent: [%d bytes]\n", ret);
	}

	if (client_socket > 0)
	{
		return closesocket(client_socket);
	}
}

int process_request(struct QuadraticEquation* request, struct SquareRootData* response)
{
	double a = request->a, b = request->b, c = request->c;
	
	char bc = '+';
	if (b < 0)
	{
		bc = '\0';
	}

	char cc = '+';
	if (c < 0)
	{
		cc = '\0';
	}

	printf("Equastion %.5f*x^2%c%.5f*x%c%.5f=0\n", a, bc, b, cc, c);

	double D = b * b - 4 * a * c;

	if (D < 0)
	{
		response->result = NO_ROOT;
		return 0;
	}

	response->x1 = (-b - sqrt(D)) / 2 / a;

	response->x2 = (-b + sqrt(D)) / 2 / a;

	if (fabs(response->x1 - response->x2) < 1e-9)
	{
		response->result = ONE_ROOT;
	}
	else {
		response->result = TWO_ROOT;
	}

	printf("Results: x1=%.3f, x2=%.3f\n", response->x1, response->x2);

	return 0;
}
*/