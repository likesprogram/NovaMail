import os
import json
import shutil
import torch
import gc
import tempfile
from torch.utils.data import Dataset
from transformers import BertTokenizer, BertForSequenceClassification, Trainer, TrainingArguments
from sklearn.model_selection import train_test_split
from EmailProcess import extractTextFromEml

# ===========================
# 1. 配置
# ===========================
DATA_DIR = "data"
MODEL_BASE_DIR = "models"
PYTORCH_MODEL_DIR = os.path.join(MODEL_BASE_DIR, "pytorch")
ONNX_MODEL_DIR = os.path.join(MODEL_BASE_DIR, "onnx")
ONNX_PATH = os.path.join(ONNX_MODEL_DIR, "bert.onnx")

LABELS = ["normal", "advertisement", "spam"]

os.makedirs(PYTORCH_MODEL_DIR, exist_ok=True)
os.makedirs(ONNX_MODEL_DIR, exist_ok=True)

# ===========================
# 2. 自动检测设备
# ===========================
def get_device():
    if torch.cuda.is_available():
        device = torch.device("cuda")
        print(f"⚡ 检测到 GPU: {torch.cuda.get_device_name(0)}")
    elif torch.backends.mps.is_available():  # 适配 macOS
        device = torch.device("mps")
        print("🍎 检测到 Apple MPS 加速器")
    else:
        device = torch.device("cpu")
        print("🧠 使用 CPU 模式（未检测到 GPU）")
    return device

DEVICE = get_device()

# ===========================
# 3. 数据集
# ===========================
class EmailDataset(Dataset):
    def __init__(self, file_paths, labels, tokenizer, max_len=256):
        self.file_paths = file_paths
        self.labels = labels
        self.tokenizer = tokenizer
        self.max_len = max_len

    def __len__(self):
        return len(self.file_paths)

    def __getitem__(self, idx):
        path = self.file_paths[idx]
        label = self.labels[idx]
        text = extractTextFromEml(path) or "empty"

        encoding = self.tokenizer(
            text,
            truncation=True,
            padding="max_length",
            max_length=self.max_len,
            return_tensors="pt"
        )

        return {
            "input_ids": encoding["input_ids"].squeeze(0),
            "attention_mask": encoding["attention_mask"].squeeze(0),
            "labels": torch.tensor(label, dtype=torch.long)
        }

# ===========================
# 4. 加载文件与标签
# ===========================
def load_email_data():
    file_paths, labels = [], []
    for label_idx, label_name in enumerate(LABELS):
        dir_path = os.path.join(DATA_DIR, label_name)
        if not os.path.exists(dir_path):
            continue
        for file in os.listdir(dir_path):
            if file.endswith(".eml"):
                file_paths.append(os.path.join(dir_path, file))
                labels.append(label_idx)
    return file_paths, labels

