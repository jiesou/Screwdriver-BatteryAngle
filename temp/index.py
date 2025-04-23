import json
import csv

def parse_record_to_csv(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        csv_writer = csv.writer(outfile)
        # 写入CSV头部
        csv_writer.writerow(['millis', 'frequency'])
        
        for line in infile:
            if line.startswith('data:'):
                try:
                    # 提取JSON部分
                    json_str = line.split('data: ')[1].strip()
                    data = json.loads(json_str)
                    
                    # 提取所需字段
                    millis = data.get('millis', '')
                    frequency = data.get('frequency', '')
                    
                    # 写入CSV行
                    csv_writer.writerow([millis, frequency])
                except (json.JSONDecodeError, IndexError) as e:
                    print(f"Error parsing line: {line.strip()}. Error: {e}")

# 使用示例
input_filename = 'record.txt'
output_filename = 'output.csv'
parse_record_to_csv(input_filename, output_filename)

print(f"CSV文件已生成: {output_filename}")
