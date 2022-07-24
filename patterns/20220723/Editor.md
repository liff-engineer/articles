[TOC]

# 具有复杂交互的应用程序

## 拆分结构

- 界面层 `View`
- 应用服务层 `AppService`
- 服务层 `Service`

## 交互要素拆分

- 输入输出设备 `IOPort` 
- 输入服务 `InputService`
  - 由各种 `Inputer` 构成；
  - `Inputer` 依赖于输入设备 `IOPort` ;
  - `Inputer` 得到输入信息后，发送到对应的消息通道 `Message Channel` ;
  - 输入服务构造并维护 `Inputer` 的生命周期，向外提供一致行为 。
- 输出服务 `OutputService`
  - 由各种 `Outputer` 构成；
  - `Outputer`依赖于输出设备 `IOPort` ;
  - `Outputer` 向外提供特定接口 ，以支持输出特定信息 ；
  - 输出服务构造并维护`Outputer`的生命周期，向外提供一致行为。
- 应用服务 `AppService`

输入输出上下文 `IOContext` , 可以添加输入输出设备 `IOTarget`。

输入输出服务 `IOService` , 



`Ui` 是指  `User Interact` ，用户交互。

- `UiContext` : `UiTarget`
- `UiService` : `Inputer`、`Outputer`
- `AppService`、`Actor`

