#include "mbed.h"
#include "ble/BLE.h"


/** User interface I/O **/

// instantiate USB Serial
Serial serial(USBTX, USBRX);

// Status LED
DigitalOut statusLed(LED1, 0);

// Timer for blinking the statusLed
Ticker ticker;


/** Bluetooth Peripheral Properties **/

// Broadcast name
const static char BROADCAST_NAME[] = "MyDevice";

// Device Information UUID
static const uint16_t deviceInformationServiceUuid  = 0x180a;

// Battery Level UUID
static const uint16_t batteryLevelServiceUuid  = 0x180f;

// array of all Service UUIDs
static const uint16_t uuid16_list[] = { customServiceUuid };

// Number of bytes in Characteristic
static const uint8_t characteristicLength = 20;

// Device Name Characteristic UUID
static const uint16_t deviceNameCharacteristicUuid = 0x2a00;

// Modul Number Characteristic UUID
static const uint16_t modelNumberCharacteristicUuid = 0x2a00;

// Serial Number Characteristic UUID
static const uint16_t serialNumberCharacteristicUuid = 0x2a00;

// Battery Level Characteristic UUID
static const uint16_t batteryLevelCharacteristicUuid = 0x2a00;

// model and serial numbers
static const char* modelNumber = "1AB2";
static const char* serialNumber = "1234";
int batteryLevel = 100;


/** Functions **/

/**
 * visually signal that program has not crashed
 */
void blinkHeartbeat(void);

/**
 * Callback triggered when the ble initialization process has finished
 *
 * @param[in] params Information about the initialized Peripheral
 */
void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params);

/**
 * Callback handler when a Central has disconnected
 * 
 * @param[i] params Information about the connection
 */
void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params);


/** Build Service and Characteristic Relationships **/

// Create a read/write/notify Characteristic
static uint8_t deviceNameCharacteristicValue[characteristicLength] = BROADCAST_NAME;
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(characteristicValue)> deviceNameCharacteristic(
    deviceNameCharacteristicUuid, 
    characteristicValue);

static uint8_t modelNumberCharacteristicValue[characteristicLength] = modelNumber;
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(characteristicValue)> modelNumberCharacteristic(
    modelNumberCharacteristicUuid, 
    characteristicValue);

static uint8_t serialNumberCharacteristicValue[characteristicLength] = serialNumber;
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(characteristicValue)> serialNumberCharacteristic(
    serialNumberCharacteristicUuid, 
    characteristicValue);
 
// Bind Characteristics to Services
GattCharacteristic *deviceInformationCharacteristics[] = {&deviceNameCharacteristic, &modelNumberCharacteristic, serialNumberCharacteristic};
GattService        deviceInformationService(deviceInformationServiceUuid, deviceInformationCharacteristics, sizeof(deviceInformationCharacteristics) / sizeof(GattCharacteristic *));


static uint8_t batteryLevelCharacteristicValue = batteryLevel;
ReadOnlyGattCharacteristic<uint8_t, sizeof(characteristicValue)> batteryLevelCharacteristic(
    serialNumberCharacteristicUuid, 
    characteristicValue,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

GattCharacteristic *batteryLevelCharateristics[] = {&batteryLevelCharacteristic}
GattService        batteryLevelService(batteryLevelServiceUuid, batteryLevelCharateristics, sizeof(batteryLevelCharateristics) / sizeof(GattCharacteristic *));

/**
 * Main program and loop
 */
int main(void) {
    serial.baud(9600);
    serial.printf("Starting Peripheral\r\n");

    ticker.attach(blinkHeartbeat, 1); // Blink status led every 1 second

    // initialized Bluetooth Radio
    BLE &ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(onBluetoothInitialized);

    
    // wait for Bluetooth Radio to be initialized
    while (ble.hasInitialized()  == false);

    while (1) {
        // save power when possible
        ble.waitForEvent();
    }
}


void blinkHeartbeat(void) {
    statusLed = !statusLed; /* Do blinky on LED1 to indicate system aliveness. */
}


void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params) {
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    // quit if there's a problem
    if (error != BLE_ERROR_NONE) {
        return;
    }

    // Ensure that it is the default instance of BLE 
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    serial.printf("Describing Peripheral...");
    
    // attach Services
    ble.addService(customService);
 
    // process disconnections with a callback
    ble.gap().onDisconnection(onCentralDisconnected);

    // advertising parametirs
    ble.gap().accumulateAdvertisingPayload(
        GapAdvertisingData::BREDR_NOT_SUPPORTED |   // Device is Peripheral only
        GapAdvertisingData::LE_GENERAL_DISCOVERABLE); // always discoverable
    // broadcast name
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BROADCAST_NAME, sizeof(BROADCAST_NAME));
    //  advertise services
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    // allow connections
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // advertise every 1000ms
    ble.gap().setAdvertisingInterval(1000); // 1000ms


    // set the GATT values
    ble.gattServer().write(deviceNameCharacteristic.getValueHandle(), BROADCAST_NAME, characteristicLength);
    ble.gattServer().write(modelNumberCharacteristic.getValueHandle(), modelNumber, characteristicLength);
    ble.gattServer().write(serialNumber.getValueHandle(), serialNumber, characteristicLength);
    ble.gattServer().write(batteryLevelCharateristics.getValueHandle(), batteryLevel, characteristicLength);


    // begin advertising
    ble.gap().startAdvertising();

    serial.printf(" done\r\n");
}


void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params) {
    BLE::Instance().gap().startAdvertising();
    serial.printf("Central disconnected\r\n");
}


