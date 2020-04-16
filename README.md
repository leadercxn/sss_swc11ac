# sss_swc11ac概述

## 该part负责ble、lora的通信
1. 终端传感器通过lora的形式和swc11ac进行交互
2. 遥控器通过ble广播数据的形式，把按键键值发到swc11ac
3. swc11ac 以serialization的串口形式，与stm32进行交互。实际上，stm32以命令的实行查询上swc11ac



