// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/discovery/linux_discovery.c */

#include "sk_types.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int sk_path_exists(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0;
}

void sk_linux_discover(void)
{
    DIR *directory;
    struct dirent *entry;

    puts("[SnapKitty Linux AArch64]");

    if (sk_path_exists("/dev/dri"))
        puts("DRM device subsystem available");

    if (sk_path_exists("/dev/kfd"))
        puts("AMD KFD device available");

    if (sk_path_exists("/dev/nvidiactl"))
        puts("NVIDIA driver device available");

    directory = opendir("/sys/class/drm");

    if (directory == NULL)
        return;

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, "renderD", 7U) == 0)
            printf("DRM render node: %s\n", entry->d_name);
    }

    closedir(directory);
}
