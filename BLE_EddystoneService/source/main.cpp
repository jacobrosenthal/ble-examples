/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed-drivers/mbed.h"
#include "ble/BLE.h"
#include "EddystoneService.h"
#include "LEDService.h"

#include "PersistentStorageHelper/ConfigParamsPersistence.h"

EddystoneService *eddyServicePtr;
LEDService *ledServicePtr;

/* Default UID frame data */
static const UIDNamespaceID_t uidNamespaceID = {0x3b, 0xe4, 0x01, 0xaa, 0x7c, 0x68, 0x9e, 0x99, 0x90, 0x85};
static const UIDInstanceID_t  uidInstanceID  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Values for ADV packets related to firmware levels, calibrated based on measured values at 1m */
static const PowerLevels_t defaultAdvPowerLevels = {-47, -33, -21, -13};
/* Values for radio power levels, provided by manufacturer. */
static const PowerLevels_t radioPowerLevels      = {-30, -16, -4, 4};

DigitalOut actuatedLED(LED1, 1);

/**
 * Callback triggered upon a disconnection event.
 */
static void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *cbParams)
{
    (void) cbParams;
    BLE::Instance().gap().startAdvertising();
}

void onDataWrittenCallback(const GattWriteCallbackParams *params) {
    if ((params->handle == ledServicePtr->getValueHandle()) && (params->len == 1)) {
        actuatedLED = !*(params->data);
    }
}

static void onBleInitError(BLE::InitializationCompleteCallbackContext* initContext)
{
    /* Initialization error handling goes here... */
    (void) initContext;
}

static void initializeEddystoneToDefaults(BLE &ble)
{
    /* Set everything to defaults */
    eddyServicePtr = new EddystoneService(ble, defaultAdvPowerLevels, radioPowerLevels);
    eddyServicePtr->setUIDData(uidNamespaceID, uidInstanceID);
}

static void bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext)
{
    BLE         &ble  = initContext->ble;
    ble_error_t error = initContext->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(initContext);
        return;
    }

    ble.gattServer().onDataWritten(onDataWrittenCallback);

    bool initialValueForLEDCharacteristic = false;
    ledServicePtr = new LEDService(ble, initialValueForLEDCharacteristic);

    ble.gap().onDisconnection(disconnectionCallback);

    EddystoneService::EddystoneParams_t params;
    if (loadEddystoneServiceConfigParams(&params)) {
        eddyServicePtr = new EddystoneService(ble, params, radioPowerLevels);
    } else {
        initializeEddystoneToDefaults(ble);
    }

    eddyServicePtr->startBeaconService();
}

void app_start(int, char *[])
{
    /* Tell standard C library to not allocate large buffers for these streams */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    setbuf(stdin, NULL);

    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
}
