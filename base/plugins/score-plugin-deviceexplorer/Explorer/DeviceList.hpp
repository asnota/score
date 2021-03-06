#pragma once
#include <Device/Protocol/DeviceInterface.hpp>
#include <wobjectdefs.h>
#include <Device/Protocol/DeviceSettings.hpp>
#include <QString>
#include <algorithm>
#include <functional>
#include <score_plugin_deviceexplorer_export.h>
#include <vector>

namespace Device
{

/**
 * @brief The DeviceList class
 *
 * Once a device is added with addDevice, the DeviceList
 * has ownership.
 *
 * This is not the case for the local device, which is owned
 * by LocalTreeDocumentPlugin
 */
class SCORE_PLUGIN_DEVICEEXPLORER_EXPORT DeviceList : public QObject
{
  W_OBJECT(DeviceList)
public:
  DeviceInterface& device(const QString& name) const;
  DeviceInterface& device(const Device::Node& name) const;

  DeviceInterface* findDevice(const QString& name) const;

  void addDevice(DeviceInterface* dev);
  void removeDevice(const QString& name);

  void apply(std::function<void(Device::DeviceInterface&)>);
  void setLogging(bool);

  void setLocalDevice(DeviceInterface*);
  void setAudioDevice(DeviceInterface* dev)
  {
    m_audioDevice = dev;
  }
  DeviceInterface* localDevice() const
  {
    return m_localDevice;
  }
  DeviceInterface* audioDevice() const
  {
    return m_audioDevice;
  }
  const std::vector<DeviceInterface*>& devices() const;
public:
  void logInbound(const QString& arg_1) const W_SIGNAL(logInbound, arg_1);
  void logOutbound(const QString& arg_1) const W_SIGNAL(logOutbound, arg_1);
  void logError(const QString& arg_1) const W_SIGNAL(logError, arg_1);

private:
  std::vector<DeviceInterface*> m_devices;
  DeviceInterface *m_localDevice{}, *m_audioDevice{};
  bool m_logging = false;
};

SCORE_PLUGIN_DEVICEEXPLORER_EXPORT DeviceLogging get_cur_logging(bool b);
}
