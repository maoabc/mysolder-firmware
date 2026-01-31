# 基于Zephyr的便携式烙铁固件

它基于Zephyr RTOS，针对STM32硬件（例如C210配置）。固件负责加热控制、姿态感应和用户界面功能。完整项目（包括硬件和外壳）请参阅主仓库：[maoabc/my-solder](https://github.com/maoabc/my-solder)。

## 项目结构
- **boards/**: 板级定义和配置（例如C210专用）。
- **src/**: 固件主源代码文件。
- 关键文件：
  - `CMakeLists.txt`: 构建配置文件。
  - `prj.conf` & `prj_release.conf`: Zephyr内核和驱动设置（调试 vs. 发布版）。
  - `west.yml`: 依赖清单（用于west工具拉取Zephyr模块）。
  - `c210.yaml`: C210硬件叠加配置。
  - `c210_not_use.ioc`: 未使用的STM32CubeMX配置（仅参考）。

## 特性
- **加热控制**：支持T12/C210加热芯。
- **姿态传感器**：自动检测闲置状态，实现休眠以节省电量和提升安全。
- **调试支持**：通过DFU或ST-Link刷机。

## 如何编译和刷机
这个固件需要Zephyr工具链。编译前，确保安装了Zephyr SDK（包括west工具）。以下是详细步骤，使用本地manifest（west.yml）初始化，以匹配你的项目结构。

### 1. 环境准备
- 安装Python 3和pip。
- 安装west：`pip install west`。
- 安装Zephyr SDK：参考[Zephyr官方文档](https://docs.zephyrproject.org/latest/getting_started/index.html)（下载并设置环境变量，如ZEPHYR_TOOLCHAIN_VARIANT）。
- 其他依赖：CMake、Ninja、dtc等（west会自动检查）。

### 2. 设置工作区并克隆项目
1. 创建工作区目录：
``` bash
mkdir workspace
cd workspace
```
2. 克隆本固件仓库到firmware子目录：
`git clone https://github.com/maoabc/mysolder-firmware firmware`
### 3. 初始化west并更新依赖
1. 使用本地manifest初始化west（-l选项指向firmware目录的west.yml）：
``` bash
west init -l firmware
west update #这步会通过网络下载zephyr系统相关，请保证网络没问题，特别很多依赖会被墙。

```

**注意**：初始化后，Zephyr会自动clone到workspace目录下，作为依赖。你的firmware目录是应用层。

### 4. 编译固件
zephyr启动时会禁止stm32g4死电池模式的下拉，等待ucpd初始化时再重新下拉，这个过程容易受到影响从而导致pd握手失败。因此需要先修改zephyr-master/soc/st/stm32/stm32g4x/soc.c文件，把里面LL_PWR_DisableUCPDDeadBattery();注释掉再进行编译，这样就不会有pd兼容问题。
``` bash
cd firmware
west build  -p auto -b t12_g431  -- -DCONF_FILE="prj.conf prj_release.conf"
```

编译成功后，会在build/zephyr下生成二进制文件（zephyr.bin,zephyr.elf，zephyr.hex等等）可以用来刷机。
