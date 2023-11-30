/*
 *  ramdisk ssh server
 *
 *  Copyright (c) 2015 xerub
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <dirent.h>
#include <net/if.h>
#include <assert.h>
#include <CoreFoundation/CoreFoundation.h>
#include "IOKit/IOKitLib.h"
#include "IOKit/IOTypes.h"
#include "IOUSBDeviceControllerLib.h"
#include "IOKit/IOKitKeys.h"  // Asegúrate de que esta ruta esté correctamente configurada


IOUSBDeviceDescriptionRef IOUSBDeviceDescriptionCreateWithType(CFAllocatorRef, CFStringRef);

char bootargs[2048];
size_t len = sizeof(bootargs) - 1;

io_service_t
get_service(const char *name, unsigned int retry)
{
    io_service_t service;
    CFDictionaryRef match = IOServiceMatching(name);

    while (1) {
        CFRetain(match);
        service = IOServiceGetMatchingService(kIOMasterPortDefault, match);
        if (service || !retry--) {
            break;
        }
        printf("Didn't find %s, trying again\n", name);
        sleep(1);
    }

    CFRelease(match);
    return service;
}

char* getSerialNumberString() {
    static char serialNumber[128];
    FILE* pipe = popen("/usr/bin/SerialNumber", "r"); 

    if (!pipe) return NULL; 

    if (fgets(serialNumber, sizeof(serialNumber), pipe) == NULL) {
        pclose(pipe);
        return NULL; // Retorna NULL si no hay salida del comando
    }

    pclose(pipe);
    char* start = serialNumber; // Inicio de la cadena
    char* end; // Fin de la cadena
    // Mueve el inicio al primer carácter no espacio
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Si toda la cadena era espacios, retorna una cadena vacía
    if (*start == 0) {
        serialNumber[0] = '\0';
        return serialNumber;
    }
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';

    // Si es necesario, desplaza la cadena al principio del buffer
    if (start > serialNumber) {
        memmove(serialNumber, start, (end + 1) - start);
    }

    return serialNumber; // Retorna el buffer con el número de serie sin espacios al principio y al final
}

/* reversed from restored_external */
int
init_usb(void)
{
    int i;
    CFNumberRef n;
    io_service_t service;
    CFMutableDictionaryRef dict;
    IOUSBDeviceDescriptionRef desc;
    IOUSBDeviceControllerRef controller;

    desc = IOUSBDeviceDescriptionCreateWithType(kCFAllocatorDefault, CFSTR("standardMuxOnly")); /* standardRestore */
    if (!desc) {
        return -1;
    }
    //IOUSBDeviceDescriptionSetSerialString(desc, CFSTR("SSHRD_Script " __DATE__ " " __TIME__ ));

    char serialString[256];
    //snprintf(serialString, sizeof(serialString), "SSHRD_Script %s %s %s", getSerialNumberString(), __DATE__, __TIME__);
    snprintf(serialString, sizeof(serialString), "SSHRD_Script %s", getSerialNumberString());
    CFStringRef serialStringRef = CFStringCreateWithCString(kCFAllocatorDefault, serialString, kCFStringEncodingUTF8);
    IOUSBDeviceDescriptionSetSerialString(desc, serialStringRef);

    //liberar serialStringRef después de su uso
    CFRelease(serialStringRef);
    controller = 0;
    while (IOUSBDeviceControllerCreate(kCFAllocatorDefault, &controller)) {
        printf("Unable to get USB device controller\n");
        sleep(3);
    }
    IOUSBDeviceControllerSetDescription(controller, desc);

    CFRelease(desc);
    CFRelease(controller);

    service = get_service("AppleUSBDeviceMux", 10);
    if (!service) {
        return -1;
    }

    dict = CFDictionaryCreateMutable(NULL, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    i = 7;
    n = CFNumberCreate(NULL, kCFNumberIntType, &i);
    CFDictionarySetValue(dict, CFSTR("DebugLevel"), n);
    CFRelease(n);

    i = IORegistryEntrySetCFProperties(service, dict);
    CFRelease(dict);
    IOObjectRelease(service);

    return i;
}


bool getDeviceModel(char* model, size_t size) {
    FILE* pipe = popen("uname -m", "r");  // Comando que podría devolver el modelo del dispositivo
    if (!pipe) return false;

    bool success = fgets(model, size, pipe) != NULL;
    pclose(pipe);
    return success;
}

bool getSystemVersionDetails(char* productName, size_t productNameSize, char* productVersion, size_t productVersionSize, char* buildVersion, size_t buildVersionSize) {
    FILE* pipe = popen("sw_vers", "r");
    if (!pipe) return false;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        if (sscanf(buffer, "ProductName: %255s", productName) == 1) {
            // ProductName encontrado
        } else if (sscanf(buffer, "ProductVersion: %255s", productVersion) == 1) {
            // ProductVersion encontrado
        } else if (sscanf(buffer, "BuildVersion: %255s", buildVersion) == 1) {
            // BuildVersion encontrado
        }
    }

    pclose(pipe);
    return true;
}


