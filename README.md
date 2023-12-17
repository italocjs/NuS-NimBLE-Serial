# Nordic UART Service (NuS) and BLE serial communications (NimBLE stack)

Library for serial communications through Bluetooth Low Energy on ESP32-Arduino boards

In summary, this library provides:

- A generic class to implement custom serial communications through BLE.
- A BLE serial communications object that can be used as Arduino's [Serial](https://www.arduino.cc/reference/en/language/functions/communication/serial/).
- A more efficient BLE serial communications object that can handle incoming data in packets, eluding active wait thanks to blocking semantics.

## Supported DevKit boards

Any DevKit supported by [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) and based on the [Arduino core for Espressif's boards](https://github.com/espressif/arduino-esp32)
(since [FreeRTOS](https://www.freertos.org/Embedded-RTOS-Binary-Semaphores.html) is required, except for custom protocols).

## Introduction

Serial communications are already available through the old [Bluetooth classic](https://www.argenox.com/library/bluetooth-classic/introduction-to-bluetooth-classic/) specification (see [this tutorial](https://circuitdigest.com/microcontroller-projects/using-classic-bluetooth-in-esp32-and-toogle-an-led)), [Serial Port Profile (SPP)](https://www.bluetooth.com/specifications/specs/serial-port-profile-1-2/).
However, this is not the case with the [Bluetooth Low Energy (BLE) specification](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy).
**No standard** protocol was defined for serial communications in BLE (see [this article](https://punchthrough.com/serial-over-ble/) for further information).

As bluetooth classic is being dropped in favor of BLE, an alternative is needed. [Nordic UART Service (NuS)](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/bluetooth_services/services/nus.html) is a popular alternative, if not the *de facto* standard.
This library implements the Nordic UART service on the *NimBLE-Arduino* stack.

## BLE serial communications apps

You may need a generic terminal (PC or smartphone) application in order to communicate with your Android application. These are several free alternatives (known to me):

- Android:
  - [nRF connect for mobile](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp)
  - [Serial bluetooth terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal)
- iOS:
  - [nRF connect for mobile](https://apps.apple.com/es/app/nrf-connect-for-mobile/id1054362403)

## API

The **basic rules** are:

- You must initialize the *NimBLE stack* **before** using this library. See [NimBLEDevice::init()](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_device.html).
- You must also call `start()` **after** all code initialization is complete.
- Just one object can use the Nordic UART Service. For example, this code **fails** at run time:

  ```c++
  void setup() {
    ...
    NuSerial.start();
    NuStream.start(); // raises an exception (runtime_error)
  }
  ```

Read code commentaries for more information. You may learn from the provided [examples](./examples/).

## Non-blocking serial communications

```c++
#include "NuSerial.hpp"
```

In short, use the `NuSerial` object as you do with the Arduino's `Serial` object. For example:

```c++
void setup()
{
    ...
    NimBLEDevice::init("My device");
    ...
    NuSerial.begin(115200); // Note: parameter is ignored
}

void loop()
{
    if (NuSerial.available())
    {
        // read incoming data and do something
        ...
    } else {
        // other background processing
        ...
    }
}
```

And take into account:

- As you should know, inherited `Stream` methods (for example, `NuSerial.read()`) will immediately return if there is no data available. But, this is also the case when no peer device is connected. Use `NuSerial.isConnected()` to know the case.
- `NuSerial.begin()` or `NuSerial.start()` must be called at least once before reading. Calling more than once have no effect.
- `NuSerial.end()` (as well as `NuSerial.disconnect()`) will terminate any peer connection. It's not required to call `NuSerial.begin()` (nor `NuSerial.start()`) again after `NuSerial.end()` (or `NuSerial.disconnect()`).

## Blocking serial communications

```c++
#include "NuStream.hpp"
```

Use the `NuStream` object (not to be confused with Arduino's streams), based on blocking semantics. The advantages are:

- Efficiency in terms of CPU usage, since no active waiting is used.
- Performance, since incoming bytes are processed in packets, not one bye one.
- Simplicity. Only two methods are strictly needed: `read()` and `write()`. You don't need to worry about data being available or not.

As a disadvantage, multi-tasking app design must be adopted.

For example:

```c++
void setup()
{
    ...
    NimBLEDevice::init("My device");
    ... // other initialization
    NuStream.start(); // don't forget this!!
}

void loop()
{
    size_t size;
    const uint8_t *data = NuStream.read(size);
    while (data)
    {
        // do something with data and size
        ...
        data = NuStream.read(size);
    }
    // No peer connection at this point
}
```

Take into account that:

- **Just one** OS task can work with `NuStream` (others will get blocked).
- Data should be processed as soon as possible. Use other tasks and buffers/queues for time-consuming computation.
  While data is being processed, the peer will stay blocked, unable to send another packet.

### Custom serial communications protocol

```c++
#include "NuS.hpp"

class MyCustomSerialProtocol: public NordicUARTService {
    public:
        void onWrite(NimBLECharacteristic *pCharacteristic) override;
    ...
}
```

Derive a new class and override `onWrite(NimBLECharacteristic *pCharacteristic)` (see [NimBLECharacteristicCallbacks::onWrite](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_characteristic_callbacks.html)). Then, use `pCharacteristic` to read incoming data. For example:

```c++
void MyCustomSerialProtocol::onWrite(NimBLECharacteristic *pCharacteristic)
{
    // Retrieve a pointer to received data and its size
    NimBLEAttValue val = pCharacteristic->getValue();
    const uint8_t *receivedData = val.data();
    suze_t receivedDataSize = val.size();

    // Custom processing here
    ...
}
```

Since just one object can use the Noridic UART Service, you should also implement a
[singleton pattern](https://www.geeksforgeeks.org/implementation-of-singleton-class-in-cpp/).
