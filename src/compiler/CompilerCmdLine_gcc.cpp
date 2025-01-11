#include <fstream>
#include <sstream>

#include "hscpp/compiler/CompilerCmdLine_gcc.h"
#include "hscpp/Log.h"
#include "hscpp/Util.h"

namespace hscpp
{

    CompilerCmdLine_gcc::CompilerCmdLine_gcc(CompilerConfig* pConfig)
        : m_pConfig(pConfig)
    {}

    bool CompilerCmdLine_gcc::GenerateCommandFile(const fs::path &commandFilePath,
                                                  const fs::path& moduleFilePath,
                                                  const ICompiler::Input &input)
    {
        std::ofstream commandFile(commandFilePath.u8string().c_str());
        std::stringstream command;

        if (!commandFile.is_open())
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to open command file "
                         << commandFilePath << log::End(".");
            return false;
        }

        // For paths, must convert '\' to '/', as both clang and g++ don't seem to be able to deal
        // with the Windows variants in cmdfiles, even on a Win32 platform.

        // Output module name.
        command << "-o " << util::UnixSlashes(moduleFilePath.u8string()) << std::endl;

        for (const auto& option : input.compileOptions)
        {
            command << option << std::endl;
        }

        for (const auto& option : input.linkOptions)
        {
            command << option << std::endl;
        }

        for (const auto& preprocessorDefinition : input.preprocessorDefinitions)
        {
            command << "-D " << "\"" << preprocessorDefinition << "\"" << std::endl;
        }

        for (const auto& includeDirectory : input.includeDirectoryPaths)
        {
            command << "-I " << "\"" << util::UnixSlashes(includeDirectory.u8string()) << "\"" << std::endl;
        }

        for (const auto& libraryDirectory : input.libraryDirectoryPaths)
        {
            command << "-L " << "\"" << util::UnixSlashes(libraryDirectory.u8string()) << "\"" << std::endl;
        }

        for (const auto& library : input.libraryPaths)
        {
            if (library.parent_path().empty())
            {
                command << "-l " << "\"" << library.filename().u8string() << "\"" << std::endl;
            }
            else
            {
                command << "\"" << util::UnixSlashes(library.u8string()) << "\"" << std::endl;
            }
        }

        for (const auto& file : input.sourceFilePaths)
        {
            command << "\"" << util::UnixSlashes(file.u8string()) << "\"" << std::endl;
        }

        // Print effective command line.
        log::Build() << m_pConfig->executable.u8string() << "\n" << command.str() << log::End();

        // Write command file.
        commandFile << command.str();

        return true;
    }

    bool CompilerCmdLine_gcc::GenerateNinjaBuildFile(const fs::path &commandFilePath,
                                                  const fs::path& moduleFilePath,
                                                  const ICompiler::Input &input)
    {
        std::ofstream commandFile(commandFilePath.u8string().c_str());
        std::stringstream command;

        if (!commandFile.is_open())
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to open command file "
                         << commandFilePath << log::End(".");
            return false;
        }

        command << "cflags =";
        for (const auto& option : input.compileOptions) {
            command << " " << option;
        }
        for (const auto& preprocessorDefinition : input.preprocessorDefinitions) {
            command << " -D " << "\"" << preprocessorDefinition << "\"";
        }
        for (const auto& includeDirectory : input.includeDirectoryPaths)
        {
            command << " -I " << "\"" << util::UnixSlashes(includeDirectory.u8string()) << "\"";
        }
        command << std::endl;
        command << "lflags =";
        for (const auto& option : input.compileOptions) {
            command << " " << option;
        }
        for (const auto& option : input.linkOptions)
        {
            command << " " << option;
        }
        for (const auto& libraryDirectory : input.libraryDirectoryPaths)
        {
            command << " -L " << "\"" << util::UnixSlashes(libraryDirectory.u8string()) << "\"";
        }
        for (const auto& library : input.libraryPaths)
        {
            if (library.parent_path().empty())
            {
                command << " -l " << "\"" << library.filename().u8string() << "\"";
            }
            else
            {
                command << " \"" << util::UnixSlashes(library.u8string()) << "\"";
            }
        }
        command << std::endl;

        command << "rule cc" << std::endl;
        command << "  depfile = $out.d" << std::endl;
        command << "  deps = gcc" << std::endl;
        command << "  command = " << m_pConfig->executable.u8string() << " $cflags -MD -MT $out -MF $out.d -o $out -c $in" << std::endl;

        command << "rule ld" << std::endl;
        command << "  command = " << m_pConfig->executable.u8string() << " $lflags -o $out $in" << std::endl;
        command << "  restat = 1" << std::endl;

        std::vector<fs::path> objFiles;
        for (const auto& file : input.sourceFilePaths)
        {
            std::filesystem::path relativePath = std::filesystem::relative(file, m_pConfig->projPath);
            relativePath.replace_extension(".o");
            auto buildOut = input.buildDirectoryPath / relativePath;
            objFiles.push_back(buildOut);
            command << "build " << util::UnixSlashes(buildOut.u8string()) << ": ";
            command << "cc " << util::UnixSlashes(file.u8string());
            command << std::endl;
        }

        command << "build " << util::UnixSlashes(moduleFilePath.u8string()) << ": ";
        command << "ld";

        for (const auto& buildOut : objFiles)
        {
            command << " " << util::UnixSlashes(buildOut.u8string());
        }
        command << std::endl;

        // Print effective command line.
        log::Build() << m_pConfig->ninjaExecutable.u8string()
                     << " -C " << commandFilePath.parent_path().u8string()
                     << log::End();

        // Write command file.
        commandFile << command.str();

        return true;
    }

}