#include "micro_inetd.c" /* I know, I am a bad person for doing this */
char *execve_params[] = { "micro_inetd", "22", "/usr/local/bin/dropbear", "-i", NULL };

/* chopped from https://code.google.com/p/iphone-dataprotection/ */
int
main(int argc, char *argv[])
{

    printf("Starting ramdisk tool\n");
    io_service_t service = get_service("IOWatchDogTimer", 0);
    if (service) {
        int zero = 0;
        CFNumberRef n = CFNumberCreate(NULL, kCFNumberSInt32Type, &zero);
        IORegistryEntrySetCFProperties(service, n);
        CFRelease(n);
        IOObjectRelease(service);
    }

    if (init_usb()) {
        printf("USB init FAIL\n");
    } else {
        printf("USB init done\n");
    }

    int i;
    struct stat st;
    for (i = 0; i < 10; i++) {
        printf("Waiting for data partition\n\n\n");
        if(!stat("/dev/disk0s2s1", &st)) {
            break;
        }
        if(!stat("/dev/disk0s1s2", &st)) {
            break;
        }
        if(!stat("/dev/disk1s1", &st)) {
            break;
        }
        if(!stat("/dev/disk1s2", &st)) {
            break;
        }
        sleep(5);
    }
   
    const char* RED = "\x1b[31m";
    const char* GREEN = "\x1b[32m";
    const char* YELLOW = "\x1b[33m";
    const char* BLUE = "\x1b[34m";
    const char* MAGENTA = "\x1b[35m";
    const char* CYAN = "\x1b[36m";
    const char* RESET = "\x1b[0m";
    //const char* PURPLE = "\033[35m";

    printf("%s                 ,xNMM.         \n%s", GREEN, RESET);
    printf("%s               .OMMMMo          \n%s", GREEN, RESET);
    printf("%s               OMMM0,           \n%s", GREEN, RESET);
    printf("%s     .;loddo:' loolloddol;.     \n%s", GREEN, RESET);
    printf("%s   cKMMMMMMMMMMNWMMMMMMMMMM0:   \n%s", GREEN, RESET);
    printf("%s  KMMMMMMMMMMMMMMMMMMMMMMMWd.   \n%s", YELLOW, RESET);
    printf("%s XMMMMMMMMMMMMMMMMMMMMMMMX.     \n%s", YELLOW, RESET);
    printf("%s;MMMMMMMMMMMMMMMMMMMMMMMM:      \n%s", RED, RESET);
    printf("%s:MMMMMMMMMMMMMMMMMMMMMMMM:      \n%s", RED, RESET);
    printf("%s.MMMMMMMMMMMMMMMMMMMMMMMMMX.    \n%s", RED, RESET);
    printf("%s kMMMMMMMMMMMMMMMMMMMMMMMMWd.   \n%s", RED, RESET);
    printf("%s .XMMMMMMMMMMMMMMMMMMMMMMMMMMk  \n%s", MAGENTA, RESET);
    printf("%s .XMMMMMMMMMMMMMMMMMMMMMMMMK.   \n%s", MAGENTA, RESET);
    printf("%s   kMMMMMMMMMMMMMMMMMMMMMMd     \n%s", BLUE, RESET);
    printf("%s     KMMMMMMMWXXWMMMMMMMk.      \n%s", BLUE, RESET);
    printf("%s       .cooc,.    .,coo:.       \n%s", BLUE, RESET);

    printf("---------- ALDAZ OS RAMDISK -------------\n");

    printf("%sCompiled: %s" __DATE__ " " __TIME__ "\n", YELLOW, RESET);

    printf("%sPort:%s 22\n", YELLOW, RESET);

    printf("%sRAMDISK:%s SSHRD_Script\n", YELLOW, RESET);

    char model[256];
    char productName[256];
    char productVersion[256];
    char buildVersion[256];

    if (getDeviceModel(model, sizeof(model))) {

        printf("%sModel: %s%s %s\n", YELLOW, RESET, model, getSerialNumberString());
    } else {
        printf("Device Model: Unable model.\n");
    }

    if (getSystemVersionDetails(productName, sizeof(productName), productVersion, sizeof(productVersion), buildVersion, sizeof(buildVersion))) {
        printf("%sProductName: %s%s", YELLOW, RESET, productName);
        printf("\n%sProductVersion: %s%s", YELLOW, RESET, productVersion);
        printf("\n%sBuildVersion: %s%s", YELLOW, RESET, buildVersion);
    } else {
        printf("Unable to get system version details.\n");
    }

    printf("\n%sRunning server:%s OK", YELLOW, RESET);
    printf("\n\n---------- ALDAZ OS RAMDISK -------------\n");

    return main2(4, execve_params);
}

