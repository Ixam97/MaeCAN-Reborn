/*
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Maximilian Goldschmidt wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* 
* ----------------------------------------------------------------------------
* MäCAN-Updater
* ----------------------------------------------------------------------------
*
* This updater was made for the MäCAN-Bootloader only: https://github.com/Ixam97/MaeCAN-Bootloader/
*
* Last changed: [2021-09-14.1]
*/

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <getopt.h>
#include <iostream>
#include <fstream>

#define VERSION "1.0"
#define PAGE_SIZE 256

using namespace std;

extern char *optarg;

uint8_t complete_buffer[0xfffff];
uint32_t complete_buffer_index;

char IP[17] = "192.168.0.5";
int PORT = 15731;

uint32_t can_uid = 0;

uint8_t page_buffer[PAGE_SIZE];
uint8_t page_buffer_index = 0;
uint16_t page_count = 0;
uint8_t page_index = 0;

uint8_t remote_type = 0;
uint16_t remote_pagesize = 0;
uint16_t remote_pagecount = 0;

ifstream hexfile;

int checkForArguments(char *_optarg);
int printUsage();
int startWinsock(void);
int recvTimeOutTCP(SOCKET _s, long _sec, long _usec);
int sendCanFrame(SOCKET _s, uint8_t cmd, uint8_t dlc, uint8_t _data[8]);
int sendCanFrame(SOCKET _s, uint8_t cmd, uint8_t dlc, uint8_t _data[8], uint16_t _hash);
void sendPage(SOCKET _s, uint8_t page);

