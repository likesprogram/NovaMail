#pragma once
#include <LikesProgram/Configuration.hpp>
#include <onnxruntime_cxx_api.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace NovaMail::Classification {
    struct BertClassifyResult {
        int classId = -1;
        std::string label;
        float confidence = 0.0f;
        std::vector<float> probabilities;
    };

    class BertMailClassifier {
    public:
        BertMailClassifier(
            const std::string& modelPath,
            const std::string& vocabPath,
            const std::string& labelMapPath,
            size_t maxLength = 256
        );

        BertMailClassifier(const BertMailClassifier&) = delete;
        BertMailClassifier& operator=(const BertMailClassifier&) = delete;

        BertMailClassifier(BertMailClassifier&&) noexcept = default;
        BertMailClassifier& operator=(BertMailClassifier&&) noexcept = default;

        BertClassifyResult Predict(std::string_view text);

    private:
        struct Utf8Char {
            uint32_t codepoint = 0;
            std::string bytes;
        };

    private:
        void LoadVocab(const std::string& vocabPath);
        void LoadLabelMap(const std::string& labelMapPath);
        void LoadInputNames();
        void LoadOutputNames();

        int64_t TokenId(const std::string& token) const;

        std::vector<std::string> Tokenize(std::string_view text) const;
        std::vector<std::string> BasicTokenize(std::string_view text) const;
        std::vector<std::string> WordPieceTokenize(const std::string& token) const;

        std::vector<int64_t> BuildInputIds(const std::vector<std::string>& tokens) const;
        std::vector<int64_t> BuildAttentionMask(size_t validLength) const;
        std::vector<int64_t> BuildTokenTypeIds() const;

        static std::vector<Utf8Char> DecodeUtf8(std::string_view input);
        static bool IsChineseChar(uint32_t cp);
        static bool IsWhitespace(uint32_t cp);
        static bool IsPunctuation(uint32_t cp);
        static std::string ToLowerAscii(std::string_view input);
        static std::vector<float> Softmax(const float* logits, size_t count);

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_sessionOptions;
        std::unique_ptr<Ort::Session> m_session;

        std::unordered_map<std::string, int64_t> m_vocab;
        std::unordered_map<int64_t, std::string> m_idToLabel;

        std::vector<std::string> m_inputNames;
        std::vector<std::string> m_outputNames;

        size_t m_maxLength = 256;
    };
}