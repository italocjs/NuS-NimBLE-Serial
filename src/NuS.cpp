/**
 * @author Ángel Fernández Pineda. Madrid. Spain.
 * @date 2023-12-18
 * @brief Nordic UART Service implementation on NimBLE stack
 *
 * @note NimBLE-Arduino library is required.
 *       https://github.com/h2zero/NimBLE-Arduino
 *
 * @copyright Creative Commons Attribution 4.0 International (CC BY 4.0)
 *
 */

#include <NimBLEDevice.h>
#include <exception>
#include <vector>
#include <string.h>
#include "NuS.hpp"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

#define NORDIC_UART_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHARACTERISTIC_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

//-----------------------------------------------------------------------------
// Constructor / Initialization
//-----------------------------------------------------------------------------

void NordicUARTService::init()
{
  pServer = NimBLEDevice::createServer();
  if (pServer)
  {
    pServer->setCallbacks(this);
    pServer->getAdvertising()->addServiceUUID(NORDIC_UART_SERVICE_UUID);
    pNuS = pServer->createService(NORDIC_UART_SERVICE_UUID);
    if (pNuS)
    {
      pTxCharacteristic = pNuS->createCharacteristic(TX_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);
      if (pTxCharacteristic)
      {
        NimBLECharacteristic *pRxCharacteristic = pNuS->createCharacteristic(RX_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
        if (pRxCharacteristic)
        {
          pRxCharacteristic->setCallbacks(this);
          connected = false;
          return;
        }
      }
    }
  }
  // Unable to initialize server
  throw std::runtime_error("Unable to create BLE server and/or Nordic UART Service");
}

void NordicUARTService::start(void)
{
  if (!started)
  {
    init();
    pNuS->start();
    started = true;
    pServer->startAdvertising();
  }
}

//-----------------------------------------------------------------------------
// Terminate connection
//-----------------------------------------------------------------------------

void NordicUARTService::disconnect(void)
{
  std::vector<uint16_t> devices = pServer->getPeerDevices();
  for (uint16_t id : devices)
    pServer->disconnect(id);
}

//-----------------------------------------------------------------------------
// GATT server events
//-----------------------------------------------------------------------------

void NordicUARTService::onConnect(NimBLEServer *pServer)
{
  connected = true;
};

void NordicUARTService::onDisconnect(NimBLEServer *pServer)
{
  connected = false;
  pServer->startAdvertising();
};

//-----------------------------------------------------------------------------
// Data transmission
//-----------------------------------------------------------------------------

size_t NordicUARTService::write(const uint8_t *data, size_t size)
{
  if (connected)
  {
    pTxCharacteristic->setValue(data, size);
    pTxCharacteristic->notify();
    return size;
  }
  return 0;
}

size_t NordicUARTService::send(const char *str, bool includeNullTerminatingChar)
{
  if (connected)
  {
    size_t size = includeNullTerminatingChar ? strlen(str) + 1 : strlen(str);
    pTxCharacteristic->setValue((uint8_t *)str, size);
    pTxCharacteristic->notify();
    return size;
  }
  return 0;
}