/**
 *****************************************************************************
 **
 **  File        : syscalls.c
 **
 **  Abstract    : Atollic TrueSTUDIO Minimal System calls file
 **
 ** 		          For more information about which c-functions
 **                need which of these lowlevel functions
 **                please consult the Newlib libc-manual
 **
 **  Environment : Atollic TrueSTUDIO
 **
 **  Distribution: The file is distributed “as is,” without any warranty
 **                of any kind.
 **
 **  (c)Copyright Atollic AB.
 **  You may use this file as-is or modify it according to the needs of your
 **  project. Distribution of this file (unmodified or modified) is not
 **  permitted. Atollic AB permit registered Atollic TrueSTUDIO(R) users the
 **  rights to distribute the assembled, compiled & linked contents of this
 **  file as part of an application binary file, provided that it is built
 **  using the Atollic TrueSTUDIO(R) Pro toolchain.
 **
 *****************************************************************************
 */

/* Includes */
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#include "timing.h"
#include "usbd_cdc.h"
#include "usbd_misc.h"

#include "BlueDisplay.h"
#include "LocalGUI/LocalTinyPrint.h"

/* Variables */
#undef errno
extern int errno;
//extern int __io_getchar(void) __attribute__((weak));

register char * stack_ptr asm("sp");

char *__env[1] = { 0 };
char **environ = __env;

/* Functions */
void initialise_monitor_handles(void) {
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    _kill(status, -1);
    while (1) {
    } /* Make sure we hang here */
}

int _read(int file, char *ptr, int len) {
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
//        *ptr++ = __io_getchar();
    }

    return len;
//    if (isUsbCdcReady() && USB_PacketReceived) {
//        int DataIdx;
//        uint8_t * tReceiveBufferPtr = &USB_ReceiveBuffer[0];
//        // read from USB receive buffer
//        for (DataIdx = 0; DataIdx < len && DataIdx < USB_ReceiveLength; DataIdx++) {
//            *ptr++ = *tReceiveBufferPtr++;
//        }
//        return DataIdx;
//    }
    return 0;
}

int _write(int file, char *ptr, int len) {

    if (isUsbCdcReady()) {
        // try to send over USB
        USBD_CDC_SetTxBuffer(&USBDDeviceHandle, (uint8_t*) ptr, len);
        setTimeoutMillis(2);
        bool isTimeout = false;
        while (USBD_CDC_TransmitPacket(&USBDDeviceHandle) == USBD_BUSY) {
            if (isTimeoutSimple()) {
                isTimeout = true;
                break;
            }
        }
        // Fallback
        if (isTimeout) {
            myPrint(ptr, len);
        }
    } else {
        // USB not available
        writeStringC(ptr, len);
    }
    return len;
}

char *heap_end;

extern char estack asm("_estack");
char * sLowestStackPointer = &estack;

caddr_t _sbrk(int incr) {
    extern char HeapStart asm("_sccmram");
    extern char HeapEnd asm("_eheap");
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &HeapStart;
    }

    prev_heap_end = heap_end;
    if (stack_ptr < sLowestStackPointer) {
        sLowestStackPointer = stack_ptr;
    }
    if (heap_end + incr > &HeapEnd) {
        if (isLocalDisplayAvailable) {
            drawTextC(0, TEXT_SIZE_11_ASCEND, "Heap CCRAM 8kB exhausted", TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
            delayMillis(1000);
        }
//		write(1, "Heap and stack collision\n", 25);
//		abort();
        errno = ENOMEM;
        return (caddr_t) -1;
    }

    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _open(char *path, int flags, ...) {
    /* Pretend like we always fail */
    return -1;
}

int _wait(int *status) {
    errno = ECHILD;
    return -1;
}

int _unlink(char *name) {
    errno = ENOENT;
    return -1;
}

int _times(struct tms *buf) {
    return -1;
}

int _stat(char *file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _link(char *old, char *new) {
    errno = EMLINK;
    return -1;
}

int _fork(void) {
    errno = EAGAIN;
    return -1;
}

int _execve(char *name, char **argv, char **env) {
    errno = ENOMEM;
    return -1;
}
