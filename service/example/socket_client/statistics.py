import re

# 读取statistics.txt文件
with open('statistics.txt', 'r') as file:
    lines = file.readlines()

# 只保留非空行
non_empty_lines = [line for line in lines if line.strip()]

# 将非空行合并为一个字符串
content = ''.join(non_empty_lines)

# 使用正则表达式提取所有的数值
numbers = [int(num) for num in re.findall(r'<(\d+)>', content)]

# 将数值分组，每组8个数值（对应9个指标）
grouped_numbers = [numbers[i:i+9] for i in range(0, len(numbers), 9)]

# 检查分组后的数据是否符合预期
if len(grouped_numbers[-1]) != 9:
    print("数据行数量不是9的整数倍，请检查输入文件。")
else:
    # 计算每个指标的平均值或总数
    averages = [round(sum(col) / len(col), 4) for col in zip(*grouped_numbers)][:6]  # 前5个指标求平均值
    totals = [sum(col) for col in zip(*grouped_numbers)][6:]  # 后3个指标求总数

    # 计算慢包和快包的比例
    slow_packet_ratio = round(totals[1] / totals[0] * 100, 2)  # 慢包数 / 总包数 * 100%
    fast_packet_ratio = round(totals[2] / totals[0] * 100, 2)  # 快包数 / 总包数 * 100%

    # 打印结果
    print(f"平均总TTL: {averages[0]}ms")
    print(f"平均服务器TTL: {averages[1]}ms")
    print(f"平均服务器收包耗时: {averages[2]}ms")
    print(f"平均客户端收包耗时: {averages[3]}ms")
    print(f"平均网络线程传递到逻辑线程开销: {averages[4]}ms")
    print(f"平均逻辑线程传递到网络线程开销: {averages[5]}ms")
    print(f"总包数: {totals[0]}")
    print(f"慢包数: {totals[1]}，占比: {slow_packet_ratio}%")
    print(f"快包数: {totals[2]}，占比: {fast_packet_ratio}%")
