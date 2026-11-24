#pragma once

#include "interface.h"

class IVEngineServer;

class VspLib
{
private:
    IVEngineServer * m_pEngine;
    CreateInterfaceFn m_pInterfaceFactory;
    CreateInterfaceFn m_pGameServerFactory;
public:
    VspLib(void);
    ~VspLib(void);
    int initialize(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
};
