# ROS 2 Middleware Implementation for GreenstoneSoft's Swift DDS

`rmw_swiftdds_cpp` is a [ROS 2](https://docs.ros.org/en/rolling) middleware implementation, providing an interface between ROS 2 and [GreenstoneSoft's](https://www.greenstonesoft.com/en_homepage) [Swift DDS](https://github.com/greenstonesoft/greenstone-dds) middleware.

## Getting started

This implementation is available in all ROS 2 distributions, both from binaries and from sources.
You can specify Swift DDS as your ROS 2 middleware layer in two different ways:

1. Exporting `RMW_IMPLEMENTATION` environment variable:
    ```bash
    export RMW_IMPLEMENTATION=rmw_swiftdds_cpp
    ```
1. When launching your ROS 2 application:
    ```bash
    RMW_IMPLEMENTATION=rmw_swiftdds_cpp ros2 run <your_package> <your application>
    ```

## Install SwiftDDS from deb

`rmw_swiftdds_cpp` depends on the installation of [SwiftDDS](https://github.com/greenstonesoft/greenstone-dds), so SwiftDDS must be installed.

1. Download the SwiftDDS installation package from the package folder in the repository(e.g. greenstone-swift-dds_3.0.5_amd64.deb).
1. Run the installation command:
    ```bash
    sudo dpkg -i /path/to/your/deb/greenstone-swift-dds_3.0.5_amd64.deb
    ```

## Install rmw_swiftdds_cpp from source code

1. Clone `rmw_swiftdds_cpp` in the ROS 2 workspace source directory(e.g. ros2_ws).
    ```bash
    cd ros2_ws/src
    git clone xxxxxxx
    ```
1. Install necessary packages for `rmw_swiftdds_cpp`.
    ```bash
    cd ..
    rosdep update
    rosdep install --from src -i
    ```
1.  Run colcon build.
    ```bash
    colcon build --symlink-install [--packages-select rmw_swiftdds_cpp]
    source ./install/setup.bash
    ```

## Advance usage

ROS 2 only allows for the configuration of certain middleware features.
For example, see [ROS 2 QoS policies](https://docs.ros.org/en/rolling/Concepts/About-Quality-of-Service-Settings.html#qos-policies).
In addition to ROS 2 QoS policies, `rmw_swiftdds_cpp` sets the following Swift DDS configurable parameters:

* Publication mode: `ASYNC`
* Data Sharing: `ON`

### Enable Zero Copy Data Sharing

ROS 2 provides [Loaned Messages](https://design.ros2.org/articles/zero_copy.html) that allow the user application to loan the messages memory from the RMW implementation to eliminate the data copy between the ROS 2 application and the RMW implementation.
Furthermore, `rmw_swiftdds_cpp`, through Swift DDS, provides both a [Shared Memory Transport]() and [Local Sender]() to speed up the intra-host communication.

By default, `rmw_swiftdds_cpp` uses [Shared Memory Transport]() for intra-host communication, along with network based transports (UDPv4) for inter-host message delivery.

In order to achieve a Zero Copy message delivery, applications need to both enable Swift DDS Shared Memory Transport mechanism, and use the [Loaned Messages](https://design.ros2.org/articles/zero_copy.html) API.

### About Swift DDS config

Swift DDS allows further configuration of the communication environment through a JSON file.Exporting `RMW_SWIFTDDS_CONFIG` environment variable to your configuration file:
```bash
export RMW_SWIFTDDS_CONFIG=<your_configuration>
```

A JSON configuration file looks like the following (example):

    ```json
    {
        "ip_address": ["192.168.1.100"],
        "shared_memory": true,
        "shared_memory_buffer_size": 20000000,
        "WLP": false,
        "send_mode": "async"
    }
    ```

* ip_address : This is a list of IP addresses. If this item is not configured, messages will be sent from all valid IP addresses. If configured, messages will be sent from the valid IP addresses listed here. Note that an excessive number of IP addresses may degrade performance.
* shared_memory : Whether to enable shared memory. If not configured, the default is true. Note that enabling shared memory may affect your Wireshark packet capture.
* shared_memory_buffer_size : The size of the shared memory pool. The unit is Byte.
* WLP : If set to true, QoS liveliness is enabled. If not configured, the default is false.
* send_mode : If true, the publisher sends synchronously (executed in the publish thread). If false, the message is cached in the queue and awaits processing by internal DDS threads. The default is false.

## Quality Declaration files

Quality Declarations for each package in this repository:

* [`rmw_swiftdds_cpp`](rmw_swiftdds_cpp/QUALITY_DECLARATION.md)
