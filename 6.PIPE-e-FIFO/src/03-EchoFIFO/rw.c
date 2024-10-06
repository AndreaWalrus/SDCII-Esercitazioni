#include <unistd.h>
#include <errno.h>
#include "common.h"


int readOneByOne(int fd, char* buf, char separator) {
    int ret;
    int bytes_read=0;

    /** [TO DO] READ THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can read from a FIFO as from a regular file descriptor
     * - since you don't know the length of the message, just
     *   read one byte at a time from the socket
     * - leave the cycle when 'separator' ('\n') is encountered 
     * - repeat the read() when interrupted before reading any data
     * - return the number of bytes read
     * - reading 0 bytes means that the other process has closed
     *   the FIFO unexpectedly: this is an error that should be
     *   dealt with!
     **/
    while(1){
        ret = read(fd, buf+bytes_read, 1);
        if(ret==0) handle_error("all pipe writers are closed");
        if(ret==-1) continue;
        if(buf[bytes_read]==separator) break;
        bytes_read++;
    }
    fflush(stdout);
    return bytes_read;

}

void writeMsg(int fd, char* buf, int size) {

    int ret;
    /** [TO DO] SEND THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can write on the FIFO as on a regular file descriptor
     * - make sure that all the bytes have been written: use a while
     *   cycle in the implementation as we did for file descriptors!
     **/
    size_t written_bytes=0;
    size_t bytes_left = size;
    while(bytes_left>0){
        ret = write(fd, buf+written_bytes, bytes_left);
        if(ret==-1) continue;
        if(ret>0){bytes_left-=ret; written_bytes+=ret;}
    }
    //printf("Sent %ld bytes\n",written_bytes);
    fflush(stdout);

}
