#include "main.h"
#include "stdio.h"
#include "string.h"
#include "winsock2.h"
#include "net-utils.h"
#include "shared-data.h"
#include "threads.h"  // Підключаємо бібліотеку для роботи з потоками

#define DEFAULT_PORT 7701

// Структура для передачі даних до потоку
typedef struct {
    const char* host;
    short port;
} ClientData;

// Функція виведення правил використання
void usage(const char* exe_name) {
    printf("Usage:\n");
    printf("\t%s -h <host> -p <port>\n", exe_name);
}

// Оголошення функції для кожного клієнтського потоку
int client_thread(void* arg);

// Функція старту клієнта
int start(int argc, char* argv[]) {
    char host[2048] = "";
    int port = DEFAULT_PORT;
    int num_clients;

    if (argc >= 3) {
        char arg_line[128] = { 0 };

        combine_arg_line(arg_line, argv, 1, argc);

        int ret = sscanf(arg_line, "-h %s -p %d", host, &port);
        if (ret < 2) {
            usage(argv[0]);
            return -1;
        }
    }
    else {
        printf("Enter server address (-h <host> -p <port>): ");
        int ret = scanf("-h %s -p %d", host, &port);
        if (ret < 2) {
            usage(argv[0]);
            return -2;
        }
    }

    printf("Enter number of clients to simulate: ");
    int ret = scanf("%d", &num_clients);
    if (ret != 1 || num_clients < 1) {
        printf("Invalid number of clients\n");
        return -3;
    }

    // Створення декількох підключень
    thrd_t* threads = malloc(num_clients * sizeof(thrd_t)); // Масив для зберігання потоків
    if (threads == NULL) {
        printf("Failed to allocate memory for threads\n");
        return -4;
    }

    for (int i = 0; i < num_clients; ++i) {
        // Створення структури для передачі в потік
        ClientData* client_data = malloc(sizeof(ClientData));
        if (client_data == NULL) {
            printf("Failed to allocate memory for client data\n");
            return -5;
        }
        client_data->host = host;
        client_data->port = port;

        // Запуск кожного клієнта в окремому потоці
        int ret = thrd_create(&threads[i], client_thread, client_data);
        if (ret != thrd_success) {
            printf("Failed to create thread for client %d\n", i + 1);
            return -6;
        }
    }

    // Очікуємо завершення всіх потоків
    for (int i = 0; i < num_clients; ++i) {
        thrd_join(threads[i], NULL);
    }

    free(threads);  // Вивільняємо пам'ять для потоків
    return 0;
}

// Функція для кожного клієнтського потоку
int client_thread(void* arg) {
    ClientData* data = (ClientData*)arg;

    int result = init_client(data->host, data->port);

    free(arg);  // Вивільняємо пам'ять після завершення потоку
    return result;
}

// Функція ініціалізації підключення клієнта
int init_client(const char* host, short port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Cannot create client socket\n");
        return -1;
    }

    printf("Socket created\n");

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    char target_host[2048] = "";
    resolve_address(host, target_host);

    server_address.sin_addr.s_addr = inet_addr(target_host);

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        printf("Cannot connect to server %s:%d\n", target_host, port);
        return -2;
    }

    printf("Connected to server: %s:%d\n", target_host, port);

    return process_connection(client_socket);
}

// Функція обробки з'єднання з сервером
int process_connection(SOCKET client_socket) {
    char buffer[1024];

    printf("Enter message to send: ");
    int ret = scanf("%s", buffer);
    if (ret < 1) {
        printf("Cannot read input\n");
        return -10;
    }

    // Надсилаємо дані на сервер
    ret = send(client_socket, buffer, strlen(buffer), 0);
    if (ret <= 0) {
        printf("Error sending data\n");
        return -11;
    }

    printf("====> Sent: [%d bytes]\n", ret);

    // Отримуємо відповідь від сервера
    ret = recv(client_socket, buffer, sizeof(buffer), 0);
    if (ret <= 0) {
        printf("Error receiving data\n");
        return -12;
    }

    printf("<==== Received: [%d bytes] Data: %s\n", ret);

    // Закриваємо з'єднання
    closesocket(client_socket);
    return 0;
}

































/*#include "main.h"

void usage(const char* exe_name)
{
	printf("Usage:\n");
	printf("\t%s -h <host> -p <port>\n", exe_name);
}

int start(int argc, char* argv[])
{
	char host[2048] = "";

	int port;

	if (argc >= 3)
	{
		char arg_line[4 * 1024] = "";
		
		combine_arg_line(arg_line, argv, 1, argc);

		int ret = sscanf(arg_line, "-h %s -p %d", host, &port);
		
		if (ret < 2) {
			usage(argv[0]);
			return -1;
		}
	}
	else {
		printf("Enter server address (-h <host> -p <port>): ");

		int ret = scanf("-h %s -p %d", host, &port);

		if (ret < 2)
		{
			usage(argv[0]);

			return -2;
		}
	}

	return init_client(host, port);
}

int init_client(const char* host, short port)
{
	SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (client_socket <= 0)
	{
		printf("Cannot create client socket\n");
		return -1;
	}

	printf("Socket created\n");

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);

	char target_host[2048] = "";
	resolve_address(host, target_host);

	server_address.sin_addr.s_addr = inet_addr(target_host);

	if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address))) {
		printf("Cannot connect to port %s:%d\n", target_host, port);
		return -2;
	}

	printf("Success connection to server: %s:%d\n", target_host, port);

	return process_connection(client_socket);
}

int process_connection(SOCKET client_socket)
{
	struct QuadraticEquation request;

	printf("Enter a, b, c:");

	int ret = scanf("%lf %lf %lf", &(request.a), &(request.b), &(request.c));

	if (ret < 3) {
		printf("Cannot read quadratic coeffs\n");
		return -10;
	}

	ret = send(client_socket, (char*)&request, sizeof(request), 0);

	if (ret <= 0)
	{
		printf("Sending data error\n");
		return -11;
	}

	printf("====> Sent: [%d bytes]\n", ret);
	
	struct SquareRootData response;

	ret = recv(client_socket, (char*)&response, sizeof(response), 0);

	printf("<==== Received echo: [%d bytes]\n", ret);

	process_request(&request, &response);

	return closesocket(client_socket);
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

	printf("Equastion %.5f*x^2%c%.5f*x%c%.5f=0 ", a, bc, b, cc, c);

	switch (response->result)
	{
	case NO_ROOT:
		printf("has no root\n");
		break;
	case ONE_ROOT:
		printf("has equal roots: x1=x2=%.5f\n", response->x1);
		break;
	default:
		printf("has roots: x1=%.5f, x2=%.5f\n", response->x1, response->x2);
		break;
	}
	return 0;
}
*/