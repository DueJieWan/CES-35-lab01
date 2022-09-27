#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>


int getIPv4fromName(std::string parced[], char *ipstr){
	struct addrinfo hints;
    struct addrinfo* res;

	// hints - modo de configurar o socket para o tipo de transporte
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	// IPv4
	hints.ai_socktype = SOCK_STREAM;	// TCP
    

	// funcao de obtencao do endereco via DNS - getaddrinfo
	// funcao preenche o buffer "res" e obtem o codigo de resposta "status"
	int status = 0;
	if ( (status = getaddrinfo(parced[0].c_str(), parced[1].c_str(), &hints, &res)) != 0 ) {
		std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
		return 2;
	}

	std::cout << "IP addressees for " << parced[0] << ": " << std::endl;

	// a estrutura de dados eh generica e portanto precisa de type cast
	struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;

	
	// conversao do endereco IP para string
	char stri[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(res->ai_family, &(ipv4->sin_addr), stri, sizeof(stri));
	strcpy(ipstr, stri);
	std::cout << " " << ipstr << std::endl;

    freeaddrinfo(res);
	return 0;
}


void parceURL(char url[], std::string parced[]){
    //parcing http://hostname:port/objectname
    // first is the hostname
    int i = 0, j = 0, k=0;
    for(i = 1; i < strlen(url); i++ ){
        if (url[i] == '/' && url[j] == '/' ){
            i++;
            break;
        }
        j = i;
    }
    for(k = i; k < strlen(url); k++){
        if(url[k] != ':'){
            parced[0] = parced[0] + url[k];
        }else{
            k++;
            break;
        }
    }

    // followed by port
    for(i = k; i < strlen(url); i++){
        if (url[i] != '/'){
            parced[1] = parced[1] + url[i];
        }else{
            i++;
            break;
        }
    }

    // objectname is the last
    for(k = i; k < strlen(url); k++)
        parced[2] = parced[2] + url[k];
}

int main(int argc, char *argv[]){

    // 3 strings to keep parced input
    // parced[0] keeps hostname
    // parced[1] keeps port number
    // parced[2] keeps object name
    std::string parced[3];
    parceURL(argv[1], parced);
    std::cout << "Hostname:" << parced[0] << " port:" << parced[1] << " object:" << parced[2] << std::endl;

    // cria o socket TCP IP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    char ipstr[INET_ADDRSTRLEN] = {'\0'};
	getIPv4fromName(parced, ipstr);

    // atividade de preenchimento da estrutura de endereço IP
    // e porta de conexão, precisa ser zerado o restante da estrutura
    // struct sockaddr_in {
    //  short            sin_family;   // e.g. AF_INET, AF_INET6
    //  unsigned short   sin_port;     // e.g. htons(3490)
    //  struct in_addr   sin_addr;     // see struct in_addr, below
    //  char             sin_zero[8];  // zero this if you want to
    // };
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(std::stoi(parced[1])); // short, network byte order
    serverAddr.sin_addr.s_addr = inet_addr(ipstr);
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    // conecta com o servidor atraves do socket
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
        perror("connect");
        return 2;
    }

    // a partir do SO, eh possivel obter o endereço IP usando
    // pelo cliente (nos mesmos) usando a funcao getsockname
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1){
        perror("getsockname");
        return 3;
    }

    // em caso de multiplos endereços, isso permite o melhor controle
    // a rotina abaixo, imprime o valor do endereço IP do cliente
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    std::cout << "Set up a connection from: " << ipstr << ":" << ntohs(clientAddr.sin_port) << std::endl;

    // trecho de código para leitura e envio de dados nessa conexao
    // buffer eh o buffer de dados a ser recebido no socket com 20 bytes
    // input eh para a leitura do teclado
    // ss eh para receber o valor de volta
    bool isEnd = false;
    char buf[20] = {0};
    std::string input;
    std::stringstream ss;

    while (!isEnd){
        // zera o buffer
        memset(buf, '\0', sizeof(buf));

        // leitura do teclado
        std::cout << "send: ";
        std::cin >> input;

        // converte a string lida em vetor de bytes
        // com o tamanho do vetor de caracteres
        if (send(sockfd, input.c_str(), input.size(), 0) == -1)
        {
            perror("send");
            return 4;
        }

        // recebe no buffer uma certa quantidade de bytes ate 20
        if (recv(sockfd, buf, 20, 0) == -1)
        {
            perror("recv");
            return 5;
        }

        // coloca o conteudo do buffer na string
        // imprime o buffer na tela
        ss << buf << std::endl;
        std::cout << "echo: ";
        std::cout << buf << std::endl;

        // se a string tiver o valor close, sair do loop de eco
        if (ss.str() == "close\n")
            break;

        // zera a string ss
        ss.str("");
    }

    // fecha o socket
    close(sockfd);
    
    return 0;
}