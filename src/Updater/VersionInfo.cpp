#include "VersionInfo.h"
#include <sstream>
#include <vector>

SemanticVersion SemanticVersion::FromString(const std::string& str) {
    SemanticVersion v;
    std::string s = str;
    // Remove 'v' prefix
    if (!s.empty() && s[0] == 'v') s = s.substr(1);

    // Split by '-'
    size_t dash = s.find('-');
    std::string core = (dash != std::string::npos) ? s.substr(0, dash) : s;
    if (dash != std::string::npos) v.prerelease = s.substr(dash + 1);

    // Split core by '.'
    std::vector<int> parts;
    std::stringstream ss(core);
    std::string part;
    while (std::getline(ss, part, '.')) {
        try { parts.push_back(std::stoi(part)); } catch (...) { parts.push_back(0); }
    }
    if (parts.size() >= 1) v.major = parts[0];
    if (parts.size() >= 2) v.minor = parts[1];
    if (parts.size() >= 3) v.patch = parts[2];
    if (parts.size() >= 4) v.build = parts[3];
    return v;
}

SemanticVersion SemanticVersion::Current() {
    // In production this would come from VERSION resource or build system
    return SemanticVersion{1, 0, 0, "alpha", 0};
}

std::string SemanticVersion::ToString() const {
    std::string s = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (build > 0) s += "." + std::to_string(build);
    if (!prerelease.empty()) s += "-" + prerelease;
    return s;
}

bool SemanticVersion::IsNewerThan(const SemanticVersion& other) const {
    if (major != other.major) return major > other.major;
    if (minor != other.minor) return minor > other.minor;
    if (patch != other.patch) return patch > other.patch;
    if (build != other.build) return build > other.build;
    // Pre-release ordering: empty > non-empty (stable > alpha/beta)
    if (prerelease.empty() && !other.prerelease.empty()) return true;
    if (!prerelease.empty() && other.prerelease.empty()) return false;
    return prerelease > other.prerelease; // lexicographic for same stability
}
