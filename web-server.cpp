#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

#include <thread>
#include <chrono>


int getIPv4fromName(char *argv[], struct addrinfo* res, char *ipstr){
	struct addrinfo hints;

	// hints - modo de configurar o socket para o tipo de transporte
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	// IPv4
	hints.ai_socktype = SOCK_STREAM;	// TCP

	// funcao de obtencao do endereco via DNS - getaddrinfo
	// funcao preenche o buffer "res" e obtem o codigo de resposta "status"
	int status = 0;
	if ( (status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0 ) {
		std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
		return 2;
	}

	std::cout << "IP addressees for " << argv[1] << ": " << std::endl;

	// a estrutura de dados eh generica e portanto precisa de type cast
	struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;

	
	// conversao do endereco IP para string
	char stri[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(res->ai_family, &(ipv4->sin_addr), stri, sizeof(stri));
	strcpy(ipstr, stri);
	std::cout << " " << ipstr << std::endl;

	return 0;
}


int main(int argc, char *argv[]){
	struct addrinfo* res;

	if (argc != 4) {
        	std::cerr << "usage: web-server host port dir" << std::endl;
        	return 1;
	}

	// cria um socket para IPv4, handshake socket, para  protocolo TCP
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	

	// SOCKETS config
	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		return 1;
	}
	
	char ipstr[INET_ADDRSTRLEN] = {'\0'};
	getIPv4fromName(argv, res, ipstr);

	// struct sockaddr_in{
	// short 		sin_family;	// e.g. AF_INET, AF_INET6
	// unsigned short	sin_port;	// e.g. htons(3490)
	// struct in_addr 	sin_addr;	// see struct in_addr, below
	// char 		sin_zero[8];	// zero this if you want to use IPv4
	// };

	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(std::stoi(argv[2]));	// stoi converte string to int
	addr.sin_addr.s_addr = inet_addr(ipstr);
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	
	// realizar o bind (registrar a porta para uso com o SO) para o socket
  	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    	perror("bind");
    	return 2;
  	}

  	// colocar o socket em modo de escuta, ouvindo a porta 
  	if (listen(sockfd, 1) == -1) {
    	perror("listen");
    	return 3;
  	}

	// aceitar a conexao TCP
  	// verificar que sockfd e clientSockfd sao sockets diferentes
  	// sockfd eh a "socket de boas vindas"
  	// clientSockfd eh a "socket diretamente com o cliente"
  	struct sockaddr_in clientAddr;
  	socklen_t clientAddrSize = sizeof(clientAddr);
  	int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

  	if (clientSockfd == -1) {
    	perror("accept");
    	return 4;
  	}

  	// usa um vetor de caracteres para preencher o endereço IP do cliente
  	inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  	std::cout << "Accept a connection from: " << ipstr << ":" << ntohs(clientAddr.sin_port) << std::endl;

  	// faz leitura e escrita dos dados da conexao 
  	// utiliza um buffer de 20 bytes (char)
  	bool isEnd = false;
  	char buf[20] = {0};
  	std::stringstream ss;
	

	while (!isEnd) {
    	// zera a memoria do buffer
    	memset(buf, '\0', sizeof(buf));

    	// recebe ate 20 bytes do cliente remoto
    	if (recv(clientSockfd, buf, 20, 0) == -1) {
      		perror("recv");
      		return 5;
    	}

    	// Imprime o valor recebido no servidor antes de reenviar
    	// para o cliente de volta
    	ss << buf << std::endl;
    	std::cout << buf << std::endl;

    	// envia de volta o buffer recebido como um echo
    	if (send(clientSockfd, buf, 20, 0) == -1) {
      		perror("send");
      		return 6;
    	}

    	// o conteudo do buffer convertido para string pode 
    	// ser comparado com palavras-chave
    	if (ss.str() == "close\n")
      		break;

    	// zera a string para receber a proxima
    	ss.str("");
  	}

  	// fecha o socket
  	close(clientSockfd);
	
	return 0;
}