int main(int argc, char *argv[])
{
    printf("maecan-updater version %s\n", VERSION);
    char filepath[256];
    int opt;

    while ((opt = getopt(argc, argv, "f:t:i:p:?h")) != -1){
        switch (opt) {
        case '?':
        case 'h':
            return printUsage();
            break;
        case 'f':
            // File path
            if (checkForArguments(optarg)) return printUsage();
            strncpy(filepath, optarg, sizeof(filepath) - 1);
            break;
        case 't':
            // File path
            if (checkForArguments(optarg)) return printUsage();
            can_uid = stoul(optarg, NULL, 16);
            break;
        case 'i':
            // Remote IP address
            if (checkForArguments(optarg)) return printUsage();
            strncpy(IP, optarg, sizeof(IP) - 1);
            break;
        case 'p':
            // Remote port
            if (checkForArguments(optarg)) return printUsage();
            PORT = stoul(optarg, NULL, 10);
            break;
        default:
            return printUsage();
            break;
        }
    }

    if (argc <= 1) return printUsage();
    if (can_uid == 0) return printUsage();

    hexfile.open(filepath);

    if (!hexfile.is_open()) {
        printf("ERROR: Invalid file or path!\n");
        return 1;
    }
    char ch, s_byte_count[3], s_data_address[5], s_line_type[3], s_data_byte[3], s_extended_segment_address[5], s_address_offset[5], s_checksum[3];
    uint8_t byte_count, line_type, data_byte;
    uint16_t data_address, extended_segment_address;
    uint32_t address_offset = 0;

    //memset(complete_buffer, 0xff, 0xfffff);
    //memset(page_buffer, 0xff, PAGE_SIZE);
    memset(s_byte_count, 0, 3);
    memset(s_data_address, 0, 5);
    memset(s_address_offset, 0, 5);
    memset(s_extended_segment_address, 0, 5);
    memset(s_line_type, 0, 3);
    memset(s_data_byte, 0, 3);
    memset(s_checksum, 0, 3);

    uint16_t line_counter = 0;

    while (hexfile.get(ch)) {
        if (ch == ':') {
            line_counter++;
            // New line
            uint32_t checksum = 0;

            hexfile.get(s_byte_count[0]);
            hexfile.get(s_byte_count[1]);
            byte_count = stoul(s_byte_count, NULL, 16);
            //printf("Byte Count: %2d, ", byte_count);
            checksum += byte_count;

            hexfile.get(s_data_address[0]);
            hexfile.get(s_data_address[1]);
            hexfile.get(s_data_address[2]);
            hexfile.get(s_data_address[3]);
            data_address = stoul(s_data_address, NULL, 16);
            //printf("Address: 0x%04x, ", data_address);
            checksum += ((uint8_t)(data_address >> 8) + (uint8_t)data_address);

            hexfile.get(s_line_type[0]);
            hexfile.get(s_line_type[1]);
            line_type = stoul(s_line_type, NULL, 16);
            //printf("Record Type: 0x%02x ", line_type);
            checksum += line_type;

            if (line_type == 0x00) {
                // Program data
                //printf("Data: ");
                for (uint8_t i = 0; i < byte_count; i++) {
                    // Read data bytes to buffer
                    hexfile.get(s_data_byte[0]);
                    hexfile.get(s_data_byte[1]);
                    data_byte = stoul(s_data_byte, NULL, 16);
                    //printf("%02x ", data_byte);
                    checksum += data_byte;

                    complete_buffer[address_offset + data_address + i] = data_byte;
                    complete_buffer_index = address_offset + data_address + i;
                }

            } else if (line_type == 0x02) {
                // Extended Segment Address
                hexfile.get(s_extended_segment_address[0]);
                hexfile.get(s_extended_segment_address[1]);
                hexfile.get(s_extended_segment_address[2]);
                hexfile.get(s_extended_segment_address[3]);
                extended_segment_address = stoul(s_extended_segment_address, NULL, 16);
                checksum += ((uint8_t)(extended_segment_address >> 8) + (uint8_t)extended_segment_address);

                address_offset = extended_segment_address << 4;

            } else if (line_type == 0x04) {
                // Extended Linear Address
                hexfile.get(s_address_offset[0]);
                hexfile.get(s_address_offset[1]);
                hexfile.get(s_address_offset[2]);
                hexfile.get(s_address_offset[3]);
                address_offset = stoul(s_address_offset, NULL, 16);
                checksum += ((uint8_t)(address_offset >> 8) + (uint8_t)address_offset);

            }

            hexfile.get(s_checksum[0]);
            hexfile.get(s_checksum[1]);
            //printf("Checksum: 0x%02x ", (uint8_t)(-checksum));
            if ((uint8_t)(-checksum) == stoul(s_checksum, NULL, 16)) {
                //printf("Correct \n");
            } else {
                printf("\nERROR: Checksum does not match in line %d!\n", line_counter);
                return 1;
            }
        }
    }
    hexfile.close();

    printf("\nHex file parsing successfull.\n");

    // CAN communication:

    long rc;
    SOCKET s;
    SOCKADDR_IN addr;

    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(IP);

    rc = startWinsock();
    if (rc != 0) {
        printf("Error: startWinsock, error code: %ld\n", rc);
        return 1;
    }

    // Create TCP socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Error: Could not create socket, error code: %d\n", WSAGetLastError());
        return 1;
    }

    // Connect TCP socket
    rc = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));
    if (rc == SOCKET_ERROR) {
      printf("Error: Connect failed, error code: %d\n", WSAGetLastError());
      return 1;
    } else {
      printf("Connected to CAN-Interface on IP %s\n", IP);
    }

    // MAIN LOOP

    uint8_t out_data[8] = {(uint8_t)(can_uid >> 24), (uint8_t)(can_uid >> 16), (uint8_t)(can_uid >> 8), (uint8_t)can_uid,0,0,0,0};
    uint8_t updating = 0;
    uint8_t listening = 1;
    uint8_t trys = 0;
    uint8_t retry = 0;

    sendCanFrame(s, 0x80, 4, out_data);

    printf("Looking for Device with UID 0x%08x\n", can_uid);

    while (listening) {
        char recvbuff[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        bool frame_received = false;

        if (recvTimeOutTCP(s, 0, 500000) > 0) {
            if (recv(s, recvbuff, 13, 0)) {
                frame_received = true;

                uint8_t cmd = (uint8_t)(recvbuff[1])>>1;
                //uint8_t resp = (uint8_t) (recvbuff[1] & 0x01);
                uint8_t dlc = (uint8_t) recvbuff[4];
                uint8_t data[8];
                for (uint8_t i = 0; i < dlc; i++) {
                    data[i] = recvbuff[5 + i];
                }
                for (uint8_t i = dlc; i < 8; i++) {
                    data[i] = 0;
                }

                //printf("CMD: %02x, DLC: %d, RESP: %d\n", cmd, dlc, resp);

                /* check for relevant commands from CAN: */
                if (cmd == 0x40 && (dlc == 6 || dlc == 7)) {
                    /* check for UID */
                    if ((data[0] == (uint8_t)(can_uid >> 24)) && (data[1] == (uint8_t)(can_uid >> 16)) && (data[2] == (uint8_t)(can_uid >> 8)) && (data[3] == (uint8_t)can_uid)) {

                        switch (data[4]) {
                            case 0x01 : {
                                // Type:
                                remote_type = data[5];
                                break;
                            }
                            case 0x02 : {
                                // Pagesize:
                                remote_pagesize = (data[5] << 8) + data[6];
                                break;
                            }
                            case 0x03 : {
                                // Pagecount:
                                remote_pagecount = (data[5] << 8) + data[6];
                                break;
                            }
                            case 0x05 : {
                                if (dlc == 7 && data[5] == (page_index - 1)) {
                                    if (data[6] == 0) {
                                        // Transmisson error, new attempt:
                                        printf("\nPage error, new attempt...\n");
                                        sendPage(s, page_index - 1);
                                    } else if (page_index < page_count) {
                                        if (data[6] == 1) {
                                            // Transfer successfull, next page:
                                            sendPage(s, page_index);
                                            page_index++;
                                        }
                                    } else {
                                        // Update complete
                                        out_data[4] = 0x07;
                                        sendCanFrame(s, 0x80, 6, out_data);
                                        printf("\nUpdate completed.\n");
                                        exit(EXIT_SUCCESS);
                                    }

                                }
                            }
                            default : {
                                //break;
                            }
                        }
                    }

                    if (remote_type != 0 && remote_pagecount != 0 && remote_pagesize != 0 && updating == 0){
                        // Check for compatibility:
                        updating = 1;
                        out_data[4] = 0x01;
                        out_data[5] = remote_type;
                        page_count = (complete_buffer_index / remote_pagesize) + 1;
                        if (retry == 0) {
                            printf("Device found, beginning update. \nByte count: %d, Page count: %d\n", complete_buffer_index, page_count);
                        }
                        if (page_count > remote_pagecount) {
                            // Update too large:
                            printf("New update too large, aborting.\n");
                            out_data[6] = 0;
                            sendCanFrame(s, 0x80, 7, out_data);
                            exit(EXIT_FAILURE);

                        } else {

                            if (retry == 0) {
                                page_index = 0;
                                out_data[6] = 1;
                                sendCanFrame(s, 0x81, 7, out_data);
                                sendPage(s, page_index);
                                page_index++;

                            } else {

                                out_data[6] = 1;
                                sendCanFrame(s, 0x81, 7, out_data);
                                sendPage(s, page_index);
                                page_index++;
                            }
                        }
                    }
                }
            }
        }
        if (updating == 0 && trys < 10 && frame_received == false) {
            trys++;
            printf("Try %d ... \n", trys);

            out_data[0] = (can_uid >> 24) & 0xff;
            out_data[1] = (can_uid >> 16) & 0xff;
            out_data[2] = (can_uid >> 8) & 0xff;
            out_data[3] = (can_uid) & 0xff;

            sendCanFrame(s, 0x80, 4, out_data);

        } else if (trys >= 10) {
            printf("Device unreachable!\n");
            listening = 0;

        } else if (updating == 1 && page_index == 1 && frame_received == false) {
            if (retry >= 1) {
                printf("Transmission error! \n");
                trys++;
            }
            page_index = 0;
            updating = 0;
            retry = 1;

            out_data[0] = (can_uid >> 24) & 0xff;
            out_data[1] = (can_uid >> 16) & 0xff;
            out_data[2] = (can_uid >> 8) & 0xff;
            out_data[3] = (can_uid) & 0xff;

            sendCanFrame(s, 0x80, 4, out_data);
        }
    }


    return 0;
}