# ===========================
# 5. 训练函数
# ===========================
def train():
    print("📥 加载数据...")
    file_paths, labels = load_email_data()
    if not file_paths:
        raise RuntimeError("❌ 未找到任何邮件数据，请检查 data/normal 和 data/advertisement 文件夹。")

    # 模型检测
    model_bin = os.path.join(PYTORCH_MODEL_DIR, "pytorch_model.bin")
    model_safe = os.path.join(PYTORCH_MODEL_DIR, "model.safetensors")
    model_exists = os.path.exists(model_bin) or os.path.exists(model_safe)

    tokenizer_exists = (
        os.path.exists(os.path.join(PYTORCH_MODEL_DIR, "tokenizer.json")) or
        (
            os.path.exists(os.path.join(PYTORCH_MODEL_DIR, "tokenizer_config.json")) and
            os.path.exists(os.path.join(PYTORCH_MODEL_DIR, "vocab.txt"))
        )
    )

    if model_exists and tokenizer_exists:
        print("🔄 检测到已训练模型，加载 PyTorch 模型继续训练")
        tokenizer = BertTokenizer.from_pretrained(PYTORCH_MODEL_DIR)
        model = BertForSequenceClassification.from_pretrained(PYTORCH_MODEL_DIR, num_labels=len(LABELS))
    else:
        print("⬇️ 未检测到已训练模型，加载预训练模型 bert-base-uncased")
        tokenizer = BertTokenizer.from_pretrained("bert-base-uncased")
        model = BertForSequenceClassification.from_pretrained("bert-base-uncased", num_labels=len(LABELS))

    # 移动到 GPU（若可用）
    model.to(DEVICE)

    # ===========================
    # 数据拆分
    # ===========================
    train_files, test_files, train_labels, test_labels = train_test_split(
        file_paths, labels, test_size=0.2, random_state=42
    )

    train_dataset = EmailDataset(train_files, train_labels, tokenizer)
    test_dataset = EmailDataset(test_files, test_labels, tokenizer)

    training_args = TrainingArguments(
        output_dir="./results",
        num_train_epochs=2,
        per_device_train_batch_size=4,
        per_device_eval_batch_size=4,
        warmup_steps=100,
        weight_decay=0.01,
        eval_strategy="epoch",
        logging_dir="./logs",
        save_strategy="no",
        logging_steps=10,
        load_best_model_at_end=False,
        report_to="none",  # 禁用 wandb 等日志
        no_cuda=(DEVICE.type != "cuda"),  # ✅ 自动切换
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=test_dataset
    )

    print("🚀 开始训练...")
    try:
        trainer.train()
    except Exception as e:
        print("❌ 训练失败：", e)
        return model, tokenizer

    # =======================
    # 安全保存模型
    # =======================
    print("💾 保存 PyTorch 模型...")

    model.to("cpu")
    torch.cuda.empty_cache()
    gc.collect()

    try:
        with tempfile.TemporaryDirectory() as tmpdir:
            model.save_pretrained(tmpdir, safe_serialization=False)
            tokenizer.save_pretrained(tmpdir)
            os.makedirs(PYTORCH_MODEL_DIR, exist_ok=True)
            for fname in os.listdir(tmpdir):
                os.replace(os.path.join(tmpdir, fname), os.path.join(PYTORCH_MODEL_DIR, fname))

        label_map = {label: idx for idx, label in enumerate(LABELS)}
        with open(os.path.join(PYTORCH_MODEL_DIR, "label_map.json"), "w", encoding="utf-8") as f:
            json.dump(label_map, f, ensure_ascii=False, indent=2)

        print(f"✅ 模型安全保存完成: {PYTORCH_MODEL_DIR}")

    except Exception as e:
        if "cannot be opened" in str(e):
            print("⚠️ 模型文件被占用，请确认没有其他 Python 进程在使用该目录。")
        raise

    return model, tokenizer

# ===========================
# 6. 导出 ONNX
# ===========================
def export_onnx(model, tokenizer):
    print("🧩 正在导出 ONNX 模型...")
    model.to("cpu")
    model.eval()
    
    # 生成 dummy input，保持在模型同一设备
    dummy_input = tokenizer(
        "This is a sample email content.",
        return_tensors="pt",
        padding="max_length",
        truncation=True,
        max_length=256
    )
    
    torch.onnx.export(
        model,
        (dummy_input["input_ids"], dummy_input["attention_mask"]),
        ONNX_PATH,
        input_names=["input_ids", "attention_mask"],
        output_names=["logits"],
        dynamic_axes={
            "input_ids": {0: "batch_size", 1: "seq_len"},
            "attention_mask": {0: "batch_size", 1: "seq_len"},
            "logits": {0: "batch_size"}
        },
        opset_version=14,
        dynamo=False  # 关闭 FX/dynamo 导出
    )

    for filename in [
        "config.json", "tokenizer.json", "tokenizer_config.json",
        "vocab.txt", "special_tokens_map.json", "label_map.json"
    ]:
        src = os.path.join(PYTORCH_MODEL_DIR, filename)
        dst = os.path.join(ONNX_MODEL_DIR, filename)
        if os.path.exists(src):
            shutil.copy(src, dst)

    print(f"✅ ONNX 模型导出成功: {ONNX_PATH}")

# ===========================
# 主流程
# ===========================
if __name__ == "__main__":
    model, tokenizer = train()
    export_onnx(model, tokenizer)
