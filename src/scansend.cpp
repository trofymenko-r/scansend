//============================================================================
// Name        : scansend.cpp
// Author      : Ruslan Trofymenko
// Version     :
// Copyright   : 
// Description :
//============================================================================

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <fstream>
#include <string.h>
#include <sys/ioctl.h>
#include <vector>
#include <string>
#include <getopt.h>

#include <App.h>
#include <ustring.h>
#include <UsbSerial.h>

using namespace std;
using namespace sys;

string SerialType = "cp210x";

void PrinDeviceList()
{
	vector<CUsbSerial::SDeviceEntry> ttyList = CUsbSerial::GetDevicesList(SerialType);
	for (auto& Device: ttyList) {
		cout << Device.Device << endl;
	}
}

int main(int argc, char** argv) {
	int fd;
	int bytes;
	unsigned char data[21];
	string ttyDev;
	int send_time = 0;

	const char* short_options = "hld:?";

	const struct option long_options[] = {
		{"help",    no_argument,         NULL,    'h'},
		{"list",    no_argument,         NULL,    'l'},
		{"device",  required_argument,   NULL,    'd'},
	};

	int opt;
	int option_index = -1;

	string ParamStr = string(argv[1]);

	while ((opt=getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (opt) {
			case 'h':
				//PrintUsage(argv[0], DefaultSettings);
				exit(EXIT_SUCCESS);

			case 'l':
				PrinDeviceList();
				exit(EXIT_SUCCESS);

            case 'd':
            	ttyDev = optarg;
            	break;

            case 't':
            	send_time = atoi(optarg);
            	break;

            case 0:

            	break;

            case '?':
            	//PrintUsage(argv[0], DefaultSettings);
            	exit(EXIT_FAILURE);

         	default:
            	break;
		};
		option_index = -1;
	};

	vector<CUsbSerial::SDeviceEntry> ttyList = CUsbSerial::GetDevicesList(SerialType);
	if (ttyList.empty()) {
		cerr << "device not detected" << endl;
		exit(EXIT_FAILURE);
	}

	if (ttyDev.empty()) {
		if (ttyList.size() > 1) {
			cout << "specify tty device" << endl;
			exit(EXIT_FAILURE);
		}
		ttyDev = ttyList[0].Device;
	}

	fd = CUsbSerial::InitPort(ttyDev, B115200);
	if (fd <= 0) {
		cout << ttyDev << " not opened" << endl;
		exit(EXIT_FAILURE);
	}

	size_t found = ParamStr.find("#");
	if (found == std::string::npos || found == 0) {
		cerr << "error data line" << endl;
		exit(EXIT_FAILURE);
	}

	string IdStr = ParamStr.substr(0, found);
	int base;
	if (IsHexValue(IdStr)) {
		base = 16;
	} else if (IsIntValue(IdStr)) {
		base = 0;
	} else {
		cerr << "ID not recognized" << endl;
		exit(EXIT_FAILURE);
	}

	int Id = stoi(IdStr, nullptr, base);
	memset(data, 0x00, 21);
	data[0] = 0xAA;
	data[1] = 0xAA;
	for (int ii = 0; ii < 4; ii++)
		data[2 + ii] = (unsigned char)(Id >> 8*ii);

	string DataStr = ParamStr.substr(found + 1);
	if (DataStr.length() % 2 != 0) {
		cerr << "data error" << endl;
		exit(EXIT_FAILURE);
	}


	for (size_t pos = 0; pos < DataStr.length() / 2; pos++) {
		string Data = DataStr.substr(pos*2, 2);
		if (!IsHexValue(Data)) {
			cerr << Data << " is not hex value" << endl;
			exit(EXIT_FAILURE);
		}

		data[6 + pos] = stoi(Data, nullptr, 16);
	}

	data[14] = DataStr.length() / 2;

	data[15] = 0x66;
	data[16] = 0x00; // extend frame
	data[17] = 0x00; // remote frame

	for (int ii = 0; ii < 16; ii++)
		data[18] += data[2 + ii];

	data[19] = 0x55;
	data[20] = 0x55;

	while (true) {
		bytes = write(fd, data, 21);
		if (bytes != 21) {
			cerr << "error send packet" << endl;
		}
		if (send_time == 0)
			break;

		usleep(send_time*1000);
	}

	close(fd);

	return 0;
}
