#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

int main(int argc, char* argv[]) {
    int ret;

    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    /** TODO: create a socket for contacting the server
     *
     * Suggestions:
     * - protocollo AF_INET
     * - tipo SOCK_DGRAM
     */

    socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_desc==-1) handle_error("socket creation error");

    if (DEBUG) fprintf(stderr, "Socket created...\n");


    /** [SOLUTION]
     *  TODO: set up parameters for the connection and initiate a connection to the server
     *
     * Suggestions:
     * - set the 3 fields of server:
     * - - server_addr.sin_addr.s_addr: we must specify the server address
     * - - server_addr.sin_family,
     * - - server_addr.sin_port (using htons() method)
     */
    //

    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;
    memset(buf,0,buf_len);

    // main loop
    while (1) {
        char* quit_command = SERVER_COMMAND;
        size_t quit_command_len = strlen(quit_command);

        printf("Insert your message: ");

        /* Read a line from stdin
         *
         * fgets() reads up to sizeof(buf)-1 bytes and on success
         * returns the first argument passed to it. */
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);
//        buf[--msg_len] = '\0'; // remove '\n' from the end of the message

		/** TODO: send message to server
         *
         * Suggestions:
         * - sendto() with flags = 0 is equivalent to write() to a descriptor
         * - the number of bytes to send is msg_len as we have discarded '\n'
         * - don't deal with partially sent messages, but deal with other errors
         * - message size IS NOT buf size
         */

        ret = sendto(socket_desc, buf, msg_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
        if(ret==-1) handle_error("send error");
        
        if (DEBUG) fprintf(stderr, "Sent message of %d bytes...\n", ret);


        /* After a quit command we won't receive any more data from
         * the server, thus we must exit the main loop. */

        /** TODO: After a quit command we won't receive any more data from
         *  the server, thus we must exit the main loop.
         *
         * Suggestions:
         * - compare the number of bytes sent with the length of the
         *   quit command that tells the server to close the connection
         *   (see SERVER_COMMAND macro in common.h)
         * - perform a byte-to-byte comparsion when required using
         *   memcmp(const void *ptr1, const void *ptr2, size_t num)
         * - exit from the cycle when there is nothing left to receive
         */

        if(memcmp(buf, quit_command, quit_command_len)==0) break;

        /** TODO: read message from server
         * Suggestions:
         * - recvfrom() with flags = 0 is equivalent to read() from a descriptor
         * - for sockets, we get a 0 return value only when the peer closes
         *   the connection: if there are no bytes to read and we invoke
         *   recvfrom() we will get stuck, because the call is blocking!
         * - don't deal with partially sent messages in UDP, but deal with other errors
         */

        //ret = recvfrom(socket_desc, buf, buf_len, 0, NULL, NULL);
        //if(ret==0) handle_error("connection closed");

        while(1){
            ret = recvfrom(socket_desc, buf, buf_len, 0, NULL, NULL);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
	        if (ret >= 0) break;
	    }

        if (DEBUG) fprintf(stderr, "Received message of %d bytes...\n", ret);
        printf("Server response: %s\n", buf); // no need to insert '\0'
    }


    /** TODO: close socket and release unused resources
     */

    ret = close(socket_desc);
    if(ret!=0) handle_error("close error");
    if (DEBUG) fprintf(stderr, "Exiting...\n");

    exit(EXIT_SUCCESS);
}
