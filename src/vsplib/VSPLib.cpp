#include "VSPLib_class.h"
#include "eiface.h"

VspLib::VspLib(void)
{
}

VspLib::~VspLib(void)
{
}

int VspLib::initialize(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    m_pEngine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
    m_pInterfaceFactory = interfaceFactory;
    m_pGameServerFactory = gameServerFactory;
    return 0;
}
