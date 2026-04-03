#pragma once

#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <functional>
#include <vector>
#include <unordered_map>

#include "Emit.h"
#include "notify_map.h"
#include "peripheral_winrt.h"
#include "radio_watcher.h"

using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using winrt::Windows::Foundation::AsyncStatus;
using winrt::Windows::Foundation::IAsyncOperation;
using winrt::Windows::Foundation::IInspectable;

class BLEManager {
    // Struct to hold context for operations requiring a retry after pairing
    struct PendingOp
    {
        std::function<void()> retry;
        std::function<void(std::string)> error;
    };

public:
    // clang-format off
    BLEManager(const Napi::Value& receiver, const Napi::Function& callback);
    void Scan(const std::vector<winrt::guid>& serviceUUIDs, bool allowDuplicates);
    void StopScan();
    bool Connect(const std::string& uuid);
    bool Disconnect(const std::string& uuid);
    bool CancelConnect(const std::string& uuid);
    bool UpdateRSSI(const std::string& uuid);
    bool DiscoverServices(const std::string& uuid, const std::vector<winrt::guid>& serviceUUIDs);
    bool DiscoverIncludedServices(const std::string& uuid, const winrt::guid& serviceUuid, const std::vector<winrt::guid>& serviceUUIDs);
    bool DiscoverCharacteristics(const std::string& uuid, const winrt::guid& service, const std::vector<winrt::guid>& characteristicUUIDs);
    bool Read(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid);
    bool Write(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid, const Data& data, bool withoutResponse);
    bool Notify(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid, bool on);
    bool DiscoverDescriptors(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid);
    bool ReadValue(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid, const winrt::guid& descriptorUuid);
    bool WriteValue(const std::string& uuid, const winrt::guid& serviceUuid, const winrt::guid& characteristicUuid, const winrt::guid& descriptorUuid, const Data& data);
    bool ReadHandle(const std::string& uuid, int handle);
    bool WriteHandle(const std::string& uuid, int handle, Data data);
    void AttemptPairing(const std::string& uuid, std::function<void()> retry, std::function<void(std::string)> error);
    // clang-format on

private:
    // clang-format off
    void OnRadio(Radio& radio, const AdapterCapabilities& capabilities);
    void OnScanResult(BluetoothLEAdvertisementWatcher watcher, const BluetoothLEAdvertisementReceivedEventArgs& args);
    void OnScanStopped(BluetoothLEAdvertisementWatcher watcher, const BluetoothLEAdvertisementWatcherStoppedEventArgs& args);
    void OnConnected(IAsyncOperation<BluetoothLEDevice> asyncOp, AsyncStatus status, std::string uuid);
    void OnConnectionStatusChanged(BluetoothLEDevice device, winrt::Windows::Foundation::IInspectable inspectable);
    void OnGattSessionCreated(IAsyncOperation<GattSession> asyncOp, AsyncStatus status, std::string uuid);
    void OnMaxPduSizeChanged(GattSession session, winrt::Windows::Foundation::IInspectable object, std::string uuid);
    void OnServicesDiscovered(IAsyncOperation<GattDeviceServicesResult> asyncOp, AsyncStatus status, std::string uuid, std::vector<winrt::guid> serviceUUIDs);
    void OnIncludedServicesDiscovered(IAsyncOperation<GattDeviceServicesResult> asyncOp, AsyncStatus status, std::string uuid, std::string serviceId, std::vector<winrt::guid> serviceUUIDs);
    void OnCharacteristicsDiscovered(IAsyncOperation<GattCharacteristicsResult> asyncOp, AsyncStatus status, std::string uuid, std::string serviceId, std::vector<winrt::guid> characteristicUUIDs);
    void OnRead(IAsyncOperation<GattReadResult> asyncOp, AsyncStatus status, std::string uuid, winrt::guid serviceUuid, winrt::guid characteristicUuid);
    void OnWrite(IAsyncOperation<GattWriteResult> asyncOp, AsyncStatus status, std::string uuid, winrt::guid serviceUuid, winrt::guid characteristicUuid, Data data, bool withoutResponse);
    void OnNotify(IAsyncOperation<GattWriteResult> asyncOp, AsyncStatus status, GattCharacteristic characteristic, std::string uuid, winrt::guid serviceUuid, winrt::guid characteristicUuid, bool state);
    void OnValueChanged(GattCharacteristic chracteristic, const GattValueChangedEventArgs& args, std::string uuid);
    void OnDescriptorsDiscovered(IAsyncOperation<GattDescriptorsResult> asyncOp, AsyncStatus status, std::string uuid, std::string serviceId, std::string characteristicId);
    void OnReadValue(IAsyncOperation<GattReadResult> asyncOp, AsyncStatus status, std::string uuid, winrt::guid serviceUuid, winrt::guid characteristicUuid, winrt::guid descriptorUuid);
    void OnWriteValue(IAsyncOperation<GattWriteResult> asyncOp, AsyncStatus status, std::string uuid, winrt::guid serviceUuid, winrt::guid characteristicUuid, winrt::guid descriptorUuid, Data data);
    void OnReadHandle(IAsyncOperation<GattReadResult> asyncOp, AsyncStatus status, std::string uuid, int handle);
    void OnWriteHandle(IAsyncOperation<GattWriteResult> asyncOp, AsyncStatus status, std::string uuid, int handle);
    void OnPairingRequested(winrt::Windows::Devices::Enumeration::DeviceInformationCustomPairing sender, winrt::Windows::Devices::Enumeration::DevicePairingRequestedEventArgs args);
    void OnPairingCompleted(IAsyncOperation<winrt::Windows::Devices::Enumeration::DevicePairingResult> asyncOp, AsyncStatus status, std::string uuid);
    // clang-format on

    template <typename T>
    bool CheckPairingNeeded(winrt::Windows::Foundation::IAsyncOperation<T> & asyncOp, AsyncStatus status, const std::string uuid)
    {
        auto result = asyncOp.GetResults();
        if (!result)
        {
            return false;
        }
        return status == AsyncStatus::Completed && CheckPairingNeededGatt(result.Status(), uuid);
    }
    bool CheckPairingNeededGatt(GattCommunicationStatus asyncResult, const std::string uuid);
    
    bool mAllowDuplicates;

    Emit mEmit;
    RadioWatcher mWatcher;
    AdapterState mRadioState;
    BluetoothLEAdvertisementWatcher mAdvertismentWatcher;

    winrt::event_revoker<IBluetoothLEAdvertisementWatcher> mReceivedRevoker;
    winrt::event_revoker<IBluetoothLEAdvertisementWatcher> mStoppedRevoker;

    std::unordered_map<std::string, PeripheralWinrt> mDeviceMap;
    std::vector<winrt::guid> mScanServiceUUIDs;
    std::set<std::string> mAdvertismentMap;
    NotifyMap mNotifyMap;

    // Map of device UUID to list of operations waiting for pairing
    std::unordered_map<std::string, std::vector<PendingOp>> mPendingOps;
};
