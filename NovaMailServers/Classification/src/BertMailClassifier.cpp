#include <BertMailClassifier.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace NovaMail::Classification {
    BertMailClassifier::BertMailClassifier(
        const std::string& modelPath,
        const std::string& vocabPath,
        const std::string& labelMapPath,
        size_t maxLength
    )
        : m_env(ORT_LOGGING_LEVEL_WARNING, "NovaMailBertClassifier")
        , m_maxLength(maxLength) {
        if (modelPath.empty()) {
            throw std::invalid_argument("BertMailClassifier: modelPath is empty");
        }

        if (vocabPath.empty()) {
            throw std::invalid_argument("BertMailClassifier: vocabPath is empty");
        }

        if (labelMapPath.empty()) {
                throw std::runtime_error("BertMailClassifier: labelMapPath is empty");
        }

        if (m_maxLength < 8) {
            throw std::invalid_argument("BertMailClassifier: maxLength too small");
        }

        LoadVocab(vocabPath);
        LoadLabelMap(labelMapPath);
        m_sessionOptions.SetIntraOpNumThreads(1);
        m_sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        m_session = std::make_unique<Ort::Session>(m_env, modelPath.c_str(), m_sessionOptions);

        LoadInputNames();
        LoadOutputNames();
    }

    void BertMailClassifier::LoadVocab(const std::string& vocabPath) {
        std::ifstream file(vocabPath);
        if (!file.is_open()) {
            throw std::runtime_error("BertMailClassifier: failed to open vocab: " + vocabPath);
        }

        std::string line;
        int64_t id = 0;

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty()) {
                m_vocab.emplace(line, id++);
            }
        }

        const char* requiredTokens[] = {
            "[PAD]",
            "[UNK]",
            "[CLS]",
            "[SEP]"
        };

        for (const char* token : requiredTokens) {
            if (m_vocab.find(token) == m_vocab.end()) {
                throw std::runtime_error(
                    std::string("BertMailClassifier: vocab missing required token: ") + token
                );
            }
        }
    }

    void BertMailClassifier::LoadLabelMap(const std::string& labelMapPath) {
        std::ifstream file(labelMapPath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("BertMailClassifier: failed to open label_map.json: " + labelMapPath);
        }

        std::ostringstream oss;
        oss << file.rdbuf();

        LikesProgram::Configuration cfg;
        cfg.Load(LikesProgram::String(oss.str().c_str()));
        m_idToLabel.clear();

        for (auto it = cfg.beginObject(); it != cfg.endObject(); ++it) {
            LikesProgram::String label = it->first;
            int64_t id = 0;
            if (!it->second.TryGet(id)) throw std::runtime_error("BertMailClassifier: invalid label_map value for label: " + label.ToStdString());
            m_idToLabel[id] = label.ToStdString();
        }

        if (m_idToLabel.empty()) throw std::runtime_error("BertMailClassifier: label_map.json is empty");
    }

    void BertMailClassifier::LoadInputNames() {
        Ort::AllocatorWithDefaultOptions allocator;

        size_t count = m_session->GetInputCount();
        m_inputNames.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            auto name = m_session->GetInputNameAllocated(i, allocator);
            m_inputNames.emplace_back(name.get());
        }
    }

    void BertMailClassifier::LoadOutputNames() {
        Ort::AllocatorWithDefaultOptions allocator;

        size_t count = m_session->GetOutputCount();
        m_outputNames.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            auto name = m_session->GetOutputNameAllocated(i, allocator);
            m_outputNames.emplace_back(name.get());
        }

        if (m_outputNames.empty()) {
            throw std::runtime_error("BertMailClassifier: ONNX model has no output");
        }
    }

    int64_t BertMailClassifier::TokenId(const std::string& token) const {
        auto it = m_vocab.find(token);
        if (it != m_vocab.end()) {
            return it->second;
        }

        return m_vocab.at("[UNK]");
    }

    std::string BertMailClassifier::ToLowerAscii(std::string_view input) {
        std::string out;
        out.reserve(input.size());

        for (unsigned char c : input) {
            out.push_back(static_cast<char>(std::tolower(c)));
        }

        return out;
    }

    std::vector<BertMailClassifier::Utf8Char>
    BertMailClassifier::DecodeUtf8(std::string_view input) {
        std::vector<Utf8Char> chars;

        for (size_t i = 0; i < input.size();) {
            unsigned char c = static_cast<unsigned char>(input[i]);
            Utf8Char ch;

            if ((c & 0x80) == 0) {
                ch.codepoint = c;
                ch.bytes.assign(1, static_cast<char>(c));
                i += 1;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < input.size()) {
                ch.codepoint =
                    ((c & 0x1F) << 6) |
                    (static_cast<unsigned char>(input[i + 1]) & 0x3F);

                ch.bytes.assign(input.substr(i, 2));
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < input.size()) {
                ch.codepoint =
                    ((c & 0x0F) << 12) |
                    ((static_cast<unsigned char>(input[i + 1]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(input[i + 2]) & 0x3F);

                ch.bytes.assign(input.substr(i, 3));
                i += 3;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < input.size()) {
                ch.codepoint =
                    ((c & 0x07) << 18) |
                    ((static_cast<unsigned char>(input[i + 1]) & 0x3F) << 12) |
                    ((static_cast<unsigned char>(input[i + 2]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(input[i + 3]) & 0x3F);

                ch.bytes.assign(input.substr(i, 4));
                i += 4;
            } else {
                ch.codepoint = c;
                ch.bytes.assign(1, static_cast<char>(c));
                i += 1;
            }

            chars.push_back(std::move(ch));
        }

        return chars;
    }

    bool BertMailClassifier::IsChineseChar(uint32_t cp) {
        return
            (cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0x20000 && cp <= 0x2A6DF) ||
            (cp >= 0x2A700 && cp <= 0x2B73F) ||
            (cp >= 0x2B740 && cp <= 0x2B81F) ||
            (cp >= 0x2B820 && cp <= 0x2CEAF) ||
            (cp >= 0xF900 && cp <= 0xFAFF);
    }

    bool BertMailClassifier::IsWhitespace(uint32_t cp) {
        return cp == ' ' ||
               cp == '\t' ||
               cp == '\n' ||
               cp == '\r';
    }

    bool BertMailClassifier::IsPunctuation(uint32_t cp) {
        if ((cp >= 33 && cp <= 47) ||
            (cp >= 58 && cp <= 64) ||
            (cp >= 91 && cp <= 96) ||
            (cp >= 123 && cp <= 126)) {
            return true;
        }

        return
            (cp >= 0x3000 && cp <= 0x303F) ||
            (cp >= 0xFF00 && cp <= 0xFFEF);
    }

    std::vector<std::string>
    BertMailClassifier::BasicTokenize(std::string_view text) const {
        std::vector<std::string> tokens;
        std::string current;

        auto flushCurrent = [&]() {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        };

        for (const Utf8Char& ch : DecodeUtf8(text)) {
            if (IsWhitespace(ch.codepoint)) {
                flushCurrent();
                continue;
            }

            if (IsChineseChar(ch.codepoint) || IsPunctuation(ch.codepoint)) {
                flushCurrent();
                tokens.push_back(ch.bytes);
                continue;
            }

            if (ch.codepoint < 128) {
                current.push_back(
                    static_cast<char>(
                        std::tolower(static_cast<unsigned char>(ch.bytes[0]))
                    )
                );
            } else {
                current += ch.bytes;
            }
        }

        flushCurrent();
        return tokens;
    }

    std::vector<std::string>
    BertMailClassifier::WordPieceTokenize(const std::string& token) const {
        constexpr size_t MAX_INPUT_CHARS_PER_WORD = 100;

        std::vector<Utf8Char> chars = DecodeUtf8(token);

        if (chars.size() > MAX_INPUT_CHARS_PER_WORD) {
            return {"[UNK]"};
        }

        std::vector<std::string> subTokens;
        size_t start = 0;

        while (start < chars.size()) {
            size_t end = chars.size();
            std::string currentSubstr;

            while (start < end) {
                std::string piece;

                for (size_t i = start; i < end; ++i) {
                    piece += chars[i].bytes;
                }

                if (start > 0) {
                    piece = "##" + piece;
                }

                if (m_vocab.find(piece) != m_vocab.end()) {
                    currentSubstr = piece;
                    break;
                }

                --end;
            }

            if (currentSubstr.empty()) {
                return {"[UNK]"};
            }

            subTokens.push_back(currentSubstr);
            start = end;
        }

        return subTokens;
    }

    std::vector<std::string>
    BertMailClassifier::Tokenize(std::string_view text) const {
        std::vector<std::string> output;

        std::vector<std::string> basicTokens = BasicTokenize(text);

        for (const std::string& token : basicTokens) {
            std::vector<std::string> pieces = WordPieceTokenize(token);
            output.insert(output.end(), pieces.begin(), pieces.end());
        }

        return output;
    }

    std::vector<int64_t>
    BertMailClassifier::BuildInputIds(const std::vector<std::string>& tokens) const {
        std::vector<int64_t> ids;
        ids.reserve(m_maxLength);

        ids.push_back(TokenId("[CLS]"));

        for (const std::string& token : tokens) {
            if (ids.size() + 1 >= m_maxLength) break;
            ids.push_back(TokenId(token));
        }

        ids.push_back(TokenId("[SEP]"));

        const int64_t padId = TokenId("[PAD]");

        while (ids.size() < m_maxLength) ids.push_back(padId);

        return ids;
    }

    std::vector<int64_t>
    BertMailClassifier::BuildAttentionMask(size_t validLength) const {
        std::vector<int64_t> mask;
        mask.reserve(m_maxLength);

        for (size_t i = 0; i < m_maxLength; ++i) {
            mask.push_back(i < validLength ? 1 : 0);
        }

        return mask;
    }

    std::vector<int64_t>
    BertMailClassifier::BuildTokenTypeIds() const {
        return std::vector<int64_t>(m_maxLength, 0);
    }

    std::vector<float>
    BertMailClassifier::Softmax(const float* logits, size_t count) {
        std::vector<float> probs(count);

        if (count == 0) {
            return probs;
        }

        float maxLogit = logits[0];

        for (size_t i = 1; i < count; ++i) {
            maxLogit = std::max(maxLogit, logits[i]);
        }

        float sum = 0.0f;

        for (size_t i = 0; i < count; ++i) {
            probs[i] = std::exp(logits[i] - maxLogit);
            sum += probs[i];
        }

        if (sum <= 0.0f) {
            return probs;
        }

        for (float& p : probs) {
            p /= sum;
        }

        return probs;
    }

    BertClassifyResult BertMailClassifier::Predict(std::string_view text) {
        std::vector<std::string> tokens = Tokenize(text);

        size_t validLength = std::min(tokens.size() + 2, m_maxLength);

        std::vector<int64_t> inputIds = BuildInputIds(tokens);
        std::vector<int64_t> attentionMask = BuildAttentionMask(validLength);
        std::vector<int64_t> tokenTypeIds = BuildTokenTypeIds();

        std::array<int64_t, 2> inputShape = {
            1,
            static_cast<int64_t>(m_maxLength)
        };

        Ort::MemoryInfo memoryInfo =
            Ort::MemoryInfo::CreateCpu(
                OrtArenaAllocator,
                OrtMemTypeDefault
            );

        std::vector<Ort::Value> inputTensors;
        std::vector<const char*> inputNamePtrs;

        for (const std::string& inputName : m_inputNames) {
            if (inputName == "input_ids") {
                inputTensors.push_back(
                    Ort::Value::CreateTensor<int64_t>(
                        memoryInfo,
                        inputIds.data(),
                        inputIds.size(),
                        inputShape.data(),
                        inputShape.size()
                    )
                );

                inputNamePtrs.push_back(inputName.c_str());
            } else if (inputName == "attention_mask") {
                inputTensors.push_back(
                    Ort::Value::CreateTensor<int64_t>(
                        memoryInfo,
                        attentionMask.data(),
                        attentionMask.size(),
                        inputShape.data(),
                        inputShape.size()
                    )
                );

                inputNamePtrs.push_back(inputName.c_str());
            } else if (inputName == "token_type_ids") {
                inputTensors.push_back(
                    Ort::Value::CreateTensor<int64_t>(
                        memoryInfo,
                        tokenTypeIds.data(),
                        tokenTypeIds.size(),
                        inputShape.data(),
                        inputShape.size()
                    )
                );

                inputNamePtrs.push_back(inputName.c_str());
            }
        }

        if (inputTensors.empty()) {
            throw std::runtime_error(
                "BertMailClassifier::Predict failed: no recognized ONNX input name"
            );
        }

        std::vector<const char*> outputNamePtrs;
        outputNamePtrs.reserve(m_outputNames.size());

        for (const std::string& outputName : m_outputNames) {
            outputNamePtrs.push_back(outputName.c_str());
        }

        std::vector<Ort::Value> outputs = m_session->Run(
            Ort::RunOptions{nullptr},
            inputNamePtrs.data(),
            inputTensors.data(),
            inputTensors.size(),
            outputNamePtrs.data(),
            outputNamePtrs.size()
        );

        if (outputs.empty()) {
            throw std::runtime_error(
                "BertMailClassifier::Predict failed: ONNX returned no outputs"
            );
        }

        auto tensorInfo = outputs[0].GetTensorTypeAndShapeInfo();
        std::vector<int64_t> shape = tensorInfo.GetShape();

        if (shape.empty()) {
            throw std::runtime_error(
                "BertMailClassifier::Predict failed: invalid output shape"
            );
        }

        size_t classCount = static_cast<size_t>(shape.back());

        float* logits = outputs[0].GetTensorMutableData<float>();

        std::vector<float> probabilities = Softmax(logits, classCount);

        auto maxIt = std::max_element(probabilities.begin(), probabilities.end());

        BertClassifyResult result;

        if (maxIt != probabilities.end()) {
            result.classId = static_cast<int>(std::distance(probabilities.begin(), maxIt));
            result.confidence = *maxIt;
        }

        auto labelIt = m_idToLabel.find(result.classId);
        if (labelIt != m_idToLabel.end()) result.label = labelIt->second;
        else result.label = "unknown";

        result.probabilities = std::move(probabilities);

        return result;
    }
}