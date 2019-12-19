/** 
 * \class Projector
 */
#define _CRT_SECURE_NO_WARNINGS 
#include "Projector.h"
#include <io.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <Winsock2.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
//#include <netinet/tcp.h>
#include <regex>
//#include <sys/socket.h>
#include <time.h>
//#include <unistd.h>

#include "UI.h"
#pragma comment(lib, "Winmm.lib")

using namespace std;

Projector *Projector::_instance = NULL;

Projector *Projector::getInstance() {
    if (_instance == NULL) {
        _instance = new Projector();
    }
    return _instance;
}

Projector::Projector() {
    _emulateHangOpenBug = false;
    _PJLinkUseAuthentication = false;
    
    _PJLinkPower = POWER_ON;
    _PJLinkInput = 11;
    _PJLinkAVMute = 30;
    _PJLinkError = 0;
    _PJLinkLampHours = 0;
    
    _PJLinkName = "Emulator";
    
    _port = 4352;
    
    _socketListener = NULL;
    _socketTimer = NULL;
    
    _isListening = false;
    _isConnected = false;
    _serverfd = -1;
    _clientfd = -1;
    
    _clientConnected = 0;
    _clientTimeout = 0;
    
    bzero(&_serveraddr, sizeof(sockaddr_in));
    bzero(&_clientaddr, sizeof(sockaddr_in));
    
    _ui = UI::getInstance();
}

Projector::~Projector() {
    if (_isListening == true) this->close();
}

Projector::Projector(const Projector &orig) {
}

const Projector &Projector::operator=(const Projector &orig) {
	return *this;
}

int Projector::getPort() {
    return _port;
}

void Projector::setPort(int port) {
    if (port > 0) _port = port;
}

int Projector::getPowerState() {
    return _PJLinkPower;
}

void Projector::setPowerState(int value) {
    _PJLinkPower = value;
}

int Projector::getInputState() {
    return _PJLinkInput;
}

void Projector::setInputState(int value) {
    _PJLinkInput = value;
}

int Projector::getAVMuteState() {
    return _PJLinkAVMute;
}

void Projector::setAVMuteState(int value) {
    _PJLinkAVMute = value;
}

int Projector::getErrorState() {
    return _PJLinkError;
}

void Projector::setErrorState(int value) {
    _PJLinkError = value;
}

int Projector::getLampHours() {
    return _PJLinkLampHours;
}

void Projector::setLampHours(int value) {
    _PJLinkLampHours = value;
}

bool Projector::isListening() {
    return _isListening;
}

bool Projector::isConnected() {
    return _isConnected;
}

void Projector::close() {
    _isConnected = false;
    _isListening = false;
    
    ::shutdown(_clientfd, SHUT_RDWR);
    ::shutdown(_serverfd, SHUT_RDWR);
    
    // TODO: Null checks.
   /* fclose(_sin);
    fclose(_sout);*/
    
    _socketListener->join();
    delete _socketListener;
}

void Projector::closeClient() {
    _isConnected = false;
    
    ::shutdown(_clientfd, SHUT_RDWR);
    
    fclose(_sin);
    fclose(_sout);
}

void Projector::listen() {
    listen(_port);
}

void Projector::listen(int port) {
    
	
	if (_isListening == true) return;
    
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		_ui->print("WSAStartup failed with error: "+ std::to_string(err));
		
	}


    _port = port;
    
    _serveraddr.sin_family = AF_INET;
    _serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _serveraddr.sin_port = htons(_port);

	
    _serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverfd < 0) {
		int sockerr =  WSAGetLastError();
        _ui->print("Err: "+ std::to_string(sockerr));
        return;
    }
    
    if (bind(_serverfd, (sockaddr *) &_serveraddr, sizeof(_serveraddr)) < 0) {
        _ui->print("Failed to bind socket.");
        return;
    }
    
    if (::listen(_serverfd, SOMAXCONN) < 0) {
        _ui->print("Failed to listen on socket.");
        return;
    }
    
    _isListening = true;
    
    _socketListener = new thread(&Projector::doAccept, this);
}

