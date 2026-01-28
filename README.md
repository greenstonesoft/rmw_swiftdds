# ROS 2 Middleware Implementation for GreenstoneSoft's Swift DDS

`rmw_swiftdds_cpp` is a [ROS 2](https://docs.ros.org/en/rolling) middleware implementation, providing an interface between ROS 2 and [GreenstoneSoft's](https://www.greenstonesoft.com/en_homepage) [Swift DDS](https://github.com/greenstonesoft/greenstone-dds) middleware.

## Getting started

This implementation is available in all ROS 2 distributions, both from binaries and from sources.
You can specify Swift DDS as your ROS 2 middleware layer in two different ways:

1. Exporting `RMW_IMPLEMENTATION` environment variable:
    ```bash
    export RMW_IMPLEMENTATION=rmw_swiftdds_cpp
    ```
2. When launching your ROS 2 application:
    ```bash
    RMW_IMPLEMENTATION=rmw_swiftdds_cpp ros2 run <your_package> <your application>
    ```

## Install SwiftDDS from apt(Ubuntu)

`rmw_swiftdds_cpp` depends on the installation of [SwiftDDS](https://github.com/greenstonesoft/greenstone-dds), so SwiftDDS must be installed.
1. Install the registry signing key:
    ```bash
    curl -fsSL "https://packages.buildkite.com/greenstonetechnology/swiftdds-deb/gpgkey" | sudo gpg --dearmor -o /etc/apt/keyrings/greenstonetechnology_swiftdds-deb-archive-keyring.gpg
    ```
2. Configure the source:
    ```bash
    echo -e "deb [signed-by=/etc/apt/keyrings/greenstonetechnology_swiftdds-deb-archive-keyring.gpg] https://packages.buildkite.com/greenstonetechnology/swiftdds-deb/any/ any main\ndeb-src [signed-by=/etc/apt/keyrings/greenstonetechnology_swiftdds-deb-archive-keyring.gpg] https://packages.buildkite.com/greenstonetechnology/swiftdds-deb/any/ any main" | sudo tee /etc/apt/sources.list.d/buildkite-greenstonetechnology-swiftdds-deb.list > /dev/null
    ```
3. Run the installation command:
    ```bash
    sudo apt update && sudo apt install greenstone-swift-dds
    ```

## Remove SwiftDDS from apt
1. Run the uninstall command:
    ```bash
    sudo apt remove greenstone-swift-dds
    ```

## Install SwiftDDS from dnf(RHEL)
1. Registry Configuration:
    ```bash
    echo -e "[swiftdds-rpm]\nname=swiftdds_rpm\nbaseurl=https://packages.buildkite.com/greenstonetechnology/swiftdds-rpm/rpm_any/rpm_any/\$basearch\nenabled=1\nrepo_gpgcheck=1\ngpgcheck=0\ngpgkey=https://packages.buildkite.com/greenstonetechnology/swiftdds-rpm/gpgkey\npriority=1" | sudo tee /etc/yum.repos.d/swiftdds-rpm.repo > /dev/null
    ```
2. Run the installation command:
    ```bash
    sudo dnf update && sudo dnf install GreenStone-Swift-DDS
    ```

## Remove SwiftDDS from dnf
1. Run the uninstall command:
    ```bash
    sudo dnf remove GreenStone-Swift-DDS
    ```

## Install SwiftDDS from pixi(Windows 10/11)
1. Install pixi
    ```PowerShell
    irm https://pixi.sh/install.ps1 | iex
    pixi --version
    ```
2. Install SwiftDDS
    Add the “greenstone” parameter to the channels field in pixi.toml.
    ```toml
    [workspace]
    name = "pixi_ros2_rolling"
    version = "0.1.0"
    description = "Dependencies to build ROS 2 on Windows"
    authors = ["Chris Lalancette <clalancette@gmail.com>"]
    channels = ["conda-forge", "greenstone"]
    platforms = ["win-64"]
    ```

    ```PowerShell
    pixi add greenstone-swift-dds
    ```

## Remove SwiftDDS from pixi
1. Run the uninstall command
   ```PowerShell
   pixi remove greenstone-swift-dds
   ```


## Install rmw_swiftdds_cpp from source code

1. Clone `rmw_swiftdds_cpp` in the ROS 2 workspace source directory(e.g. ros2_ws).
    ```bash
    cd ~/ros2_ws/src
    git clone https://github.com/greenstonesoft/rmw_swiftdds
    cd rmw_swiftdds
    git checkout -b rolling origin/rolling
    ```
2. Install necessary packages for `rmw_swiftdds_cpp`.
    ```bash
    cd ~/ros2_ws
    rosdep update
    rosdep install --from src -i
    ```
3.  Run colcon build.
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
        "send_mode": "async",
        "enable_zero_copy": false,
        "zero_copy_shm_size": 100000000
    }
    ```

* ip_address : This is a list of IP addresses. If this item is not configured, messages will be sent from all valid IP addresses. If configured, messages will be sent from the valid IP addresses listed here. Note that an excessive number of IP addresses may degrade performance.
* shared_memory : Whether to enable shared memory. If not configured, the default is true. Note that enabling shared memory may affect your Wireshark packet capture.
* shared_memory_buffer_size : The size of the shared memory pool. The unit is Byte.
* WLP : If set to true, QoS liveliness is enabled. If not configured, the default is false.
* send_mode : If true, the publisher sends synchronously (executed in the publish thread). If false, the message is cached in the queue and awaits processing by internal DDS threads. The default is false.
* enable_zero_copy : On the premise that the message type supports loan message, If set to true, the publisher borrow_loaned_message will apply for memory from the shared memory. The defaule is false.
* zero_copy_shm_size : The size of the zero copy shared memory. The unit is Byte.

## Quality Declaration files

Quality Declarations for each package in this repository:

* [`rmw_swiftdds_cpp`](rmw_swiftdds_cpp/QUALITY_DECLARATION.md)