int checkForArguments(char *_optarg) {

    if (strcmp(_optarg, "-f") == 0 || strcmp(_optarg, "-?") == 0 || strcmp(_optarg, "-h") == 0){
       printf("Missing string to argument! (%s)\n", _optarg);
       return 1;
    }
    return 0;

}

int printUsage() {
    printf("Usage:\n");
    printf("maecan-updater -f <file path>\n");
    printf("  Version: %s\n", VERSION);
    printf("    -f <file path>   Path of the HEX-file to be programmed\n");
    printf("    -t <target UID>  CAN-UID of the device to be updated, HEX\n");
    printf("    -i <remote IP>   IP-address of the CAN-Ethernet-interface (default: 192.168.0.5)\n");
    printf("    -p <remote port> Port of the CAN-Ethernet-interface (default: 15731)\n");
    printf("    -h               Show this help\n");
    return 1;
}

int startWinsock(void) // WinSock starten
{
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2,0),&wsa);
}

int recvTimeOutTCP(SOCKET _s, long _sec, long _usec) {
    struct timeval timeout = {_sec, _usec};
    struct fd_set readSet;

    FD_ZERO(&readSet);
    FD_SET(_s, &readSet);

    return select(0, &readSet, 0, 0, &timeout);

}

int sendCanFrame(SOCKET _s, uint8_t cmd, uint8_t dlc, uint8_t _data[8]) {
    return sendCanFrame(_s, cmd, dlc, _data, 0x0300);
}