void Projector::doAccept() {
    while (_isListening == true) {
        this->accept();
    }
}

void Projector::accept() {
    if (_isConnected == true) return;
    
    bzero(&_clientaddr, sizeof(_clientaddr));
    
    // TODO: Implement poll and yield thread.
    
    // Only accept one connection.
	_clientlen = sizeof(sockaddr_in);
    _clientfd = ::accept(_serverfd, (sockaddr *) &_clientaddr, &_clientlen);
    if (_clientfd < 0) {
        if (_isListening == true) _ui->print("Failed to accept client.: "+ std::to_string(WSAGetLastError())); // If _isListening is false, user closed the socket listener.
        _ui->refresh();
        return;
    }
    else {
        _clientConnected = time(NULL);
        
        // Convert to human time.
        struct tm *connectedTime = localtime(&_clientConnected);
        
        string hour = to_string(connectedTime->tm_hour);
        if (hour.length() == 1) hour = "0" + hour;
        
        string minute = to_string(connectedTime->tm_min);
        if (minute.length() == 1) minute = "0" + minute;
        
        string second = to_string(connectedTime->tm_sec);
        if (second.length() == 1) second = "0" + second;
        
        _ui->print("Client connected. " + hour + ":" + minute + ":" + second);
        
        // Configure client socket to blocking.
       /* int sflags = fcntl(_clientfd, F_GETFL, 0);
        fcntl(_clientfd, F_SETFL, sflags & ~O_NONBLOCK);*/
		int iResult;
		u_long iMode = 0;
		iResult = ioctlsocket(_clientfd, FIONBIO, &iMode);


        // Set TCP_NODELAY
        int tcpVal = 1;
        setsockopt(_clientfd, IPPROTO_TCP, TCP_NODELAY, (char*)&tcpVal, sizeof (int));
        


       /* _sin = _fdopen(_clientfd, "r");
        _sout = _fdopen(_clientfd, "w");*/

		
        
        // Set the fd buffers to line buffered (default is fully buffered).
    /*   setvbuf(_sin, NULL, _IONBF, -1);
        setvbuf(_sout, NULL, _IOLBF, -1);*/
        
        _isConnected = true;
        
        resetSocketTimeout();
        
        // Acknowledge client connection.
        string greeting = "PJLINK 0\r\n";
        
        if (_PJLinkUseAuthentication == true) {
            // TODO: Implement PJLink connection challenge.
            //       PJLink spec p23.
        }
		DWORD bytesWritten;
		send(_clientfd, greeting.c_str(), greeting.length(), 0);
		//WriteFile((HANDLE)_clientfd, greeting.c_str(), greeting.length(), &bytesWritten, NULL);
        /*fprintf(_sout, "%s\r\n", greeting.c_str());
        fflush(_sout);*/
        
        _ui->print("> " + greeting);
        
        while (_isConnected == true) {
            doRead();
        }
    }
}

