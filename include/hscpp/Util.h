#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include "hscpp/Platform.h"

namespace hscpp { namespace util
{

    bool IsWhitespace(const std::string& str);
    std::string Trim(const std::string& str);
    std::string Quote(const std::string& str);

    std::string UnixSlashes(const std::string& str);
    std::string DosSlashes(const std::string& str);
    std::string FindAndReplace(const std::string& str, const std::string& toFind, const std::string& toReplace);
    std::string NinjaBuildEscape(const std::string& str);

    bool IsHeaderFile(const fs::path& filePath);
    bool IsSourceFile(const fs::path& filePath);

    fs::path GetHscppIncludePath();
    fs::path GetHscppSourcePath();
    fs::path GetHscppExamplesPath();
    fs::path GetHscppTestPath();
    fs::path GetHscppBuildPath();
    fs::path GetHscppBuildExamplesPath();
    fs::path GetHscppBuildTestPath();
    std::string GetHscppSharedLib(const std::string& str);
    std::string GetHscppStaticLib(const std::string& str);

    fs::path FindFile(const fs::path& rootPath, const fs::path& name);

    void SortFileEvents(const std::vector<IFileWatcher::Event>& events,
                        std::vector<fs::path>& canonicalModifiedFilePaths,
                        std::vector<fs::path>& canonicalRemovedFilePaths);

    template <typename T, typename THasher=std::hash<T>>
    void Deduplicate(std::vector<T>& input)
    {
        std::unordered_set<T, THasher> seen;
        input.erase(std::remove_if(input.begin(), input.end(), [&seen](const T& val){
            if (seen.find(val) != seen.end())
            {
                return true;
            }

            seen.insert(val);
            return false;
        }), input.end());
    }

}}