#pragma once
#include <string>

struct SemanticVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    std::string prerelease; // "alpha", "beta", "rc1" etc.
    int build = 0;

    static SemanticVersion FromString(const std::string& str);
    static SemanticVersion Current();
    std::string ToString() const;
    bool IsNewerThan(const SemanticVersion& other) const;
    bool IsValid() const { return major >= 0 && minor >= 0 && patch >= 0; }
};

struct ReleaseInfo {
    SemanticVersion version;
    std::string downloadUrl;
    std::string changelog;
    std::string checksum; // SHA-256
    size_t fileSize = 0;
    bool mandatory = false;
    std::string minWindowsVersion; // "10.0.19041" etc
};
