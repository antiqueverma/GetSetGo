/**
 * freertos_driver_example.c
 * 
 * Example C implementation of Windows counterpart drivers for FreeRTOS (GCC Port).
 * Connects to the "Get Set Go" MCU Simulator on 127.0.0.1:9000 using standard Winsock.
 * 
 * To compile with GCC MinGW:
 *   gcc freertos_driver_example.c -o freertos_sim.exe -lws2_32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SIMULATOR_HOST "127.0.0.1"
#define SIMULATOR_PORT 9000

static SOCKET s_sock = INVALID_SOCKET;

/* Initialize Winsock & TCP connection to Get Set Go Simulator */
int gsg_simulator_init(void) {
    WSADATA wsa;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[Driver] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }

    s_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock == INVALID_SOCKET) {
        printf("[Driver] Socket creation failed: %d\n", WSAGetLastError());
        return -1;
    }

    server.sin_addr.s_addr = inet_addr(SIMULATOR_HOST);
    server.sin_family = AF_INET;
    server.sin_port = htons(SIMULATOR_PORT);

    if (connect(s_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("[Driver] Connect to simulator failed! Is Get Set Go running on port 9000?\n");
        closesocket(s_sock);
        return -1;
    }

    printf("[Driver] Connected to Get Set Go MCU Simulator!\n");
    return 0;
}

/* Helper to send newline-terminated JSON packet */
void gsg_send_packet(const char *json_str) {
    if (s_sock == INVALID_SOCKET) return;
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s\n", json_str);
    send(s_sock, buffer, strlen(buffer), 0);
}

/* ----------------- Windows Counterpart Drivers ----------------- */

/* 1. GPIO Driver Counterpart */
void GPIO_WritePin(char port, int pin, int state) {
    char pkt[256];
    snprintf(pkt, sizeof(pkt), 
        "{\"module\":\"gpio\",\"action\":\"write\",\"port\":\"%c\",\"pin\":%d,\"state\":%d}",
        port, pin, state ? 1 : 0);
    gsg_send_packet(pkt);
}

/* 2. UART Console Driver Counterpart */
void UART_Transmit(const char *msg) {
    char pkt[512];
    snprintf(pkt, sizeof(pkt),
        "{\"module\":\"console\",\"action\":\"tx\",\"text\":\"%s\"}", msg);
    gsg_send_packet(pkt);
}

/* 3. ADC Driver Counterpart */
void ADC_Update(int channel, float voltage, int raw_12bit) {
    char pkt[256];
    snprintf(pkt, sizeof(pkt),
        "{\"module\":\"adc\",\"action\":\"update\",\"channel\":%d,\"voltage\":%.2f,\"raw\":%d}",
        channel, voltage, raw_12bit);
    gsg_send_packet(pkt);
}

/* 4. LCD 16x2 Driver Counterpart */
void LCD_Print(int line, int col, const char *text) {
    char pkt[256];
    snprintf(pkt, sizeof(pkt),
        "{\"module\":\"display\",\"action\":\"write\",\"line\":%d,\"col\":%d,\"text\":\"%s\"}",
        line, col, text);
    gsg_send_packet(pkt);
}

/* 5. EEPROM Driver Counterpart */
void EEPROM_WriteByte(int address, unsigned char data) {
    char pkt[256];
    snprintf(pkt, sizeof(pkt),
        "{\"module\":\"eeprom\",\"action\":\"write\",\"address\":%d,\"data\":[%d]}",
        address, data);
    gsg_send_packet(pkt);
}

/* 6. Flash Driver Counterpart */
void Flash_SectorErase(int sector) {
    char pkt[256];
    snprintf(pkt, sizeof(pkt),
        "{\"module\":\"flash\",\"action\":\"sector_erase\",\"sector\":%d}",
        sector);
    gsg_send_packet(pkt);
}

/* Demonstration Main */
int main(void) {
    if (gsg_simulator_init() != 0) {
        return 1;
    }

    UART_Transmit("[FreeRTOS-C-Port] Driver initialization started.\\r\\n");
    LCD_Print(0, 0, "FreeRTOS GCC");
    LCD_Print(1, 0, "Drivers Active");

    printf("Simulating FreeRTOS Task loop... Press Ctrl+C to exit.\n");
    int count = 0;
    while (1) {
        count++;
        // Toggle Heartbeat LED
        GPIO_WritePin('A', 5, count % 2);
        
        // Sample ADC
        ADC_Update(0, 3.15f, 3910);

        Sleep(500);
    }

    closesocket(s_sock);
    WSACleanup();
    return 0;
}