int sendCanFrame(SOCKET _s, uint8_t cmd, uint8_t dlc, uint8_t _data[8], uint16_t _hash) {
    char datagram[13] = {0x00, (char)cmd, (char)((uint8_t)(_hash >> 8)), (char)((uint8_t)_hash), (char)dlc, (char)_data[0], (char)_data[1], (char)_data[2], (char)_data[3], (char)_data[4], (char)_data[5], (char)_data[6], (char)_data[7]};
    long _rc = send(_s, datagram, 13, 0);
    //std::this_thread::sleep_for(std::chrono::microseconds(100));
    if (_rc != 13) {
        printf("Error: Unable to send datagram, error code: %d\n", WSAGetLastError());
        return 1;
    }
    return 0;

}

void sendPage(SOCKET _s, uint8_t page) {
    memcpy(page_buffer, &complete_buffer[remote_pagesize * page], remote_pagesize);
    uint8_t begin_frame_data[] = {(uint8_t)(can_uid >> 24), (uint8_t)(can_uid >> 16), (uint8_t)(can_uid >> 8), (uint8_t)can_uid, 0x04, page, 0, 0};
    uint8_t finished_frame_data[] = {(uint8_t)(can_uid >> 24), (uint8_t)(can_uid >> 16), (uint8_t)(can_uid >> 8), (uint8_t)can_uid, 0x05, page, 0, 0};
    uint8_t data_frame[8];
    uint8_t needed_frames = ((remote_pagesize - 1) / 8) + 1;

    sendCanFrame(_s, 0x80, 6, begin_frame_data);

    for (uint8_t i = 0; i < needed_frames; i++) {
        memcpy(data_frame, &page_buffer[i * 8], 8);
        sendCanFrame(_s, 0x80, 8, data_frame, 0x301 + i);
    }

    sendCanFrame(_s, 0x80, 6, finished_frame_data);
    printf("Page %d/%d done.\r\t\t\t\r", page + 1, page_count);

}
