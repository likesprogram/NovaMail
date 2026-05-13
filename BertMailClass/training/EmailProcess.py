import email
from bs4 import BeautifulSoup
import re

def extractTextFromEml(path: str) -> str:
    """
    从 .eml 文件提取干净的邮件文本（主题 + 正文）
    自动处理 text/plain / text/html，去除签名和回复历史
    """
    try:
        with open(path, "rb") as f:
            msg = email.message_from_binary_file(f)

        # --- 主题 ---
        subject = msg.get("Subject", "")
        try:
            # 一些邮件主题可能被 Base64 编码，如 =?utf-8?b?...?=
            from email.header import decode_header, make_header
            subject = str(make_header(decode_header(subject)))
        except Exception:
            pass

        # --- 正文 ---
        body_parts = []
        for part in msg.walk():
            content_type = part.get_content_type()
            content_disposition = str(part.get("Content-Disposition", "")).lower()
            if "attachment" in content_disposition:
                continue

            payload = part.get_payload(decode=True)
            if not payload:
                continue

            charset = part.get_content_charset() or "utf-8"

            try:
                decoded = payload.decode(charset, errors="ignore")
            except Exception:
                decoded = payload.decode("utf-8", errors="ignore")

            if content_type == "text/html":
                soup = BeautifulSoup(decoded, "html.parser")
                for tag in soup(["script", "style"]):
                    tag.decompose()
                text = soup.get_text(separator="\n")
            else:
                text = decoded

            body_parts.append(text)

        body = "\n".join(body_parts)

        # --- 清理文本 ---
        text = subject + "\n" + body
        text = cleanEmailText(text)
        return text.strip()

    except Exception as e:
        print(f"[WARN] 解析失败 {path}: {e}")
        return ""


def cleanEmailText(text: str) -> str:
    """
    清理邮件正文：
      - 删除签名、历史转发
      - 删除重复空行
      - 删除无意义块
    """
    # 删除常见签名块
    text = re.sub(r"(--|——|Best regards|此致|敬礼|发件人[:：]).*", "", text, flags=re.IGNORECASE)

    # 删除“原始邮件”、“Forwarded message”等历史引用
    text = re.split(r"(原始邮件|Forwarded message|From: .*@.*)", text)[0]

    # 删除多余的空行
    text = re.sub(r"\n{2,}", "\n", text)

    # 删除多余空格
    text = re.sub(r"[ \t]+", " ", text)

    # 删除多余标点（偶尔出现乱码块）
    text = re.sub(r"[=]{2,}", "", text)

    return text.strip()
