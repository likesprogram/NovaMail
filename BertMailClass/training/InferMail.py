import os
import json
import torch
import numpy as np
import onnxruntime as ort
from transformers import BertTokenizer
from EmailProcess import extractTextFromEml

# 配置
MODEL_DIR = "./models/onnx"
ONNX_PATH = os.path.join(MODEL_DIR, "bert.onnx")
LABEL_MAP_PATH = os.path.join(MODEL_DIR, "label_map.json")
TOKENIZER_PATH = MODEL_DIR

# 加载模型
print("正在加载模型与分词器...")
tokenizer = BertTokenizer.from_pretrained(TOKENIZER_PATH)

# 使用 ONNX Runtime 加载模型
session = ort.InferenceSession(
    ONNX_PATH, providers=["CPUExecutionProvider"]
)

# 加载标签映射
with open(LABEL_MAP_PATH, "r", encoding="utf-8") as f:
    label_map = json.load(f)

id_to_label = {v: k for k, v in label_map.items()}
print(f"模型加载完成，标签: {id_to_label}")

# 推理函数
def infer_eml(path: str):
    """
    对单个 .eml 文件进行推理分类
    """
    # 1. 提取邮件正文
    text = extractTextFromEml(path)
    if not text.strip():
        print(f"[WARN] 文件 {path} 无有效正文内容")
        return None

    # 2. 分词
    inputs = tokenizer(text, return_tensors="np",  # 返回 NumPy 格式
        padding="max_length", truncation=True, max_length=256
    )

    # 3. ONNX 推理
    ort_inputs = {
        "input_ids": inputs["input_ids"],
        "attention_mask": inputs["attention_mask"]
    }
    ort_outputs = session.run(None, ort_inputs)
    logits = ort_outputs[0]

    # 4. 计算概率 & 预测标签
    probs = torch.softmax(torch.tensor(logits), dim=-1).numpy()[0]
    pred_id = int(np.argmax(probs))
    pred_label = id_to_label[pred_id]

    print(f"文件: {path}")
    print(f"预测类别: {pred_label}")
    print(f"概率分布: {probs}")
    return pred_label


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("用法: python InferMail.py <eml文件路径>")
        exit(1)

    eml_path = sys.argv[1]
    if not os.path.exists(eml_path):
        print(f"文件不存在: {eml_path}")
        exit(1)

    infer_eml(eml_path)
