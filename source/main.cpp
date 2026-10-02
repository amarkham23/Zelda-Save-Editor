// Copyright (c) 2026 StockiestThree9
#include "ui.hpp"
#include <cstdio>
#ifdef __SWITCH__
#include <switch.h>
#endif
int main(int argc,char** argv){try{return zse::runApp(argc,argv);}catch(const std::exception& e){
#ifdef __SWITCH__
    consoleInit(nullptr);std::printf("Zelda Save Editor could not start.\n\n%s\n\nPress + to return.\n",e.what());
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState pad;padInitializeDefault(&pad);
    while(appletMainLoop()){padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;consoleUpdate(nullptr);}consoleExit(nullptr);
#else
    std::fprintf(stderr,"%s\n",e.what());
#endif
    return 1;
}}