void Projector::doRead() {
    if (_isConnected == false) return;
//    if (_sin == NULL) return;
    
    // Read command.
    char rbuf[1024]; // Read buffer.
    bzero (rbuf, sizeof(rbuf));
	DWORD bytesWritten;
    // TODO: Implement poll and yield thread.
    
	int readResult = recv(_clientfd, rbuf, sizeof(rbuf), 0);

    //ssize_t readResult = _read(_clientfd, rbuf, sizeof(rbuf));
    if (readResult<=-1) {
        // Read error.
        _isConnected = false;
        _ui->print("Read error.");
        _ui->refresh();
        return;
    }
    else if (readResult == 0) {
        // EOF
        _isConnected = false;
        _ui->refresh();
        return;
    }
    
    resetSocketTimeout();
    
    string received(rbuf);
    
    // Check for command and parameter.
	regex pjlinkCmdRegex("%1(\\w+)\\ (\\?|[0-9]+)\\r?\\n?");
	
	//regex_t regex;
    int result = 0;
    size_t nmatches = 3;
   // regmatch_t pmatch[3];
    
    /*result = regcomp(&regex, "%1(\\w+)\\ (\\?|[0-9]+)\\r?\\n?", REG_EXTENDED);
    if (result != 0) {
        _ui->print("Error compiling regex.");
    }
    else {
        result = regexec(&regex, received.c_str(), nmatches, pmatch, REG_EXTENDED);
    }*/
	smatch pmatch;
	bool found = regex_match(received.cbegin(), received.cend(),pmatch, pjlinkCmdRegex);
	

    string command = (found ) ? pmatch.str(1) : "";
    string value = (found) ? pmatch.str(2) : "";
    
    // DEBUG ////////////////////////////////////////////////////////////////////////////////
    // Change this to the entire received string minus line terminators.
    _ui->print("< " + command + " " + value);
    
    string response = "ERR1"; // Undefined PJLink command.
    
    if (command == "POWR") {
        if (value == "1") {
            _PJLinkPower = POWER_ON;
			mouse_event(0x0001, 0, 1, 0, NULL);
			Sleep(40);
			mouse_event(0x0001, 0, -1, 0, NULL);
            response = "%1POWR=OK";
        }
        else if (value == "0") {
            _PJLinkPower = POWER_OFF;
			PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
            response = "%1POWR=OK";
        }
        // Power query.
        else if (value == "?") {
            response = "%1POWR=" + to_string(_PJLinkPower);
        }
        // Power instruction out of parameter.
        else if (value == "") {
            response = "%1POWR=ERR2";
        }
    }
    
    else if (command == "INPT") {
		response = "%1INPT=ERR2";
        if (value == "?") {
            response = "%1INPT=11";
        }
        else if (value == "") {
            response = "%1INPT=ERR2";
        }
        else {
            int inputValue = stoi(value);
            response = "%1INPT=OK";
            
            if (inputValue == 11) {
                _PJLinkInput = inputValue;
            }
            else {
                // Out of range.
                response = "%1INPT=ERR2";
            }
        }
    }
    
    else if (command == "AVMT") {
        if (value == "?") {
			HRESULT coi = CoInitialize(NULL);
			BOOL isMuted = false;
			if (coi == S_OK || coi == S_FALSE) {
				IMMDeviceEnumerator* deviceEnumerator = NULL;
				HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)& deviceEnumerator);
				if (hr == S_OK)
				{

					IMMDevice* defaultDevice = NULL;

					hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
					if (hr == S_OK) {
						deviceEnumerator->Release();
					}
					deviceEnumerator = NULL;

					IAudioEndpointVolume* endpointVolume = NULL;
					hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)& endpointVolume);
					if (hr == S_OK) {
						defaultDevice->Release();
						defaultDevice = NULL;

						endpointVolume->GetMute(&isMuted);
						endpointVolume->Release();
					}
				}
				CoUninitialize();
			}
			if(isMuted)
				_PJLinkAVMute = (int)AVMUTE_AUDIO;
			else 
				_PJLinkAVMute = (int)AVMUTE_NONE;
			
			
			response = "%1AVMT=" + to_string(_PJLinkAVMute);
        }
        else if (value == "") {
            response = "%1AVMT=ERR2";
        }
        else {
            int avmtValue = stoi(value);
            response = "%1AVMT=OK";
            
            switch (avmtValue) {
                /*case AVMUTE_VIDEO:
                    _PJLinkAVMute = (_PJLinkAVMute == AVMUTE_AUDIO) ?  (int) AVMUTE_BOTH : (int) AVMUTE_VIDEO;

                    break;

                case AVMUTE_UNMUTE_VIDEO:
                    _PJLinkAVMute = (_PJLinkAVMute == AVMUTE_BOTH) ? (int) AVMUTE_AUDIO : (int) AVMUTE_NONE;
                    break;*/
                    
				case AVMUTE_AUDIO: AVMUTE_BOTH: AVMUTE_VIDEO:
				{
					_PJLinkAVMute = (_PJLinkAVMute == AVMUTE_VIDEO) ? (int)AVMUTE_BOTH : (int)AVMUTE_AUDIO;
					HRESULT coi = CoInitialize(NULL);
					if (coi == S_OK || coi == S_FALSE) {
						IMMDeviceEnumerator* deviceEnumerator = NULL;
						HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)& deviceEnumerator);
						if (hr == S_OK)
						{

							IMMDevice* defaultDevice = NULL;

							hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
							if (hr == S_OK) {
								deviceEnumerator->Release();
							}
							deviceEnumerator = NULL;

							IAudioEndpointVolume* endpointVolume = NULL;
							hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)& endpointVolume);
							if (hr == S_OK) {
								defaultDevice->Release();
								defaultDevice = NULL;

								endpointVolume->SetMute(TRUE, NULL);
								endpointVolume->Release();
							}
						}
						CoUninitialize();
					}					
					break;
				}
                    
				case AVMUTE_UNMUTE_AUDIO: AVMUTE_UNMUTE_VIDEO: AVMUTE_UNMUTE_BOTH: {
					_PJLinkAVMute = (_PJLinkAVMute == AVMUTE_BOTH) ? (int)AVMUTE_VIDEO : (int)AVMUTE_NONE;
					HRESULT coi = CoInitialize(NULL);
					if (coi == S_OK || coi == S_FALSE) {
						IMMDeviceEnumerator* deviceEnumerator = NULL;
						HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)& deviceEnumerator);
						if (hr == S_OK)
						{

							IMMDevice* defaultDevice = NULL;

							hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
							if (hr == S_OK) {
								deviceEnumerator->Release();
							}
							deviceEnumerator = NULL;

							IAudioEndpointVolume* endpointVolume = NULL;
							hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)& endpointVolume);
							if (hr == S_OK) {
								defaultDevice->Release();
								defaultDevice = NULL;

								endpointVolume->SetMute(FALSE, NULL);
								endpointVolume->Release();
							}
						}
						CoUninitialize();
					}
					break;
				}
               /* case AVMUTE_BOTH:
                    _PJLinkAVMute = AVMUTE_BOTH;
					/*waveOutGetVolume(
						NULL,
						&this->volumeLevel
					);
					waveOutSetVolume(NULL, 0);
                    break;*/
                    
                /*case AVMUTE_UNMUTE_BOTH:
                    _PJLinkAVMute = AVMUTE_NONE;
					//waveOutSetVolume(NULL, this->volumeLevel);
                    break;
					*/
                default:
                    response = "%1AVMT=ERR2";
                    break;
            }
        }
    }
    
    else if (command == "ERST") {
        if (value == "?") {
            response = "%1ERST=" + string("000000"); // TODO: Implement error status.
        }
    }
    
    else if (command == "LAMP") {
        if (value == "?") {
            string lampOn = (_PJLinkPower == POWER_ON || _PJLinkPower == POWER_WARMING) ? "1" : "0";
            response = "%1LAMP=" + to_string(_PJLinkLampHours) + " " + lampOn;
        }
    }
    
    else if (command == "NAME") {
        if (value == "?") {
			response = "%1NAME=USENDA LTI5518H";
        }
    }
	else if (command == "INFO") {
	if (value == "?") {
		response = "%1INFO=USENDA LTI5518H";
		}
	}
	else if (command == "INF1") {
	if (value == "?") {
		response = "%1INF1=USENDA";
	}
	}
	else if (command == "INF2") {
	if (value == "?") {
		response = "%1INF2=LTI5518H";
	}
	}
	else if (command == "CLSS") {
	if (value == "?") {
		response = "%1CLSS=1";
	}
	}
	else if (command == "INST") {
	if (value == "?") {
		response = "%1INST=11";
	}
	}
	response += "\r\n";
	
   /* fprintf(_sout, "%s\r\n", response.c_str());
    fflush(_sout);*/
    
	//DWORD bytesWritten;
	//WriteFile((HANDLE)_clientfd, response.c_str(), response.length(), &bytesWritten, NULL);
	send(_clientfd, response.c_str(), response.length(), 0);
    _ui->print("> " + response);
    
    _ui->refresh();
}

void Projector::resetSocketTimeout() {
    // Set timeout.
    if (_emulateHangOpenBug == true) {
        _clientTimeout = 0;
    }
    else {
        _clientTimeout = time(NULL) + 30;
    }
}
