/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/client/conductor.h"
#include "examples/peerconnection/client/flag_defs.h"
#include "examples/peerconnection/client/main_wnd.h"
#include "examples/peerconnection/client/peer_connection_client.h"
#include "rtc_base/checks.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "system_wrappers/include/field_trial.h"
#include "test/field_trial.h"
#include <iostream>
#include <fstream>
#include <windows.h>


int PASCAL wWinMain(HINSTANCE instance,
                    HINSTANCE prev_instance,
                    wchar_t* cmd_line,
                    int cmd_show) {
  rtc::WinsockInitializer winsock_init;
  rtc::Win32SocketServer w32_ss;
  rtc::Win32Thread w32_thread(&w32_ss);
#if 0  
  //freopen("debug\\in.txt","r",stdin); //输入重定向，输入数据将从in.txt文件中读取 
  FILE *fp = freopen("d:/webrtc/webrtc.log","w",stdout); //输出重定向，输出数据将保存在out.txt文件中 
  setvbuf( stdout, NULL, _IONBF, 0 );
  setvbuf( fp, NULL, _IONBF, 0 );
#endif
#if 1
  //static std::ofstream g_log("webrtc.log");
  //std::cout.rdbuf(g_log.rdbuf());
  
  // 保存cout流缓冲区指针
  std::streambuf* coutBuf = std::cout.rdbuf();
  std::ofstream of("webrtc.log", std::ios::app);
  // 获取文件out.txt流缓冲区指针
  std::streambuf* fileBuf = of.rdbuf();
  // 设置cout流缓冲区指针为out.txt的流缓冲区指针
  std::cout.rdbuf(fileBuf);
  //std::cout << std::unitbuf;
  //of << std::unitbuf;
  //setvbuf( stdout, NULL, _IONBF, 0 );
  //fileBuf->pubsetbuf(NULL, 0);
#endif
  //std::ofstream of("webrtc.log");
  std::cout << "ppt, in wWinMain, go to SetCurrentThread." << std::endl;
  //of << "ppt, in wWinMain, go to SetCurrentThread 222." << std::endl;
  //fflush(fp);
  of.flush();

  //while(1){Sleep(1000);}
  rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

  rtc::WindowsCommandLineArguments win_args;
  int argc = win_args.argc();
  char** argv = win_args.argv();

  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
  if (FLAG_help) {
    rtc::FlagList::Print(NULL, false);
    return 0;
  }

  webrtc::test::ValidateFieldTrialsStringOrDie(FLAG_force_fieldtrials);
  // InitFieldTrialsFromString stores the char*, so the char array must outlive
  // the application.
  webrtc::field_trial::InitFieldTrialsFromString(FLAG_force_fieldtrials);

  // Abort if the user specifies a port that is outside the allowed
  // range [1, 65535].
  if ((FLAG_port < 1) || (FLAG_port > 65535)) {
    printf("Error: %i is not a valid port.\n", FLAG_port);
    return -1;
  }

  MainWnd wnd(FLAG_server, FLAG_port, FLAG_autoconnect, FLAG_autocall);
  if (!wnd.Create()) {
    RTC_NOTREACHED();
    return -1;
  }

  rtc::InitializeSSL();
  PeerConnectionClient client;
  rtc::scoped_refptr<Conductor> conductor(
      new rtc::RefCountedObject<Conductor>(&client, &wnd));

  // Main loop.
  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
    if (!wnd.PreTranslateMessage(&msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  if (conductor->connection_active() || client.is_connected()) {
    while ((conductor->connection_active() || client.is_connected()) &&
           (gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
      if (!wnd.PreTranslateMessage(&msg)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

 //   of.flush();
 //   of.close();
	// 恢复cout原来的流缓冲区指针
    std::cout.rdbuf(coutBuf);
  //  std::cout << "Write Personal Information over..." << std::endl;

  rtc::CleanupSSL();
  return 0;
}
