#ifndef CLIENT_COMMON_H_
#define CLIENT_COMMON_H_

enum class Media_Source_Type{
  SOURCE_NULL = -1,
  SOURCE_FILE = 1,
  SOURCE_HW,
};
  enum SERVER_TYPE{
  	SERVER_NULL,
  	SERVER_SRS,
	//OFFER_OFFER,
  };
	
typedef  enum SERVER_TYPE Server_Type;
#if defined(WEBRTC_LINUX)
#include <unistd.h>
#define MSLEEP(x) usleep(x*1000)
#elif defined(WEBRTC_WIN)
#include <windows.h>
#define MSLEEP(x) Sleep(x)
#endif

#endif

