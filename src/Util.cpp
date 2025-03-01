#include <algorithm>
#include <array>
#include <unordered_set>

#include "hscpp/Util.h"
#include "hscpp/Log.h"
#include "hscpp/FsPathHasher.h"

namespace hscpp { namespace util
{

    const static std::unordered_set<std::string> HEADER_EXTENSIONS = {
        ".h",
        ".hh",
        ".hpp",
    };

    const static std::unordered_set<std::string> SOURCE_EXTENSIONS = {
        ".cpp",
        ".c",
        ".cc",
        ".cxx",
    };

    bool IsWhitespace(const std::string& str)
    {
        return std::all_of(str.begin(), str.end(), ::isspace);
    }

    std::string Trim(const std::string& str)
    {
        std::string whitespace = " \t\n\v\f\r";

        size_t iFirst = str.find_first_not_of(whitespace);
        size_t iLast = str.find_last_not_of(whitespace);

        if (iFirst != std::string::npos && iLast != std::string::npos)
        {
            return str.substr(iFirst, iLast - iFirst + 1);
        }

        return "";
    }

    std::string Quote(const std::string& str)
    {
        return "\"" + str + "\"";
    }

    std::string UnixSlashes(const std::string& str)
    {
        std::string replacedStr = str;
        std::replace(replacedStr.begin(), replacedStr.end(), '\\', '/');

        return replacedStr;
    }

    std::string DosSlashes(const std::string& str)
    {
        std::string replacedStr = str;
        std::replace(replacedStr.begin(), replacedStr.end(), '/', '\\');

        return replacedStr;
    }

    std::string FindAndReplace(const std::string& str, const std::string& toFind, const std::string& toReplace)
    {
        std::string replacedStr = str;
        size_t pos = 0;
        // Keep finding and replacing until the substring is not found anymore
        while ((pos = replacedStr.find(toFind, pos)) != std::string::npos) {
            replacedStr.replace(pos, toFind.length(), toReplace);
            pos += toReplace.length();  // Move past the last replacement to avoid infinite loop
        }

        return replacedStr;
    }

    std::string NinjaBuildEscape(const std::string& str)
    {
       return FindAndReplace(str, ":", "$:");
    }

    bool IsHeaderFile(const fs::path& filePath)
    {
        fs::path extension = filePath.extension();
        return HEADER_EXTENSIONS.find(extension.string()) != HEADER_EXTENSIONS.end();
    }

    bool IsSourceFile(const fs::path& filePath)
    {
        fs::path extension = filePath.extension();
        return SOURCE_EXTENSIONS.find(extension.string()) != SOURCE_EXTENSIONS.end();
    }

    fs::path GetHscppIncludePath()
    {
        return fs::path(HSCPP_ROOT_PATH) / "include";
    }

    fs::path GetHscppSourcePath()
    {
        return fs::path(HSCPP_ROOT_PATH) / "src";
    }

    fs::path GetHscppExamplesPath()
    {
        return fs::path(HSCPP_ROOT_PATH) / "examples";
    }

    fs::path GetHscppTestPath()
    {
        return fs::path(HSCPP_ROOT_PATH) / "test";
    }

    fs::path GetHscppBuildPath()
    {
        return fs::path(HSCPP_BUILD_PATH);
    }

    fs::path GetHscppBuildExamplesPath()
    {
        return fs::path(HSCPP_BUILD_PATH) / "examples";
    }

    fs::path GetHscppBuildTestPath()
    {
        return fs::path(HSCPP_BUILD_PATH) / "test";
    }

    void SortFileEvents(const std::vector<IFileWatcher::Event>& events,
                        std::vector<fs::path>& canonicalModifiedFilePaths,
                        std::vector<fs::path>& canonicalRemovedFilePaths)
    {
        canonicalModifiedFilePaths.clear();
        canonicalRemovedFilePaths.clear();

        // A file may have duplicate events, compress these into a single event.
        std::unordered_set<fs::path, FsPathHasher> dedupedModifiedFilePaths;
        std::unordered_set<fs::path, FsPathHasher> dedupedRemovedFilePaths;

        for (const auto& event : events)
        {
            // If the file was removed, we cannot get its canonical filename through the relative
            // filename. However, we can get the canonical path of its parent directory, and use
            // that to construct the canonical path of the file.
            std::error_code error;
            fs::path directoryPath = event.filePath.parent_path();
            fs::path canonicalDirectoryPath = fs::canonical(directoryPath, error);

            if (error.value() == HSCPP_ERROR_FILE_NOT_FOUND)
            {
                // Entire directory was removed. Hscpp does not support this use case.
                log::Warning() << "Directory " << directoryPath << " was removed; hscpp does not support "
                    << "removing directories at runtime." << log::End();
                continue;
            }

            // Construct the canonical path of the file. Note that this also works for deleted files.
            fs::path canonicalFilePath = canonicalDirectoryPath / event.filePath.filename();

            if (fs::exists(canonicalFilePath))
            {
                // Make sure this isn't a directory.
                if (fs::is_regular_file(canonicalFilePath))
                {
                    // Had a file event and the file exists; it must have been added or modified.
                    dedupedModifiedFilePaths.insert(canonicalFilePath);
                }
            }
            else
            {
                // Had a file event and the file no longer exists; it must have been deleted.
                dedupedRemovedFilePaths.insert(canonicalFilePath);
            }
        }

        canonicalModifiedFilePaths = std::vector<fs::path>(
                dedupedModifiedFilePaths.begin(), dedupedModifiedFilePaths.end());
        canonicalRemovedFilePaths = std::vector<fs::path>(
                dedupedRemovedFilePaths.begin(), dedupedRemovedFilePaths.end());
    }

    fs::path FindFile(const fs::path& rootPath, const fs::path& name)
    {
        if (!fs::exists(rootPath)) {
            return fs::path();
        }

        fs::path directory = fs::canonical(rootPath);

        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                if (fs::exists(entry)) {
                    if (entry.path().filename().compare(name) == 0) {
                        return entry.path();
                    }
                }
            }
        }

        log::Warning() << HSCPP_LOG_PREFIX << "Unable to find file " << name << " within " << directory << log::End();

        return fs::path();
    }

    std::string GetHscppSharedLib(const std::string& str)
    {
        static std::string prefix(HSCPP_SHARED_LIBRARY_PREFIX);
        static std::string suffix(HSCPP_SHARED_LIBRARY_SUFFIX);
        return prefix + str + suffix;
    }

    std::string GetHscppStaticLib(const std::string& str)
    {
        static std::string prefix(HSCPP_STATIC_LIBRARY_PREFIX);
        static std::string suffix(HSCPP_STATIC_LIBRARY_SUFFIX);
        return prefix + str + suffix;
    }

}}