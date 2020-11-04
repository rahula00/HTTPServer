// #include <string.h> //for strcmp
// #include <unistd.h> // read()
// #include <fcntl.h>
// #include <sys/stat.h>
// #include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <err.h>
#include <iostream>

// from Daniel's starter example
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

// for default arguments
using std::string;

void check(int n)
{
    if (n < 0)
    {
        warn("Bad input.");
        exit(1);
    }
}

//https://stackoverflow.com/questions/30695064/checking-if-string-is-only-letters-and-spaces
int checkString(const char s[])
{
    unsigned char c;
    while ((c = *s) && (isalpha(c) || isdigit(c)))
        ++s;

    return *s == '\0';
}

int checkVersion(char s[]) //Should we just check for "HTTP/1.1"
{
    char *a;
    a = strstr(s, "1.1");
    if (a != NULL){
        return 1;
    }
    else{
        return 0;
    }
}

unsigned long getaddr(char *name)
{
    unsigned long res;
    struct addrinfo hints;
    struct addrinfo *info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(name, NULL, &hints, &info) != 0 || info == NULL)
    {
        char msg[] = "getaddrinfo(): address identification error\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }
    res = ((struct sockaddr_in *)info->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(info);
    return res;
}
// We can use default arguments in C++!!
int main(int argc, char **argv)
{

    char *host = argv[1];
    char const *ports;

    // if no port given: default to 8080
    // if port give, use that one
    if(argc == 3){
        ports = argv[2];
    } else if (argc == 1){
        warn("Please specify an address (and port)!\n");   
        return EXIT_FAILURE;
    } else if (argc == 2){
        ports = "80";
    } else if (argc > 3){
        warn("Error! Too many arguments!\n");   
        return EXIT_FAILURE;
    }

    const uint64_t BUFFER_SIZE = 4096;
    char method[4];    // PUT, GET
    char filename[11]; // 10 character file name + "/"
    char httpversion[9];
    // HTTP/1.1
    char buf[BUFFER_SIZE];
    char bufCopy[BUFFER_SIZE];
    int len = 0;
    //char reqType[3];
    //unsigned short port = 8070;
    //char address[] = "localhost";
    struct addrinfo *addressname, hint;
    getaddrinfo(host, ports, &hint, &addressname);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        err(1, "socket()");
    /*
        Configure server socket
    */
    int enable = 1;

    /*
        This allows you to avoid: 'Bind: Address Already in Use' error
    */
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (ret < 0)
    {
        return EXIT_FAILURE;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(ports));
    check(bind(listen_fd, (struct sockaddr *)&servaddr, sizeof servaddr));

    check(listen(listen_fd, 500));

    while (1)
    {
        memset(buf, '\0', sizeof(buf));
        //1. accept connection
        int comm_fd = accept(listen_fd, NULL, NULL); // (socket, store client addr, store client addrlen)
                                                     //2. read http msg
        read(comm_fd, buf, BUFFER_SIZE);
        strcpy(bufCopy, buf);
        sscanf(buf, "%s /%s %s ", method, filename, httpversion);
        strcpy(buf, bufCopy);

        //dprintf(comm_fd, "%s %s %s \n", method, filename, httpversion);
        //check if file name is valid length and contains only letters or numbers
        if( (strlen(filename) != 10) || (!checkString(filename) ) || ((checkVersion(httpversion) == 0))){
            dprintf(comm_fd, "HTTP/1.1 400 Bad Request\r\n");
            close(comm_fd);
        }
        
        //int totalLength = 0;
        //3. process request
        // 200 (OK), 201 (Created), 400 (Bad Request), 403 (Forbidden), 404 (Not Found), and 500 (Internal Server Error). 
		if(!strcmp(method, "GET")){
            //dprintf(comm_fd, "IT's A GET REQUEST\r\n");

            //open file and output to socket
            struct stat fileStats;
            int fd = stat(filename, &fileStats); // get file stats
            if (fd < 0)
            { // if file is invalid
                dprintf(comm_fd, "HTTP/1.1 404 Not Found\r\n");
                close(comm_fd); // invalid file error
            }
            else if (!S_ISREG(fileStats.st_mode))
            { //returns false if not a file
                //printf("THIS IS INVALID AND NOT A FILE");
                dprintf(comm_fd, "HTTP/1.1 403 Forbidden \r\n");
                close(comm_fd);
            }
            else
            { //if not a file
                //printf("THIS IS A FILE");
                int size = fileStats.st_size;

                fd = open(filename, O_RDONLY);
                //fd = open(filename,O_RDONLY);
                if (fd < 0)
                {
                    dprintf(comm_fd, "HTTP/1.1 403 Forbidden\r\n");
                    close(comm_fd);
                    continue;
                }
                dprintf(comm_fd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", size);
                while ((len = read(fd, buf, BUFFER_SIZE)) > 0)
                {                             // read in file contents
                    write(comm_fd, buf, len); //write file content into stdout
                }

                close(fd);
                close(comm_fd); // close file
            }
        }

        
           
      	else if(!strcmp(method, "PUT")){
            if ((strlen(filename) != 10) || (!checkString(filename)) || ((checkVersion(httpversion) == 0)))
            {
                dprintf(comm_fd, "HTTP/1.1 400 Bad Request\r\n");
                close(comm_fd);
            }

            const char find[17] = "Content-Length: ";
            // fflush(stdout);
            // printf("The buffer is: %s\n", buf);
            // fflush(stdout);

            char *contentLengthString = strstr(buf, find);
            // fflush(stdout);
            // printf("The substring is: %s\n", contentLengthString);
            // fflush(stdout);

            ssize_t contentLength;
            sscanf(contentLengthString, "Content-Length: %ld", &contentLength);
            
            /*  fflush(stdout);
             printf("The content length is: %ld\n", contentLength);
             fflush(stdout); */

            //read content length
            int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 00700);

            

            if (fd < 0)
            {
                dprintf(comm_fd, "HTTP/1.1 404 Not Found\r\n");
                close(comm_fd);
                continue;
            }
            else
            {
                ssize_t writeLength;
                while ((len= read(comm_fd, buf, BUFFER_SIZE)) > 0)
                {

                    /* if (contentLength < 0)
                        contentLength = 0;
                    writeLength = (contentLength < len) ? contentLength : len;
                    write(fd, buf, writeLength);
                    contentLength -= writeLength; */

                    writeLength = write(fd, buf, len);
                    if(writeLength == contentLength){
                        break;
                    }
                }

                dprintf(comm_fd, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");

            }
            close(fd);
            close(comm_fd);
            
        
        }
		else{//if requst is not PUT or GET
            dprintf(comm_fd, "HTTP/1.1 500 Internal Server Error\r\n");
            close(comm_fd);
        }
        //4. construct response //use sprintf?
        
        //5. send repsonse
        //dprintf(comm_fd, "HTTP/1.1 200 OK\r\n");
        //dprintf(comm_fd, "Content-Length: %d\r\n\r\n", totalLength);
        //dprintf(fd, "Content-Length: %d\r\n\r\n", size);
      
        close(comm_fd);
    }
    return 0;
}