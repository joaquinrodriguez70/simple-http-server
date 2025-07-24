#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "filelog.h"

#define PORT 8080
#define BUFFER_SIZE 104857600
#define REGEX_SIZE  2048
#define EXT_SIZE    256 

const char *get_file_extension(const char *file_name) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

const char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(file_ext, "txt") == 0 || strcasecmp(file_ext, "log") == 0) {
        return "text/plain";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "image/png";
    } else {
        return "application/octet-stream";
    }
}

bool case_insensitive_compare(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return false;
        }
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

char *get_file_case_insensitive(const char *file_name) {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return NULL;
    }

    struct dirent *entry;
    char *found_file_name = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (case_insensitive_compare(entry->d_name, file_name)) {
            found_file_name = entry->d_name;
            break;
        }
    }

    closedir(dir);
    return found_file_name;
}

char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);
    size_t decoded_len = 0;

    // decode %2x to hex
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else {
            decoded[decoded_len++] = src[i];
        }
    }

    // add null terminator
    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name, 
                        const char *file_ext, 
                        char *response, 
                        size_t *response_len) {
    // build HTTP header
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    // if file not exist, response is 404 Not Found
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        snprintf(response, BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    // get file size for Content-Length
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    // copy header to response buffer
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    // copy file to response buffer
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, 
                            response + *response_len, 
                            BUFFER_SIZE - *response_len)) > 0) {
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

void build_http_response302(const char *location, 
                        char *response, 
                        size_t *response_len
                        ) {
    // build HTTP header
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE,
             "HTTP/1.1 302 Found\r\n"
             "Location: %s\r\n"
             "\r\n",
             location);

    // copy header to response buffer
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);
    
    free(header);
    return;
}


void execute(char * execArgs[]) {
int fork_rv = fork();
  if (fork_rv == 0)
  {
    fork_rv = fork();
    if (fork_rv == 0)
    {
        // we're in the child
        execvp(execArgs[0],execArgs);

         // if execvp fails
        _exit(1);
    }
    else if (fork_rv == -1)
    {
        // fork fails
        _exit(2);
    }

    _exit(0);
  }
  else if (fork_rv != -1)
  {
    // parent wait for the child (which will exit quickly)
    int status;
    waitpid(fork_rv, &status, 0);
  }
    else if (fork_rv == -1)
    {
    // error could not fork
    }
}

char * stringCleanup(char *buffer, char badchar) {
  char *res=strchr(buffer,badchar);
  if (res!=NULL) {
    *res='\0';
    filelog("modified url %s\n",buffer);

   } else
   {
     filelog("not modified %s\n",buffer);
   }
   return buffer;
}

void redirectYT(int client_fd,char *buffer, char *routeRegex, char *destination,int yt360) {
    regex_t regexURI;
    regmatch_t matchesURI[2];
    int startofURI, endofURI,lenofURI;
    char *bufferURI = (char *)malloc(BUFFER_SIZE * sizeof(char));
    
    // check if request is GET/uri
    if (regcomp(&regexURI, routeRegex, REG_EXTENDED))
      {
        printf("error with regex\n"); 
      }
        
    if (regexec(&regexURI, buffer, 2, matchesURI, 0) == 0) {
      printf("matching  %s\n",routeRegex);
      filelog("matching  %s\n",routeRegex);
      
      //execute yt360 only if flag is set
      if (yt360) {

          startofURI=matchesURI[1].rm_so;
          endofURI=matchesURI[1].rm_eo; 
          lenofURI=endofURI-startofURI+1;

          filelog("start of url %d\n",startofURI); 
          filelog("end of url %d\n",endofURI);

          strncpy(bufferURI,buffer+startofURI,lenofURI);
          bufferURI[lenofURI] = '\0';

          char *decodedURL= url_decode(bufferURI);
     
          filelog("decoded url:%s\n", decodedURL);
          //removing & and ; chars     

          stringCleanup(decodedURL,'&');
          stringCleanup(decodedURL,';');
          char * execArgs[]={"./video.sh",decodedURL,"&",NULL} ;
          
          printf("calling video.sh %s\n",decodedURL);
          filelog ("calling video.sh %s\n",decodedURL);
          execute(execArgs);

      }
      // build HTTP response
      char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
      size_t response_len;

      build_http_response302(destination,response ,&response_len);
      // send HTTP response to client
      send(client_fd, response, response_len, 0);

      free(response);

    } else
    {
      filelog("no matching uri for %s \n",routeRegex); 
    }

    free(bufferURI);
    regfree(&regexURI);


}

void get(int client_fd,char *buffer,char *regexGET) {

  regex_t regex;
  regcomp(&regex, regexGET, REG_EXTENDED);
  regmatch_t matches[2];

  if (regexec(&regex, buffer, 2, matches, 0) == 0) {
      // extract filename from request and decode URL
      buffer[matches[1].rm_eo] = '\0';
      const char *url_encoded_file_name = buffer + matches[1].rm_so;
      char *file_name = url_decode(url_encoded_file_name);
      printf("GET %s\n",file_name);
      filelog ("GET %s\n",file_name);
      
      // get file extension
      char file_ext[EXT_SIZE];
      strncpy(file_ext, get_file_extension(file_name),EXT_SIZE);

      // build HTTP response
      char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
      size_t response_len;

      build_http_response(file_name, file_ext, response, &response_len);

      // send HTTP response to client
      send(client_fd, response, response_len, 0);

      free(response);
      free(file_name);
    }
    regfree(&regex);
}


void *handle_client(void *arg) {
    int client_fd   = *((int *)arg);
    char *buffer    = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *regexURI  = (char *)malloc(REGEX_SIZE  * sizeof(char));
    char *regexGET  = (char *)malloc(REGEX_SIZE  * sizeof(char));


    // receive request data from client and store into buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    startfilelog(1,"server.log");

    //printf("%s\n",buffer);
    filelog("%s\n",buffer);
    if (bytes_received > 0) {
        //check for a specific route using GET 
        printf("Checking for a GET /uri redirect \n");
        sprintf(regexURI,"^GET /uri/\\?uri=([^ ]*) HTTP/1");
        redirectYT(client_fd, buffer, regexURI,"/uri.html",1);

        printf("Checking for a GET / redirect \n");
        sprintf(regexURI,"^GET / HTTP/1");
        redirectYT(client_fd, buffer, regexURI,"/uri.html",0);

        // check if request is GET
        printf("Checking for a GET\n");
        sprintf(regexGET, "^GET /([^ ]*) HTTP/1");
        
        get(client_fd,buffer,regexGET);
    }
    closefilelog();
    close(client_fd);
    free(arg);
    free(buffer);

    return NULL;
}

int main(int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in server_addr;


    // create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // config socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // bind socket to port
    if (bind(server_fd, 
            (struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    while (1) {
        // client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));

        // accept client connection
        if ((*client_fd = accept(server_fd, 
                                (struct sockaddr *)&client_addr, 
                                &client_addr_len)) < 0) {
            perror("accept failed");
            continue;
        }

        // create a new thread to handle client request
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}