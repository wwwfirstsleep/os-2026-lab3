# os-2026-lab3
# 处理器调度算法模拟实现与比较

姓名：王旭升　学号：24281204　班级：保密2401

## 文件

- 实验报告.docx：按提供的实验报告模板填写，包含实验步骤、算法比较、真实 IDE 截图、讨论和完整源码清单。
- code/scheduler.cpp：C++17 源码，含四种调度算法、随机进程生成、事件日志、统计和自测。
- code/data/：固定用例、I/O 用例和随机种子 20261108 对应的数据。
- code/Makefile、run.sh、run.bat：编译运行入口。
- results/：实际运行的原始文本输出，包括四种算法的完整随机事件日志。
- screenshots/：VS Code 中的原始截图。报告中的局部图均裁自这些截图。

## 编译运行

在 code 目录打开终端。macOS 或 Linux 可执行 `sh run.sh`，或依次执行：

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic scheduler.cpp -o scheduler
./scheduler --test
./scheduler --file data/basic.txt
./scheduler --file data/io.txt --algorithm FCFS --trace
./scheduler --random --seed 20261108 --count 6 --summary
./scheduler --random --seed 20261108 --algorithm RR --quantum 2 --trace
```

Windows 安装支持 C++17 的 GCC/MinGW-w64 后，在 code 目录运行 run.bat。也可在已有 C++ IDE 中新建控制台项目，加入 scheduler.cpp，启用 C++17，并将工作目录设为 code。本次实际编译测试环境为 macOS 26.6.2、arm64、Apple clang 21.0.0、VS Code。

## 输入与参数

每行依次填写进程名、到达时刻、优先级、CPU 段、I/O 段、CPU 段。运行段必须以 CPU 开头和结束，段长均为正整数。优先级数字越小越先运行。`#` 开头的行为注释。

`--algorithm` 可选 ALL、FCFS、SJF、PRIORITY、RR。`--quantum` 设置正整数时间片，默认 2。`--trace` 输出每单位时间的运行状态、就绪队列、未创建、阻塞和结束列表，以及创建、调度、阻塞、I/O 完成、时间片到期和退出事件。`--summary` 仅保留输入与汇总统计。`--random` 启用随机数据，`--seed` 指定非负整数种子，`--count` 指定 1 至 100 个进程。

FCFS、SJF、PRIORITY 均为非抢占式；SJF 比较下一 CPU 段长度。RR 在同一时刻先接收新到达和 I/O 完成的进程，再让用完时间片的进程入队。同一时刻的外部事件按输入顺序入队。I/O 使用独立计时，不模拟设备排队，调度开销为零。

结果字段：AvgTurn 为平均周转时间，AvgWeight 为平均带权周转时间，AvgWait 为平均就绪等待时间，AvgResp 为平均首次响应时间，CPU_pct 为从时刻 0 到最后完成时刻的 CPU 利用率，Dispatch 为派发次数。带权周转时间的分母为进程全部 CPU 段长度之和。

本包保存源码和测试证据，运行脚本会在本机生成可执行文件。重新运行 `./scheduler --test` 可检查四种算法的手算完成时刻、I/O、空闲、单进程、并列选择、RR 边界、SJF 下一运行段、非法输入及 400 次随机运行的时间守恒。
