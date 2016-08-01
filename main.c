/*
 * FsGhost - Simple file change monitoring tool
 *
 * Copyright (c) 2016 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int watch(const char *dir) {

    HANDLE hdir = CreateFile(
            dir,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            0,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            0);

    if (hdir == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        printf("Error - CreateFile %ld\n", err);
        return 1;
    }



    const int buffer_size = 4096;
    void *buffer = malloc(buffer_size);

    if (!buffer) {
        printf("Error - Couldn't allocate %d bytes.\n", buffer_size);
        return 1;
    }


    while (1) {
        DWORD rsize;
        int rd = ReadDirectoryChangesW(
                hdir,
                buffer,
                buffer_size,
                0,
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_SECURITY,
                &rsize,
                0,
                0);

        if (!rd) {
            printf("Error - ReadDirectoryChangesW %d\n", rd);
            return 1;
        }


        FILE_NOTIFY_INFORMATION *p = buffer;

        while (rsize > 0) {

            DWORD action = p->Action;
            DWORD len = p->FileNameLength;
            WCHAR *fname = p->FileName;

            switch (action) {
                case FILE_ACTION_ADDED: printf("added "); break;
                case FILE_ACTION_REMOVED: printf("removed "); break;
                case FILE_ACTION_MODIFIED: printf("modified "); break;
                case FILE_ACTION_RENAMED_OLD_NAME: printf("removed "); break;
                case FILE_ACTION_RENAMED_NEW_NAME: printf("added "); break;
                default: printf("error "); break;
            }

            printf("%.*S\n", (int)(len/2), fname);

            if (p->NextEntryOffset) {
                p = (void*)p + p->NextEntryOffset;
            } else {
                break;
            }
        }
    }


    return 1;
}

#elif __linux__

#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>


int watch(const char *dir) {

    int fd = inotify_init();
    if (fd < 0) {
        printf("Error - inotify_init failed.\n");
        return 1;
    }

    const int buffer_size = 4096;
    char *buffer = malloc(buffer_size);

    if (!buffer) {
        printf("Error - Couldn't allocate %d bytes.\n", buffer_size);
        return 1;
    }

    int wd = inotify_add_watch(fd, dir, IN_ALL_EVENTS);
    if (wd < 0) {
        printf("Error - inotify_add_watch failed.\n");
        return 1;
    }

    while (1) {
        int len = read(fd, buffer, buffer_size);

        if (len < 0) {
            printf("Error - read failed.\n");
            return 1;
        }


        char *p = buffer;
        while(p < buffer + len) {
            struct inotify_event *evt = (struct inotify_event *)buffer;
            if (evt->len) {
                if (evt->mask & (IN_CREATE | IN_MOVED_TO)) {
                    printf("added %s\n", evt->name);
                } else if (evt->mask & (IN_DELETE | IN_MOVED_FROM)) {
                    printf("removed %s\n", evt->name);
                } else if (evt->mask & IN_MODIFY) {
                    printf("modified %s\n", evt->name);
                }
            }

            p += (sizeof(struct inotify_event) + evt->len);
        }
    }


    return 1;
}


#elif __APPLE__
/* TODO */
#error("APPLE IS UNSUPPORTED PLATFORM")
#else
#error("UNSUPPORTED PLATFORM")
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: fsghost directory\n");
        return 0;
    }


    return watch(argv[1]);
}
