// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "device_win.h"

#include <QtCore/QPair>

#include <nx/utils/log/log_main.h>

#include "descriptors.h"

namespace nx::vms::client::desktop::joystick {

DeviceWindows::DeviceWindows(
    LPDIRECTINPUTDEVICE8 inputDevice,
    const JoystickDescriptor& modelInfo,
    const QString& path,
    QTimer* pollTimer,
    QObject* parent)
    :
    base_type(modelInfo, path, pollTimer, parent),
    m_inputDevice(inputDevice)
{
    if (!NX_ASSERT(m_inputDevice))
        return;

    updateStickAxisLimits(modelInfo);

    HRESULT status = m_inputDevice->EnumObjects(
        (LPDIENUMDEVICEOBJECTSCALLBACK)&DeviceWindows::enumObjectsCallback,
        this,
        DIDFT_ALL);

    if (status != DI_OK)
        NX_WARNING(this, "Set callback on EnumObjects failed");
}

DeviceWindows::~DeviceWindows()
{
    if (m_inputDevice)
        m_inputDevice->Release();
}

bool DeviceWindows::isValid() const
{
    return (bool)m_inputDevice;
}

bool DeviceWindows::isInitialized() const
{
    return m_xAxisInitialized && m_yAxisInitialized && m_zAxisInitialized;;
}

Device::State DeviceWindows::getNewState()
{
    if (!isValid() || !isInitialized())
        return {};

    DIJOYSTATE2 state;
    HRESULT status = m_inputDevice->GetDeviceState(
         sizeof(state),
         reinterpret_cast<LPVOID>(&state));

    if (status != DI_OK)
    {
        NX_WARNING(this, "Failed to get device state");
        return {};
    }

    StickPositions newStickPositions{
        mapAxisState(state.lX, m_axisLimits[xIndex]),
        mapAxisState(state.lY, m_axisLimits[yIndex]),
        mapAxisState(state.lZ, m_axisLimits[zIndex])
    };

    std::vector<bool> newButtonStates = m_buttonStates;
    for (int i = 0; i < newButtonStates.size() && i < sizeof(state.rgbButtons); ++i)
        newButtonStates[i] = state.rgbButtons[i];

    return {newStickPositions, newButtonStates};
}

bool DeviceWindows::enumObjectsCallback(LPCDIDEVICEOBJECTINSTANCE deviceObject, LPVOID devicePtr)
{
    auto device = reinterpret_cast<DeviceWindows*>(devicePtr);

    if (!device)
        return DIENUM_STOP;

    if ((deviceObject->dwType & DIDFT_ABSAXIS) != 0)
    {
        DIPROPRANGE range;
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwHow = DIPH_BYID;
        range.diph.dwObj = deviceObject->dwType;
        if (device->m_inputDevice->GetProperty(DIPROP_RANGE, &range.diph) == DI_OK)
        {
            if (deviceObject->guidType == GUID_XAxis)
            {
                formAxisLimits(range.lMin, range.lMax, &device->m_axisLimits[xIndex]);
                device->m_xAxisInitialized = true;
            }
            else if (deviceObject->guidType == GUID_YAxis)
            {
                formAxisLimits(range.lMin, range.lMax, &device->m_axisLimits[yIndex]);
                device->m_yAxisInitialized = true;
            }
            else if (deviceObject->guidType == GUID_ZAxis)
            {
                formAxisLimits(range.lMin, range.lMax, &device->m_axisLimits[zIndex]);
                device->m_zAxisInitialized = true;
            }
        }
    }

    return DIENUM_CONTINUE;
}

void DeviceWindows::formAxisLimits(int min, int max, Device::AxisLimits* limits)
{
    limits->min = min;
    limits->max = max;
    limits->mid = (max - min) / 2;

    const double minBounce = (max - min) / 100;
    if (minBounce > limits->bounce)
        limits->bounce = minBounce;
}

} // namespace nx::vms::client::desktop::joystick
